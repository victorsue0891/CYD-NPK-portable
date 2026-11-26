#include "TouchInput.h"
#include <Preferences.h>
#include <math.h>

constexpr int TouchInput::targetX[3];
constexpr int TouchInput::targetY[3];
void TouchInput::begin() {
    spi.begin(config::kTouchClk, config::kTouchMiso, config::kTouchMosi, config::kTouchCs);
    touch.begin(spi);
    touch.setRotation(1);
    Preferences prefs;
    size_t got = 0;
    // Create the namespace on first boot, but do not fabricate calibration data.
    if (prefs.begin("npk-touch", false)) {
        if (prefs.isKey("affine") && prefs.getBytesLength("affine") == sizeof(cal))
            got = prefs.getBytes("affine", &cal, sizeof(cal));
        prefs.end();
    }
    calibrated = got == sizeof(cal) && cal.magic == 0x4e504b31 &&
        isfinite(cal.ax) && isfinite(cal.bx) && isfinite(cal.cx) &&
        isfinite(cal.ay) && isfinite(cal.by) && isfinite(cal.cy);
    // Always show NPK at boot. Without a valid mapping, ignore touch presses.
    step = 3;
    Serial.println(calibrated ? "Touch calibration loaded; opening NPK screen." :
        "Opening NPK screen; touch disabled until calibration (hold BOOT 2 s or send c).");
}
void TouchInput::calibrate() {
    step = 0;
    samples = 0;
    sumX = sumY = 0;
    wasDown = false;
    releaseRequired = true;
    failed = false;
}
bool TouchInput::solve() {
    const float dx1 = rawX[1]-rawX[0], dy1 = rawY[1]-rawY[0];
    const float dx2 = rawX[2]-rawX[0], dy2 = rawY[2]-rawY[0];
    const float det = dx1*dy2-dx2*dy1;
    if (fabsf(det) < 200000) return false;
    cal.magic = 0x4e504b31;
    cal.ax = 272*dy2/det; cal.bx = -272*dx2/det;
    cal.ay = -192*dy1/det; cal.by = 192*dx1/det;
    cal.cx = 24-cal.ax*rawX[0]-cal.bx*rawY[0];
    cal.cy = 24-cal.ay*rawX[0]-cal.by*rawY[0];
    Preferences prefs;
    prefs.begin("npk-touch", false);
    const size_t saved = prefs.putBytes("affine", &cal, sizeof(cal));
    prefs.end();
    calibrated = true;
    Serial.println(saved == sizeof(cal) ? "Touch calibration saved." : "Touch calibration active; NVS save failed.");
    return true;
}
bool TouchInput::tick(int& x, int& y) {
    if (!calibrated && !calibrating()) return false;
    const uint32_t now = millis();
    if (now-lastSample < 15) return false;
    lastSample = now;
    const bool down = touch.touched();
    if (releaseRequired) {
        if (!down) releaseRequired = false;
        return false;
    }
    if (calibrating()) {
        if (down) {
            const TS_Point p = touch.getPoint();
            if (samples < 16) { sumX += p.x; sumY += p.y; ++samples; }
        } else if (samples) {
            if (samples >= 8) {
                rawX[step] = float(sumX)/samples;
                rawY[step] = float(sumY)/samples;
                ++step;
                if (step == 3 && !solve()) { step = 0; failed = true; }
            }
            samples = 0;
            sumX = sumY = 0;
        }
        return false;
    }
    if (!down) { wasDown = false; downAt = now; return false; }
    if (wasDown) return false;
    if (now-downAt < 50) return false;
    wasDown = true;
    const TS_Point p = touch.getPoint();
    x = lroundf(cal.ax*p.x+cal.bx*p.y+cal.cx);
    y = lroundf(cal.ay*p.x+cal.by*p.y+cal.cy);
    return x >= 0 && x < 320 && y >= 0 && y < 240;
}
