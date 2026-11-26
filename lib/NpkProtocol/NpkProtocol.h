#pragma once
#include <stddef.h>
#include <stdint.h>

namespace npk {
struct Reading { uint16_t n = 0, p = 0, k = 0; };
enum class Result { Ok, Length, Address, Function, Crc, Exception, Range };
struct Reply {
    Result result = Result::Length;
    Reading reading;
    uint8_t exception = 0;
};
uint16_t crc16(const uint8_t* data, size_t size);
void makeRequest(uint8_t address, uint8_t (&out)[8]);
Reply parseReply(const uint8_t* data, size_t size, uint8_t address);
// Remove only an exact local echo of this request. Do not search noise for values.
size_t stripEcho(const uint8_t* data, size_t size, const uint8_t (&request)[8]);
}
