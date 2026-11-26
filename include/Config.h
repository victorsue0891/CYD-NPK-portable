#pragma once
#include <stdint.h>

namespace config {
constexpr int kUartTx = 22; // CN1 -> MAX3485 DI
constexpr int kUartRx = 35; // P3 <- MAX3485 RO; external 10k pull-up to 3.3V
constexpr int kRs485Dir = 27; // CN1 -> DE and /RE tied together; LOW = receive
constexpr uint32_t kRs485EnableUs = 10; // Conservative driver-enable settling time
constexpr uint8_t kAddress = 1;
constexpr uint32_t kBaud = 9600;
constexpr uint32_t kPollMs = 2000;
constexpr uint32_t kTimeoutMs = 700;
constexpr uint32_t kFrameGapMs = 6; // > 4 characters at 9600 / 8N1
constexpr uint32_t kSettleMs = 10000; // Engineering wait; not proof of stability
constexpr uint32_t kStaleMs = 5000;
constexpr int kTouchClk = 25;
constexpr int kTouchMiso = 39;
constexpr int kTouchMosi = 32;
constexpr int kTouchCs = 33;
constexpr int kTouchIrq = 36;
constexpr int kBoot = 0;
static_assert(kAddress >= 1 && kAddress <= 247, "Use a normal Modbus slave address");
static_assert(kUartTx != kUartRx && kRs485Dir != kUartTx && kRs485Dir != kUartRx,
              "RS485 TX, RX and direction must use distinct pins");
}
