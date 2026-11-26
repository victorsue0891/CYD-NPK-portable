#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

extern uint32_t mockNow;
inline uint32_t millis() { return mockNow; }
constexpr int SERIAL_8N1 = 0;
constexpr int LOW = 0, HIGH = 1, OUTPUT = 1;
struct MockPort {
    uint8_t rx[1024]{}, tx[8]{};
    size_t count = 0, cursor = 0;
    int writes = 0;
    int uartRx = -1, uartTx = -1, dirLevel = -1, dirMode = -1;
    int txCompleted = 0, receiveReleases = 0;
    uint32_t enableDelayUs = 0;
    bool txInFlight = false;
    const uint8_t* immediateReply = nullptr;
    size_t immediateReplySize = 0;
    void feed(const uint8_t* data, size_t size) {
        count = size; cursor = 0; memcpy(rx, data, size);
    }
};
extern MockPort port;
inline void digitalWrite(int pin, int level) {
    assert(pin == 27);
    if (level == LOW) {
        assert(!port.txInFlight); // Cannot drop DE before the stop bit finishes.
        if (port.dirLevel == HIGH) {
            ++port.receiveReleases;
            if (port.immediateReply) port.feed(port.immediateReply, port.immediateReplySize);
        }
    } else {
        assert(port.dirMode == OUTPUT);
        port.enableDelayUs = 0;
    }
    port.dirLevel = level;
}
inline void pinMode(int pin, int mode) {
    assert(pin == 27 && mode == OUTPUT);
    assert(port.dirLevel == LOW); // Safe latch value before enabling output.
    port.dirMode = mode;
}
inline void delayMicroseconds(uint32_t us) { port.enableDelayUs += us; }
class HardwareSerial {
public:
    explicit HardwareSerial(int) {}
    void begin(uint32_t baud, int, int rx, int tx) {
        assert(baud == 9600 && port.dirLevel == LOW && port.dirMode == OUTPUT);
        port.uartRx = rx; port.uartTx = tx;
    }
    int available() { return int(port.count-port.cursor); }
    int read() { return available() ? port.rx[port.cursor++] : -1; }
    size_t write(const uint8_t* data, size_t size) {
        assert(port.dirLevel == HIGH && port.enableDelayUs >= 1);
        assert(!port.txInFlight);
        port.txInFlight = true;
        memcpy(port.tx, data, size); ++port.writes; return size;
    }
    void flush(bool txOnly) {
        assert(txOnly); // Must not discard queued RX bytes.
        assert(port.txInFlight && port.dirLevel == HIGH);
        port.txInFlight = false;
        ++port.txCompleted;
    }
};
struct MockConsole {
    template<typename... Args> void printf(const char*, Args...) {}
    void println(const char* = "") {}
    void print(const char*) {}
};
extern MockConsole Serial;
