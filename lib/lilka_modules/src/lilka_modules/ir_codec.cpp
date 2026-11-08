#include "ir_codec.h"

namespace lilka {
namespace mod {
namespace IrCodec {

// NEC / Samsung timings
static const uint16_t NEC_HDR_MARK = 9000;
static const uint16_t NEC_HDR_SPACE = 4500;
static const uint16_t NEC_RPT_SPACE = 2250;
static const uint16_t SAMSUNG_HDR_MARK = 4500;
static const uint16_t SAMSUNG_HDR_SPACE = 4500;
static const uint16_t PULSE_BIT_MARK = 560;
static const uint16_t PULSE_ONE_SPACE = 1690;
static const uint16_t PULSE_ZERO_SPACE = 560;
// Sony SIRC timings
static const uint16_t SONY_HDR_MARK = 2400;
static const uint16_t SONY_SPACE = 600;
static const uint16_t SONY_ONE_MARK = 1200;
static const uint16_t SONY_ZERO_MARK = 600;

// Demodulating receivers stretch/shrink marks by ~100 us, so be generous.
static bool match(uint16_t measured, uint16_t expected) {
    uint32_t tol = expected * 30 / 100 + 100;
    return measured + tol >= expected && measured <= expected + tol;
}

static size_t encodePulseDistance(uint16_t hdrMark, uint16_t hdrSpace, uint32_t data, uint16_t* out, size_t max) {
    const size_t need = 2 + 32 * 2 + 1;
    if (max < need) return 0;
    size_t n = 0;
    out[n++] = hdrMark;
    out[n++] = hdrSpace;
    for (int i = 0; i < 32; i++) {
        out[n++] = PULSE_BIT_MARK;
        out[n++] = (data >> i) & 1 ? PULSE_ONE_SPACE : PULSE_ZERO_SPACE;
    }
    out[n++] = PULSE_BIT_MARK;
    return n;
}

static bool decodePulseDistance(const uint16_t* d, size_t count, uint16_t hdrMark, uint16_t hdrSpace, uint32_t& data) {
    if (count < 2 + 32 * 2 + 1) return false;
    if (!match(d[0], hdrMark) || !match(d[1], hdrSpace)) return false;
    data = 0;
    for (int i = 0; i < 32; i++) {
        uint16_t mark = d[2 + i * 2], space = d[3 + i * 2];
        if (!match(mark, PULSE_BIT_MARK)) return false;
        if (match(space, PULSE_ONE_SPACE)) {
            data |= 1UL << i;
        } else if (!match(space, PULSE_ZERO_SPACE)) {
            return false;
        }
    }
    return match(d[66], PULSE_BIT_MARK);
}

size_t encode(const IrCode& code, uint16_t* out, size_t max) {
    switch (code.protocol) {
        case IrProtocol::NEC: {
            if (code.repeat) return encodeNecRepeat(out, max);
            uint32_t addr =
                code.address > 0xFF ? (code.address & 0xFFFF) : ((code.address & 0xFF) | ((~code.address & 0xFF) << 8));
            uint32_t data = addr | ((code.command & 0xFF) << 16) | ((~code.command & 0xFF) << 24);
            return encodePulseDistance(NEC_HDR_MARK, NEC_HDR_SPACE, data, out, max);
        }
        case IrProtocol::Samsung: {
            uint32_t a = code.address & 0xFF;
            uint32_t data = a | (a << 8) | ((code.command & 0xFF) << 16) | ((~code.command & 0xFF) << 24);
            return encodePulseDistance(SAMSUNG_HDR_MARK, SAMSUNG_HDR_SPACE, data, out, max);
        }
        case IrProtocol::Sony: {
            uint8_t bits = code.bits == 15 || code.bits == 20 ? code.bits : 12;
            size_t need = 2 + bits * 2 - 1;
            if (max < need) return 0;
            uint32_t data = (code.command & 0x7F) | (code.address << 7);
            size_t n = 0;
            out[n++] = SONY_HDR_MARK;
            out[n++] = SONY_SPACE;
            for (int i = 0; i < bits; i++) {
                out[n++] = (data >> i) & 1 ? SONY_ONE_MARK : SONY_ZERO_MARK;
                if (i != bits - 1) out[n++] = SONY_SPACE;
            }
            return n;
        }
        default:
            return 0;
    }
}

size_t encodeNecRepeat(uint16_t* out, size_t max) {
    if (max < 3) return 0;
    out[0] = NEC_HDR_MARK;
    out[1] = NEC_RPT_SPACE;
    out[2] = PULSE_BIT_MARK;
    return 3;
}

static bool decodeSony(const uint16_t* d, size_t count, IrCode& out) {
    if (count < 2 || !match(d[0], SONY_HDR_MARK) || !match(d[1], SONY_SPACE)) return false;
    size_t marks = (count - 1) / 2; // header space + (mark, space)*; the last space may be missing
    if (marks != 12 && marks != 15 && marks != 20) return false;
    uint32_t data = 0;
    for (size_t i = 0; i < marks; i++) {
        uint16_t mark = d[2 + i * 2];
        if (match(mark, SONY_ONE_MARK)) {
            data |= 1UL << i;
        } else if (!match(mark, SONY_ZERO_MARK)) {
            return false;
        }
        if (i != marks - 1 && !match(d[3 + i * 2], SONY_SPACE)) return false;
    }
    out.protocol = IrProtocol::Sony;
    out.command = data & 0x7F;
    out.address = data >> 7;
    out.bits = marks;
    out.repeat = false;
    return true;
}

bool decode(const uint16_t* d, size_t count, IrCode& out) {
    out = {};
    uint32_t data;
    if (count >= 3 && count < 10 && match(d[0], NEC_HDR_MARK) && match(d[1], NEC_RPT_SPACE) &&
        match(d[2], PULSE_BIT_MARK)) {
        out.protocol = IrProtocol::NEC;
        out.repeat = true;
        return true;
    }
    if (decodePulseDistance(d, count, NEC_HDR_MARK, NEC_HDR_SPACE, data)) {
        uint8_t a = data & 0xFF, na = (data >> 8) & 0xFF, c = (data >> 16) & 0xFF, nc = data >> 24;
        if ((uint8_t)~c != nc) return false;
        out.protocol = IrProtocol::NEC;
        out.address = (uint8_t)~a == na ? a : (data & 0xFFFF);
        out.command = c;
        out.bits = 32;
        return true;
    }
    if (decodePulseDistance(d, count, SAMSUNG_HDR_MARK, SAMSUNG_HDR_SPACE, data)) {
        uint8_t c = (data >> 16) & 0xFF, nc = data >> 24;
        if ((uint8_t)~c != nc) return false;
        out.protocol = IrProtocol::Samsung;
        out.address = data & 0xFF;
        out.command = c;
        out.bits = 32;
        return true;
    }
    return decodeSony(d, count, out);
}

uint32_t repeatGapMs(IrProtocol protocol) {
    switch (protocol) {
        case IrProtocol::Sony:
            return 25; // 45 ms frame period
        case IrProtocol::NEC:
        case IrProtocol::Samsung:
            return 40; // ~108 ms frame period
        default:
            return 50;
    }
}

const char* name(IrProtocol protocol) {
    switch (protocol) {
        case IrProtocol::NEC:
            return "NEC";
        case IrProtocol::Samsung:
            return "SAMSUNG";
        case IrProtocol::Sony:
            return "SONY";
        default:
            return "UNKNOWN";
    }
}

} // namespace IrCodec
} // namespace mod
} // namespace lilka
