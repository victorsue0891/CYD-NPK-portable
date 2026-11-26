#include "Sensor.h"
#include "Config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uint32_t mockNow = 0;
MockPort port;
MockConsole Serial;
const uint8_t good[] = {1,3,6,0,37,0,49,0,40,0x3d,0x63};
const uint8_t badManual[] = {1,3,6,0,37,0,49,0,40,0x41,0x07};
void crc(uint8_t* data, size_t n) {
    auto c = npk::crc16(data, n-2); data[n-2] = c & 255; data[n-1] = c >> 8;
}
void tick(Sensor& s, uint32_t time) {
    mockNow = time; s.tick(time);
    assert(port.dirLevel == LOW && !port.txInFlight);
    assert(port.writes == port.txCompleted && port.writes == port.receiveReleases);
}
void start(Sensor& s, uint32_t base = 0) {
    port = MockPort{}; mockNow = base; s.begin();
    assert(port.uartRx == 35 && port.uartTx == 22);
    assert(port.dirLevel == LOW && port.dirMode == OUTPUT);
    tick(s, base+9999); assert(port.writes == 0);
    tick(s, base+10000); assert(port.writes == 1);
}
void reply(Sensor& s, const uint8_t* bytes, size_t size, uint32_t time) {
    port.feed(bytes, size); tick(s,time); tick(s,time+6);
}
int main() {
    uint8_t request[8]; npk::makeRequest(1,request);
    const uint8_t expected[] = {1,3,0,0x1e,0,3,0x65,0xcd};
    assert(memcmp(request,expected,8) == 0);
    const auto parsed = npk::parseReply(good,sizeof(good),1);
    assert(parsed.result == npk::Result::Ok);
    assert(parsed.reading.n == 37 && parsed.reading.p == 49 && parsed.reading.k == 40);
    assert(npk::parseReply(badManual,sizeof(badManual),1).result == npk::Result::Crc);
    assert(npk::parseReply(good,sizeof(good),2).result == npk::Result::Address);
    for (size_t n=0; n<sizeof(good); ++n) assert(npk::parseReply(good,n,1).result != npk::Result::Ok);
    for (size_t bit=0; bit<sizeof(good)*8; ++bit) {
        uint8_t broken[11]; memcpy(broken,good,11); broken[bit/8] ^= 1<<(bit%8);
        assert(npk::parseReply(broken,11,1).result != npk::Result::Ok);
    }
    uint8_t data[11]; memcpy(data,good,11); data[1]=4; crc(data,11);
    assert(npk::parseReply(data,11,1).result == npk::Result::Function);
    memcpy(data,good,11); data[2]=4; crc(data,11);
    assert(npk::parseReply(data,11,1).result == npk::Result::Length);
    memcpy(data,good,11); data[3]=7; data[4]=0xd0; crc(data,11);
    assert(npk::parseReply(data,11,1).result == npk::Result::Range);
    data[4]=0xcf; crc(data,11); assert(npk::parseReply(data,11,1).reading.n == 1999);
    memset(data+3,0,6); crc(data,11); assert(npk::parseReply(data,11,1).result == npk::Result::Ok);
    uint8_t exception[] = {1,0x83,2,0,0}; crc(exception,5);
    assert(npk::parseReply(exception,5,1).result == npk::Result::Exception);
    assert(npk::parseReply(exception,5,1).exception == 2);
    uint8_t echoed[19]; memcpy(echoed,request,8); memcpy(echoed+8,good,11);
    assert(npk::stripEcho(echoed,19,request) == 8);
    assert(npk::stripEcho(good,11,request) == 0);
    puts("Protocol: known request, corrected reply, bad manual CRC, truncation, 88 bit flips, address/function/length/range, exception, echo: PASS");

    {
        Sensor s; start(s);
        reply(s,good,11,10020);
        assert(s.hasValue && s.value.n == 37 && s.goodCount == 1 && s.state == SensorState::Live);
        tick(s,12000); tick(s,12700);
        assert(s.state == SensorState::Timeout && s.hasValue && s.lastGood == 10026);
        tick(s,14000); reply(s,good,11,14020);
        assert(s.state == SensorState::Live && s.goodCount == 2);
        s.togglePause(); const auto writes = port.writes;
        tick(s,20000); assert(port.writes == writes && s.paused());
        s.togglePause(); assert(!s.hasValue && s.state == SensorState::Settling);
        tick(s,30000); assert(port.writes == writes+1);
    }
    {
        Sensor s; start(s); reply(s,badManual,11,10020);
        assert(!s.hasValue && s.state == SensorState::Invalid && s.errorCount == 1);
        tick(s,12000); reply(s,exception,5,12020);
        assert(s.state == SensorState::Exception && s.exception == 2);
    }
    {
        Sensor s; start(s); reply(s,echoed,19,10020);
        assert(s.hasValue && s.value.k == 40);
    }
    {
        Sensor s; start(s); reply(s,request,8,10010);
        assert(!s.hasValue && !s.errorCount);
        reply(s,good,11,10030); assert(s.hasValue);
    }
    {
        Sensor s; start(s);
        port.feed(good,4); tick(s,10010);
        port.feed(good+4,7); tick(s,10012); tick(s,10018);
        assert(s.hasValue); // Fragmented receive, no frame gap.
    }
    {
        Sensor s; start(s); reply(s,good,4,10020);
        assert(!s.hasValue && s.state == SensorState::Invalid);
    }
    {
        Sensor s; start(s); uint8_t noise[80]{}; reply(s,noise,80,10020);
        assert(!s.hasValue && s.state == SensorState::Invalid);
    }
    {
        Sensor s; start(s); s.togglePause(); reply(s,good,11,10020);
        assert(s.paused() && !s.hasValue); // In-flight reply must not unfreeze display.
    }
    {
        Sensor s; const uint32_t base = UINT32_MAX-5000; start(s,base);
        reply(s,good,11,base+10020); assert(s.hasValue);
        tick(s,base+12000); tick(s,base+12700); assert(s.state == SensorState::Timeout);
    }
    {
        Sensor s; start(s);
        tick(s,10700); assert(s.state == SensorState::Timeout);
        // A response available immediately after DE falls must survive turnaround.
        port.immediateReply = good; port.immediateReplySize = sizeof(good);
        tick(s,12000); tick(s,12001); tick(s,12007);
        assert(s.hasValue && s.value.n == 37 && s.goodCount == 1);
    }
    puts("MAX3485: TX22/RX35/DIR27, safe startup, enable delay, DE held through TX completion, receive release, immediate reply: PASS");
    puts("Sensor state machine: warmup, polling, timeout, recovery, stale retention, pause/resume, errors, echo, fragments, overflow, millis wrap: PASS");
    return 0;
}
