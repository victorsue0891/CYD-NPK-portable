#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "Config.h"

class TouchInput {
public:
    void begin();
    void calibrate();
    // Returns a new press only. Calibration consumes input until complete.
    bool tick(int& x, int& y);
    bool calibrating() const { return step < 3; }
    bool ready() const { return calibrated && !calibrating(); }
    int calibrationStep() const { return step; }
    bool failed = false;
    static constexpr int targetX[3] = {24, 296, 24};
    static constexpr int targetY[3] = {24, 24, 216};
private:
    SPIClass spi{VSPI};
    XPT2046_Touchscreen touch{config::kTouchCs, config::kTouchIrq};
    struct Calibration { uint32_t magic; float ax, bx, cx, ay, by, cy; } cal{};
    int step = 3, samples = 0;
    bool wasDown = false, releaseRequired = false, calibrated = false;
    int32_t sumX = 0, sumY = 0;
    float rawX[3]{}, rawY[3]{};
    uint32_t lastSample = 0, downAt = 0;
    bool solve();
};
