#include "cc1101.h"

#include <Arduino.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

namespace lilka {
namespace mod {

enum : uint8_t {
    IOCFG2 = 0x00,
    IOCFG0 = 0x02,
    FIFOTHR = 0x03,
    SYNC1 = 0x04,
    SYNC0 = 0x05,
    PKTLEN = 0x06,
    PKTCTRL1 = 0x07,
    PKTCTRL0 = 0x08,
    FSCTRL1 = 0x0B,
    FREQ2 = 0x0D,
    FREQ1 = 0x0E,
    FREQ0 = 0x0F,
    MDMCFG4 = 0x10,
    MDMCFG3 = 0x11,
    MDMCFG2 = 0x12,
    MDMCFG1 = 0x13,
    DEVIATN = 0x15,
    MCSM1 = 0x17,
    MCSM0 = 0x18,
    FOCCFG = 0x19,
    AGCCTRL2 = 0x1B,
    FREND0 = 0x22,
    FSCAL3 = 0x23,
    FSCAL2 = 0x24,
    FSCAL1 = 0x25,
    FSCAL0 = 0x26,
    TEST2 = 0x2C,
    TEST1 = 0x2D,
    TEST0 = 0x2E,
    PARTNUM = 0x30,
    VERSION = 0x31,
    RSSI = 0x34,
    MARCSTATE = 0x35,
    RXBYTES = 0x3B,
    PATABLE = 0x3E,
    FIFO = 0x3F,
    SRES = 0x30,
    SCAL = 0x33,
    SRX = 0x34,
    STX = 0x35,
    SIDLE = 0x36,
    SFRX = 0x3A,
    SFTX = 0x3B,
    BURST = 0x40,
    READ = 0x80,
    MARC_IDLE = 0x01,
    MARC_RX = 0x0D,
    MARC_RXFIFO_OVERFLOW = 0x11,
    MARC_TXFIFO_UNDERFLOW = 0x16,
};

static uint32_t nowMs() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

Cc1101::~Cc1101() {
    end();
}

uint8_t Cc1101::xfer(uint8_t header, const uint8_t* tx, uint8_t* rx, size_t n) {
    cfg.spi->beginTransaction(settings);
    digitalWrite(cfg.csn, LOW);
    // CHIP_RDYn: MISO goes low when the crystal is running.
    if (cfg.miso >= 0) {
        int64_t t0 = esp_timer_get_time();
        while (digitalRead(cfg.miso) && esp_timer_get_time() - t0 < 5000) {
        }
    }
    uint8_t st = cfg.spi->transfer(header);
    for (size_t i = 0; i < n; i++) {
        uint8_t r = cfg.spi->transfer(tx ? tx[i] : 0);
        if (rx) rx[i] = r;
    }
    digitalWrite(cfg.csn, HIGH);
    cfg.spi->endTransaction();
    return st;
}

uint8_t Cc1101::strobe(uint8_t s) {
    return xfer(s, nullptr, nullptr, 0);
}

void Cc1101::writeRegister(uint8_t reg, uint8_t value) {
    xfer(reg, &value, nullptr, 1);
}

uint8_t Cc1101::readRegister(uint8_t reg) {
    uint8_t v = 0;
    xfer(reg | READ, nullptr, &v, 1);
    return v;
}

uint8_t Cc1101::readStatus(uint8_t reg) {
    uint8_t v = 0;
    xfer(reg | READ | BURST, nullptr, &v, 1);
    return v;
}

uint8_t Cc1101::marcState() {
    return readStatus(MARCSTATE) & 0x1F;
}

bool Cc1101::waitMarc(uint8_t state, uint32_t ms) {
    uint32_t t0 = nowMs();
    while (marcState() != state) {
        if (nowMs() - t0 > ms) return false;
        vTaskDelay(1);
    }
    return true;
}

uint8_t Cc1101::partNumber() {
    return readStatus(PARTNUM);
}

uint8_t Cc1101::version() {
    return readStatus(VERSION);
}

void Cc1101::reset() {
    // Manual power-on reset (datasheet 19.1.2).
    digitalWrite(cfg.csn, HIGH);
    delayMicroseconds(5);
    digitalWrite(cfg.csn, LOW);
    delayMicroseconds(10);
    digitalWrite(cfg.csn, HIGH);
    delayMicroseconds(45);
    strobe(SRES);
    vTaskDelay(pdMS_TO_TICKS(1));
}

bool Cc1101::setFrequency(float mhz) {
    bool ok = (mhz >= 300 && mhz <= 348) || (mhz >= 387 && mhz <= 464) || (mhz >= 779 && mhz <= 928);
    if (!ok) return false;
    cfg.frequencyMhz = mhz;
    if (!running) return true;
    bool wasReceiving = receiving;
    idle();
    uint32_t f = (uint32_t)(mhz * 1e6 * 65536.0 / cfg.xtalHz + 0.5);
    writeRegister(FREQ2, (f >> 16) & 0xFF);
    writeRegister(FREQ1, (f >> 8) & 0xFF);
    writeRegister(FREQ0, f & 0xFF);
    setPower(cfg.powerDbm); // PA table depends on the band
    strobe(SCAL);
    waitMarc(MARC_IDLE, 10);
    if (wasReceiving) startReceive();
    return true;
}

void Cc1101::setPower(int8_t dbm) {
    cfg.powerDbm = dbm;
    // TI DN013 PA tables for -30, -20, -15, -10, 0, 5, 7, 10 dBm
    static const int8_t levels[8] = {-30, -20, -15, -10, 0, 5, 7, 10};
    static const uint8_t t315[8] = {0x12, 0x0D, 0x1C, 0x34, 0x51, 0x85, 0xCB, 0xC2};
    static const uint8_t t433[8] = {0x12, 0x0E, 0x1D, 0x34, 0x60, 0x84, 0xC8, 0xC0};
    static const uint8_t t868[8] = {0x03, 0x0F, 0x1E, 0x27, 0x50, 0x81, 0xCB, 0xC2};
    static const uint8_t t915[8] = {0x03, 0x0E, 0x1E, 0x27, 0x8E, 0xCD, 0xC7, 0xC0};
    const uint8_t* table = t433;
    if (cfg.frequencyMhz < 350) {
        table = t315;
    } else if (cfg.frequencyMhz > 900) {
        table = t915;
    } else if (cfg.frequencyMhz > 700) {
        table = t868;
    }
    int idx = 0;
    for (int i = 0; i < 8; i++) {
        if (levels[i] <= dbm) idx = i;
    }
    const uint8_t pa[2] = {0x00, table[idx]};
    if (cfg.modulation == Modulation::ASK) {
        // OOK: PATABLE[0] = off level, PATABLE[1] = on level
        xfer(PATABLE | BURST, pa, nullptr, 2);
        writeRegister(FREND0, 0x11);
    } else {
        writeRegister(PATABLE, table[idx]);
        writeRegister(FREND0, 0x10);
    }
}

bool Cc1101::configure() {
    const double xtal = cfg.xtalHz;
    if (cfg.dataRate < 600 || cfg.dataRate > 500000) return false;

    // Data rate: R = (256 + M) * 2^E * fxosc / 2^28
    int drE = (int)floor(log2(cfg.dataRate * pow(2, 20) / xtal));
    int drM = (int)lround(cfg.dataRate * pow(2, 28) / (xtal * pow(2, drE)) - 256);
    if (drM == 256) {
        drM = 0;
        drE++;
    }
    if (drE < 0 || drE > 15) return false;

    // RX filter bandwidth: BW = fxosc / (8 * (4 + M) * 2^E); pick the narrowest >= requested
    int bwE = 0, bwM = 0;
    for (int e = 3; e >= 0; e--) {
        bool found = false;
        for (int m = 3; m >= 0; m--) {
            double bw = xtal / (8.0 * (4 + m) * (1 << e));
            if (bw >= cfg.bandwidthHz) {
                bwE = e;
                bwM = m;
                found = true;
                break;
            }
        }
        if (found) break;
    }

    // Deviation: f = fxosc / 2^17 * (8 + M) * 2^E; pick the closest
    int devE = 0, devM = 0;
    double best = 1e12;
    for (int e = 0; e < 8; e++) {
        for (int m = 0; m < 8; m++) {
            double d = xtal / 131072.0 * (8 + m) * (1 << e);
            if (fabs(d - cfg.deviationHz) < best) {
                best = fabs(d - cfg.deviationHz);
                devE = e;
                devM = m;
            }
        }
    }

    strobe(SIDLE);
    writeRegister(IOCFG2, 0x29); // CHIP_RDYn
    writeRegister(IOCFG0, 0x06); // asserts on sync word, deasserts at end of packet
    writeRegister(FIFOTHR, 0x47);
    writeRegister(SYNC1, cfg.syncWord >> 8);
    writeRegister(SYNC0, cfg.syncWord & 0xFF);
    writeRegister(PKTLEN, MAX_PACKET);
    writeRegister(PKTCTRL1, 0x04); // append RSSI/LQI, no address check
    writeRegister(PKTCTRL0, cfg.crc ? 0x05 : 0x01); // variable length (+ CRC)
    writeRegister(FSCTRL1, cfg.bandwidthHz > 325000 ? 0x0C : 0x06);
    writeRegister(MDMCFG4, (uint8_t)((bwE << 6) | (bwM << 4) | drE));
    writeRegister(MDMCFG3, (uint8_t)drM);
    // 30/32 sync bits, no Manchester
    writeRegister(MDMCFG2, (uint8_t)((static_cast<uint8_t>(cfg.modulation) << 4) | 0x03));
    writeRegister(MDMCFG1, 0x22); // 4 preamble bytes
    writeRegister(DEVIATN, (uint8_t)((devE << 4) | devM));
    writeRegister(MCSM1, 0x30); // back to IDLE after RX and TX
    writeRegister(MCSM0, 0x18); // autocal IDLE -> RX/TX
    writeRegister(FOCCFG, 0x16);
    writeRegister(AGCCTRL2, 0x43);
    writeRegister(FSCAL3, 0xE9);
    writeRegister(FSCAL2, 0x2A);
    writeRegister(FSCAL1, 0x00);
    writeRegister(FSCAL0, 0x1F);
    writeRegister(TEST2, 0x81);
    writeRegister(TEST1, 0x35);
    writeRegister(TEST0, 0x09);
    running = true;
    return setFrequency(cfg.frequencyMhz);
}

bool Cc1101::begin(const Config& conf) {
    end();
    cfg = conf;
    if (!cfg.spi) return false;
    settings = SPISettings(cfg.spiFrequency, MSBFIRST, SPI_MODE0);
    pinMode(cfg.csn, OUTPUT);
    digitalWrite(cfg.csn, HIGH);
    if (cfg.gdo0 >= 0) pinMode(cfg.gdo0, INPUT);
    if (cfg.gdo2 >= 0) pinMode(cfg.gdo2, INPUT);
    reset();
    uint8_t ver = version();
    if (partNumber() != 0x00 || ver == 0x00 || ver == 0xFF) return false;
    if (!configure()) {
        running = false;
        return false;
    }
    return true;
}

void Cc1101::end() {
    if (!running) return;
    idle();
    running = false;
}

void Cc1101::idle() {
    strobe(SIDLE);
    waitMarc(MARC_IDLE, 10);
    receiving = false;
}

void Cc1101::startReceive() {
    if (!running) return;
    idle();
    strobe(SFRX);
    strobe(SRX);
    receiving = true;
}

bool Cc1101::send(const uint8_t* data, uint8_t length, uint32_t timeoutMs) {
    if (!running || length > MAX_PACKET) return false;
    bool wasReceiving = receiving;
    idle();
    strobe(SFTX);
    uint8_t buf[MAX_PACKET + 1];
    buf[0] = length;
    for (uint8_t i = 0; i < length; i++)
        buf[i + 1] = data[i];
    xfer(FIFO | BURST, buf, nullptr, length + 1);
    strobe(STX);
    // Leaves IDLE for TX, comes back to IDLE (MCSM1) when the packet is out.
    vTaskDelay(1);
    bool ok = waitMarc(MARC_IDLE, timeoutMs);
    if (!ok || marcState() == MARC_TXFIFO_UNDERFLOW) {
        strobe(SIDLE);
        strobe(SFTX);
        ok = false;
    }
    if (wasReceiving) startReceive();
    return ok;
}

bool Cc1101::receive(Packet& packet) {
    if (!running || !receiving) return false;
    uint8_t state = marcState();
    if (state == MARC_RXFIFO_OVERFLOW) {
        startReceive();
        return false;
    }
    if (state != MARC_IDLE) return false; // still receiving
    // Errata: read RXBYTES until two reads agree.
    uint8_t bytes = readStatus(RXBYTES), again;
    while ((again = readStatus(RXBYTES)) != bytes)
        bytes = again;
    bytes &= 0x7F;
    if (bytes < 3) {
        startReceive();
        return false;
    }
    uint8_t len = 0;
    xfer(FIFO | READ, nullptr, &len, 1);
    if (len == 0 || len > MAX_PACKET || len + 3 > bytes) {
        startReceive();
        return false;
    }
    xfer(FIFO | READ | BURST, nullptr, packet.data, len);
    uint8_t st[2];
    xfer(FIFO | READ | BURST, nullptr, st, 2);
    packet.length = len;
    packet.rssi = (int8_t)st[0] / 2.0f - 74.0f;
    packet.lqi = st[1] & 0x7F;
    packet.crcOk = st[1] & 0x80;
    startReceive();
    return true;
}

float Cc1101::rssi() {
    return (int8_t)readStatus(RSSI) / 2.0f - 74.0f;
}

} // namespace mod
} // namespace lilka
