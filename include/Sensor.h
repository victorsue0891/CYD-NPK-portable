#pragma once
#include <Arduino.h>
#include <NpkProtocol.h>

enum class SensorState { Settling, Waiting, Live, Paused, Timeout, Invalid, Exception };
class Sensor {
public:
    void begin();
    void tick(uint32_t now);
    void togglePause();
    void remeasure();
    bool paused() const { return state == SensorState::Paused; }
    uint32_t settleRemaining(uint32_t now) const;
    SensorState state = SensorState::Settling;
    npk::Reading value;
    npk::Result lastError = npk::Result::Ok;
    uint8_t exception = 0;
    bool hasValue = false;
    uint32_t lastGood = 0, goodCount = 0, errorCount = 0;
private:
    HardwareSerial uart{2};
    uint8_t request[8]{}, rx[64]{};
    size_t used = 0;
    bool pending = false, overflow = false, settling = true;
    uint32_t settleStart = 0, sentAt = 0, lastRx = 0, lastPoll = 0;
    void finish(const npk::Reply& reply, uint32_t now);
};
