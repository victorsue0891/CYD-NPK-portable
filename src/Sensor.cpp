#include "Sensor.h"
#include "Config.h"

void Sensor::begin() {
    // Preload the output latch before enabling the pin. The external pull-down
    // keeps DE low during reset, before firmware can take control.
    digitalWrite(config::kRs485Dir, LOW);
    pinMode(config::kRs485Dir, OUTPUT);
    uart.begin(config::kBaud, SERIAL_8N1, config::kUartRx, config::kUartTx);
    npk::makeRequest(config::kAddress, request);
    remeasure();
}
void Sensor::remeasure() {
    pending = false;
    used = 0;
    hasValue = false;
    settling = true;
    settleStart = millis();
    state = SensorState::Settling;
    Serial.println("New measurement: allow 10 s for sensor response; verify stability in soil.");
}
void Sensor::togglePause() {
    if (paused()) { remeasure(); return; }
    pending = false;
    state = SensorState::Paused;
}
uint32_t Sensor::settleRemaining(uint32_t now) const {
    const uint32_t elapsed = now - settleStart;
    return elapsed >= config::kSettleMs ? 0 : config::kSettleMs - elapsed;
}
void Sensor::finish(const npk::Reply& reply, uint32_t now) {
    pending = false;
    lastError = reply.result;
    exception = reply.exception;
    if (reply.result == npk::Result::Ok) {
        value = reply.reading;
        lastGood = now;
        hasValue = true;
        ++goodCount;
        state = SensorState::Live;
        Serial.printf("N=%u P=%u K=%u mg/kg; uptime_ms=%lu\n", value.n, value.p, value.k, static_cast<unsigned long>(now));
    } else {
        ++errorCount;
        state = reply.result == npk::Result::Exception ? SensorState::Exception : SensorState::Invalid;
        Serial.printf("Invalid response: reason=%d exception=%u\n", int(lastError), exception);
    }
}
void Sensor::tick(uint32_t now) {
    // Bounded drain keeps UI responsive even on a noisy line.
    for (int i = 0; i < 128 && uart.available(); ++i) {
        const int byte = uart.read();
        lastRx = now;
        if (pending) {
            if (used < sizeof(rx)) rx[used++] = uint8_t(byte);
            else overflow = true;
        }
    }
    if (paused()) return;
    if (settling) {
        if (settleRemaining(now)) return;
        settling = false;
        state = SensorState::Waiting;
        lastPoll = now - config::kPollMs;
    }
    if (pending) {
        if (used && now - lastRx >= config::kFrameGapMs) {
            const size_t skip = npk::stripEcho(rx, used, request);
            if (skip == used && !overflow) {
                used = 0; // Local echo alone; continue waiting for the sensor.
            } else {
                Serial.print("RX:");
                for (size_t i = 0; i < used; ++i) Serial.printf(" %02X", rx[i]);
                Serial.println();
                finish(overflow ? npk::Reply{} : npk::parseReply(rx + skip, used - skip, config::kAddress), now);
            }
        }
        if (pending && now - sentAt >= config::kTimeoutMs) {
            pending = false;
            state = SensorState::Timeout;
            ++errorCount;
            Serial.println("RS485 timeout; check power, A/B and address.");
        }
        return;
    }
    // Silence also prevents the next request colliding with a late response.
    if (now - lastPoll >= config::kPollMs && now - lastRx >= config::kFrameGapMs && !uart.available()) {
        used = 0;
        overflow = false;
        pending = true;
        lastPoll = now;
        digitalWrite(config::kRs485Dir, HIGH); // Enable driver, disable receiver.
        delayMicroseconds(config::kRs485EnableUs);
        uart.write(request, sizeof(request));
        // Arduino ESP32 2.0.17 waits for UART TX idle, including the stop bit.
        // true preserves RX data; never clear a prompt sensor reply here.
        uart.flush(true);
        digitalWrite(config::kRs485Dir, LOW); // Release A/B and enable receiver.
        sentAt = millis();
    }
}
