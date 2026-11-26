#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ChineseFont.h"
#include "Config.h"
#include "Sensor.h"
#include "TouchInput.h"
#include "UiText.h"

TFT_eSPI tft;
TFT_eSprite canvas(&tft);
Sensor sensor;
TouchInput input;
constexpr uint16_t BG = 0x0842, CARD = 0x10c4, WHITE = 0xef7d;
constexpr uint16_t MUTED = 0x8c92, GREEN = 0x572e, AMBER = 0xfd29, RED = 0xfaaa;
constexpr uint16_t ACCENT[3] = {0x572e, 0x55ff, 0xfd29};

void chinese(const char* text, int x, int y, uint16_t color) {
    canvas.setTextColor(color);
    const uint8_t* p = reinterpret_cast<const uint8_t*>(text);
    while (*p) {
        uint32_t cp = *p++;
        if ((cp & 0xf0) == 0xe0 && p[0] && p[1]) {
            cp = ((cp & 15) << 12) | ((p[0] & 63) << 6) | (p[1] & 63);
            p += 2;
        } else if ((cp & 0xe0) == 0xc0 && p[0]) {
            cp = ((cp & 31) << 6) | (*p++ & 63);
        }
        bool found = false;
        for (const auto& glyph : chineseGlyphs) {
            if (pgm_read_dword(&glyph.code) == cp) {
                canvas.drawBitmap(x, y, glyph.bits, 18, 18, color);
                found = true;
                break;
            }
        }
        if (!found) canvas.drawChar(cp < 128 ? cp : '?', x, y, 2);
        x += cp < 128 ? 9 : 18;
    }
}
void ascii(const char* text, int x, int y, uint16_t color, int font = 2) {
    canvas.setTextColor(color);
    canvas.setTextDatum(TL_DATUM);
    canvas.drawString(text, x, y, font);
}
bool stale(uint32_t now) { return sensor.hasValue && now-sensor.lastGood > config::kStaleMs; }

void render(uint32_t now) {
    canvas.fillSprite(BG);
    if (input.calibrating()) {
        chinese(ui::Calibrate, 124, 70, WHITE);
        chinese(ui::TouchTarget, 70, 105, MUTED);
        if (input.failed) chinese(ui::CalFailed, 70, 140, AMBER);
        const int i = input.calibrationStep();
        const int x = TouchInput::targetX[i], y = TouchInput::targetY[i];
        canvas.drawCircle(x, y, 12, GREEN);
        canvas.drawFastHLine(x-18, y, 37, GREEN);
        canvas.drawFastVLine(x, y-18, 37, GREEN);
        char step[12]; snprintf(step, sizeof(step), "%d / 3", i+1);
        ascii(step, 140, 170, WHITE);
        canvas.pushSprite(0, 0);
        return;
    }
    chinese(ui::Title, 12, 7, WHITE);
    ascii("NPK / FIELD", 226, 8, MUTED);
    canvas.drawFastHLine(12, 30, 296, CARD);
    const char* status = ui::Waiting;
    uint16_t statusColor = AMBER;
    switch (sensor.state) {
        case SensorState::Settling: status = ui::Warmup; break;
        case SensorState::Waiting: status = ui::Waiting; break;
        case SensorState::Live: status = ui::Live; statusColor = GREEN; break;
        case SensorState::Paused: status = ui::Paused; break;
        case SensorState::Timeout: status = ui::Timeout; statusColor = RED; break;
        case SensorState::Invalid: status = ui::Invalid; statusColor = RED; break;
        case SensorState::Exception: status = ui::Exception; statusColor = RED; break;
    }
    if (sensor.state == SensorState::Live && stale(now)) {
        status = ui::Stale;
        statusColor = AMBER;
    }
    canvas.fillCircle(16, 44, 3, statusColor);
    chinese(status, 26, 36, statusColor);
    if (sensor.state == SensorState::Settling) {
        char count[12]; snprintf(count, sizeof(count), "%lu s", static_cast<unsigned long>((sensor.settleRemaining(now)+999)/1000));
        ascii(count, 110, 36, AMBER);
    }
    if (sensor.state == SensorState::Exception) {
        char code[8]; snprintf(code, sizeof(code), "0x%02X", sensor.exception);
        ascii(code, 214, 36, RED);
    }
    const bool old = sensor.paused() || stale(now) || sensor.state == SensorState::Timeout ||
                     sensor.state == SensorState::Invalid || sensor.state == SensorState::Exception;
    const char* labels[3] = {ui::Nitrogen, ui::Phosphorus, ui::Potassium};
    const char* names[3] = {"N", "P", "K"};
    const uint16_t values[3] = {sensor.value.n, sensor.value.p, sensor.value.k};
    for (int i = 0; i < 3; ++i) {
        const int y = 60 + i*43;
        canvas.fillRoundRect(12, y, 296, 39, 5, CARD);
        canvas.fillRoundRect(12, y, 4, 39, 2, ACCENT[i]);
        ascii(names[i], 25, y+6, ACCENT[i], 4);
        chinese(labels[i], 54, y+11, WHITE);
        char value[8];
        if (sensor.hasValue) snprintf(value, sizeof(value), "%u", values[i]);
        else snprintf(value, sizeof(value), "---");
        canvas.setTextColor(old ? MUTED : WHITE);
        canvas.setTextDatum(TR_DATUM);
        canvas.drawString(value, 238, y+6, 4);
        ascii("mg/kg", 250, y+15, MUTED);
    }
    if (sensor.hasValue) {
        chinese(old ? ui::Old : ui::Age, 12, 190, old ? AMBER : MUTED);
        char age[18]; snprintf(age, sizeof(age), "%lu", static_cast<unsigned long>((now-sensor.lastGood)/1000));
        ascii(age, 75, 190, MUTED);
        chinese(ui::SecondsAgo, 158, 190, MUTED);
    } else chinese(ui::NoData, 12, 190, MUTED);
    if (input.ready()) {
        canvas.fillRoundRect(12, 211, 142, 28, 4, CARD);
        canvas.fillRoundRect(166, 211, 142, 28, 4, 0x2249);
        chinese(sensor.paused() ? ui::Resume : ui::Pause, 65, 216, WHITE);
        chinese(ui::Measure, 201, 216, WHITE);
    } else {
        chinese(ui::TouchSetup, 52, 216, MUTED);
    }
    canvas.pushSprite(0, 0);
}

void setup() {
    Serial.begin(115200);
    pinMode(config::kBoot, INPUT_PULLUP);
    // Board RGB LEDs are active-low; leave them off.
    for (int pin : {4, 16, 17}) { pinMode(pin, OUTPUT); digitalWrite(pin, HIGH); }
    pinMode(5, OUTPUT); digitalWrite(5, HIGH); // Keep unused SD card deselected.
    tft.init();
    tft.setRotation(1);
    pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
    canvas.setColorDepth(8);
    if (!canvas.createSprite(320, 240)) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.drawString("Display buffer allocation failed", 5, 100, 2);
        while (true) delay(1000);
    }
    sensor.begin();
    input.begin();
    Serial.println("CYD NPK: h=pause/resume, r=new measurement, c=touch calibration.");
    render(millis());
}
void loop() {
    uint32_t now = millis();
    sensor.tick(now);
    int x = 0, y = 0;
    if (input.tick(x, y) && y >= 211 && y < 240) {
        if (x >= 12 && x < 154) sensor.togglePause();
        if (x >= 166 && x < 308) sensor.remeasure();
    }
    if (Serial.available()) {
        switch (Serial.read()) {
            case 'h': sensor.togglePause(); break;
            case 'r': sensor.remeasure(); break;
            case 'c': input.calibrate(); break;
        }
    }
    static bool bootDown = false, longPress = false;
    static uint32_t bootAt = 0;
    const bool down = digitalRead(config::kBoot) == LOW;
    if (down && !bootDown) { bootAt = now; longPress = false; }
    if (down && now-bootAt >= 2000 && !longPress) { input.calibrate(); longPress = true; }
    if (!down && bootDown && !longPress && now-bootAt >= 40 && !input.calibrating()) sensor.togglePause();
    bootDown = down;
    static uint32_t rendered = 0;
    if (now-rendered >= 150) { rendered = now; render(now); }
    delay(2);
}
