#include "NpkProtocol.h"
#include <string.h>

namespace npk {
uint16_t crc16(const uint8_t* data, size_t size) {
    uint16_t crc = 0xffff;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 1) ? (crc >> 1) ^ 0xa001 : crc >> 1;
    }
    return crc;
}
void makeRequest(uint8_t address, uint8_t (&out)[8]) {
    const uint8_t base[] = {address, 3, 0, 0x1e, 0, 3, 0, 0};
    memcpy(out, base, 8);
    const uint16_t crc = crc16(out, 6);
    out[6] = crc & 0xff;
    out[7] = crc >> 8;
}
Reply parseReply(const uint8_t* data, size_t size, uint8_t address) {
    Reply reply;
    if (size != 5 && size != 11) return reply;
    if (data[0] != address) { reply.result = Result::Address; return reply; }
    const uint16_t wireCrc = uint16_t(data[size-2]) | uint16_t(data[size-1]) << 8;
    if (crc16(data, size-2) != wireCrc) { reply.result = Result::Crc; return reply; }
    if (size == 5 && data[1] == 0x83) {
        reply.result = Result::Exception;
        reply.exception = data[2];
        return reply;
    }
    if (data[1] != 3) { reply.result = Result::Function; return reply; }
    if (size != 11 || data[2] != 6) return reply;
    Reading value;
    value.n = (uint16_t(data[3]) << 8) | data[4];
    value.p = (uint16_t(data[5]) << 8) | data[6];
    value.k = (uint16_t(data[7]) << 8) | data[8];
    if (value.n > 1999 || value.p > 1999 || value.k > 1999) {
        reply.result = Result::Range;
        return reply;
    }
    reply.reading = value;
    reply.result = Result::Ok;
    return reply;
}
size_t stripEcho(const uint8_t* data, size_t size, const uint8_t (&request)[8]) {
    return size >= 8 && memcmp(data, request, 8) == 0 ? 8 : 0;
}
}
