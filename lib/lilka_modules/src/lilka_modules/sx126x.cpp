#include "sx126x.h"

#include <Arduino.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

namespace lilka {
namespace mod {

// Opcodes
enum : uint8_t {
    OP_SET_SLEEP = 0x84,
    OP_SET_STANDBY = 0x80,
    OP_SET_TX = 0x83,
    OP_SET_RX = 0x82,
    OP_SET_REGULATOR = 0x96,
    OP_CALIBRATE = 0x89,
    OP_CALIBRATE_IMAGE = 0x98,
    OP_SET_PA_CONFIG = 0x95,
    OP_SET_DIO_IRQ = 0x08,
    OP_GET_IRQ = 0x12,
    OP_CLEAR_IRQ = 0x02,
    OP_SET_TCXO = 0x97,
    OP_SET_RF_FREQ = 0x86,
    OP_SET_PACKET_TYPE = 0x8A,
    OP_SET_TX_PARAMS = 0x8E,
    OP_SET_MOD_PARAMS = 0x8B,
    OP_SET_PACKET_PARAMS = 0x8C,
    OP_SET_BUFFER_BASE = 0x8F,
    OP_GET_STATUS = 0xC0,
    OP_GET_RSSI_INST = 0x15,
    OP_GET_RX_BUFFER_STATUS = 0x13,
    OP_GET_PACKET_STATUS = 0x14,
    OP_GET_DEVICE_ERRORS = 0x17,
    OP_CLEAR_DEVICE_ERRORS = 0x07,
    OP_WRITE_REGISTER = 0x0D,
    OP_READ_REGISTER = 0x1D,
    OP_WRITE_BUFFER = 0x0E,
    OP_READ_BUFFER = 0x1E,
};

enum : uint16_t {
    REG_SYNC_WORD = 0x0740,
    REG_IQ_POLARITY = 0x0736,
    REG_TX_CLAMP = 0x08D8,
    IRQ_TX_DONE = 0x0001,
    IRQ_RX_DONE = 0x0002,
    IRQ_HEADER_ERR = 0x0020,
    IRQ_CRC_ERR = 0x0040,
    IRQ_TIMEOUT = 0x0200,
};

static uint32_t nowMs() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static bool bandwidthCode(uint32_t hz, uint8_t& code) {
    static const struct {
        uint32_t hz;
        uint8_t code;
    } table[] = {
        {7800, 0x00},
        {10400, 0x08},
        {15600, 0x01},
        {20800, 0x09},
        {31250, 0x02},
        {41700, 0x0A},
        {62500, 0x03},
        {125000, 0x04},
        {250000, 0x05},
        {500000, 0x06},
    };
    for (auto& e : table) {
        if (e.hz == hz) {
            code = e.code;
            return true;
        }
    }
    return false;
}

SX126x::~SX126x() {
    end();
}

bool SX126x::waitBusy(uint32_t timeoutMs) {
    uint32_t t0 = nowMs();
    while (digitalRead(cfg.busy)) {
        if (nowMs() - t0 > timeoutMs) return false;
        delayMicroseconds(10);
    }
    return true;
}

bool SX126x::xfer(const uint8_t* tx, uint8_t* rx, size_t n) {
    if (asleep) {
        // A falling edge on NSS wakes the chip up; it then keeps BUSY high until it is ready.
        digitalWrite(cfg.nss, LOW);
        delayMicroseconds(100);
        digitalWrite(cfg.nss, HIGH);
        asleep = false;
        waitBusy(10);
    }
    if (!waitBusy()) return false;
    cfg.spi->beginTransaction(settings);
    digitalWrite(cfg.nss, LOW);
    for (size_t i = 0; i < n; i++) {
        uint8_t r = cfg.spi->transfer(tx ? tx[i] : 0);
        if (rx) rx[i] = r;
    }
    digitalWrite(cfg.nss, HIGH);
    cfg.spi->endTransaction();
    return true;
}

bool SX126x::command(const uint8_t* bytes, size_t n) {
    return xfer(bytes, nullptr, n);
}

uint8_t SX126x::readRegister(uint16_t addr) {
    const uint8_t tx[5] = {OP_READ_REGISTER, (uint8_t)(addr >> 8), (uint8_t)addr, 0, 0};
    uint8_t rx[5] = {};
    xfer(tx, rx, 5);
    return rx[4];
}

void SX126x::writeRegister(uint16_t addr, uint8_t value) {
    const uint8_t tx[4] = {OP_WRITE_REGISTER, (uint8_t)(addr >> 8), (uint8_t)addr, value};
    command(tx, 4);
}

uint8_t SX126x::status() {
    const uint8_t tx[2] = {OP_GET_STATUS, 0};
    uint8_t rx[2] = {};
    xfer(tx, rx, 2);
    return rx[1];
}

uint16_t SX126x::irqStatus() {
    const uint8_t tx[4] = {OP_GET_IRQ, 0, 0, 0};
    uint8_t rx[4] = {};
    xfer(tx, rx, 4);
    return (rx[2] << 8) | rx[3];
}

void SX126x::clearIrq() {
    const uint8_t c[3] = {OP_CLEAR_IRQ, 0xFF, 0xFF};
    command(c, 3);
}

uint16_t SX126x::deviceErrors() {
    const uint8_t tx[4] = {OP_GET_DEVICE_ERRORS, 0, 0, 0};
    uint8_t rx[4] = {};
    xfer(tx, rx, 4);
    return (rx[2] << 8) | rx[3];
}

void SX126x::writeBuffer(uint8_t offset, const uint8_t* data, size_t n) {
    if (asleep) status(); // wake up
    if (!waitBusy()) return;
    cfg.spi->beginTransaction(settings);
    digitalWrite(cfg.nss, LOW);
    cfg.spi->transfer(OP_WRITE_BUFFER);
    cfg.spi->transfer(offset);
    for (size_t i = 0; i < n; i++)
        cfg.spi->transfer(data[i]);
    digitalWrite(cfg.nss, HIGH);
    cfg.spi->endTransaction();
}

void SX126x::readBuffer(uint8_t offset, uint8_t* data, size_t n) {
    if (asleep) status(); // wake up
    if (!waitBusy()) return;
    cfg.spi->beginTransaction(settings);
    digitalWrite(cfg.nss, LOW);
    cfg.spi->transfer(OP_READ_BUFFER);
    cfg.spi->transfer(offset);
    cfg.spi->transfer(0); // status
    for (size_t i = 0; i < n; i++)
        data[i] = cfg.spi->transfer(0);
    digitalWrite(cfg.nss, HIGH);
    cfg.spi->endTransaction();
}

void SX126x::setRfSwitch(bool tx) {
    if (cfg.rxen >= 0) digitalWrite(cfg.rxen, tx ? LOW : HIGH);
    if (cfg.txen >= 0) digitalWrite(cfg.txen, tx ? HIGH : LOW);
}

bool SX126x::reset() {
    asleep = false;
    if (cfg.nrst >= 0) {
        digitalWrite(cfg.nrst, LOW);
        vTaskDelay(pdMS_TO_TICKS(2));
        digitalWrite(cfg.nrst, HIGH);
    }
    return waitBusy(200);
}

void SX126x::setPacketParams(uint8_t length) {
    const uint8_t c[7] = {
        OP_SET_PACKET_PARAMS,
        (uint8_t)(cfg.preambleLength >> 8),
        (uint8_t)cfg.preambleLength,
        0x00, // explicit header
        length,
        (uint8_t)(cfg.crc ? 0x01 : 0x00),
        (uint8_t)(cfg.invertIq ? 0x01 : 0x00),
    };
    command(c, 7);
}

SX126x::Result SX126x::setFrequency(uint32_t hz) {
    bool ok = cfg.chip == Chip::SX1268 ? (hz >= 410000000 && hz <= 493000000) : (hz >= 850000000 && hz <= 930000000);
    if (!ok) return Result::InvalidParam;
    cfg.frequencyHz = hz;
    if (!running) return Result::OK;
    bool wasReceiving = receiving;
    standby();
    uint8_t img[3] = {OP_CALIBRATE_IMAGE, 0, 0};
    if (hz < 440000000) {
        img[1] = 0x6B, img[2] = 0x6F; // 430-440 MHz
    } else if (hz < 510000000) {
        img[1] = 0x75, img[2] = 0x81; // 470-510 MHz
    } else if (hz < 900000000) {
        img[1] = 0xD7, img[2] = 0xDB; // 863-870 MHz
    } else {
        img[1] = 0xE1, img[2] = 0xE9; // 902-928 MHz
    }
    command(img, 3);
    if (!waitBusy(500)) return Result::BusyTimeout;
    uint32_t frf = (uint32_t)(((uint64_t)hz << 25) / 32000000ULL);
    const uint8_t f[5] = {
        OP_SET_RF_FREQ, (uint8_t)(frf >> 24), (uint8_t)(frf >> 16), (uint8_t)(frf >> 8), (uint8_t)frf
    };
    command(f, 5);
    return wasReceiving ? startReceive() : Result::OK;
}

SX126x::Result SX126x::setTxPower(int8_t dbm) {
    if (dbm < -9 || dbm > 22) return Result::InvalidParam;
    cfg.powerDbm = dbm;
    if (!running) return Result::OK;
    // SX1262: hpMax 0x07, SX1268: hpMax 0x06 (datasheet 13.1.14, +22 dBm rows)
    const uint8_t pa[5] = {OP_SET_PA_CONFIG, 0x04, (uint8_t)(cfg.chip == Chip::SX1262 ? 0x07 : 0x06), 0x00, 0x01};
    command(pa, 5);
    const uint8_t tx[3] = {OP_SET_TX_PARAMS, (uint8_t)dbm, 0x04}; // 200 us ramp
    command(tx, 3);
    return Result::OK;
}

SX126x::Result SX126x::configure() {
    uint8_t bw;
    if (cfg.spreadingFactor < 5 || cfg.spreadingFactor > 12 || cfg.codingRate < 5 || cfg.codingRate > 8 ||
        !bandwidthCode(cfg.bandwidthHz, bw)) {
        return Result::InvalidParam;
    }
    const uint8_t standbyRc[2] = {OP_SET_STANDBY, 0x00};
    command(standbyRc, 2);
    const uint8_t dcdc[2] = {OP_SET_REGULATOR, 0x01};
    command(dcdc, 2);
    const uint8_t tcxo[5] = {OP_SET_TCXO, 0x02, 0x00, 0x01, 0x40}; // 1.8 V, 5 ms
    command(tcxo, 5);
    const uint8_t cal[2] = {OP_CALIBRATE, 0x7F};
    command(cal, 2);
    vTaskDelay(pdMS_TO_TICKS(10));
    if (!waitBusy(500)) return Result::BusyTimeout;
    const uint8_t lora[2] = {OP_SET_PACKET_TYPE, 0x01};
    command(lora, 2);

    running = true; // setFrequency/setTxPower below talk to the chip
    Result r = setFrequency(cfg.frequencyHz);
    if (r == Result::OK) r = setTxPower(cfg.powerDbm);
    if (r != Result::OK) {
        running = false;
        return r;
    }

    // Low data rate optimisation is required when a symbol is longer than 16 ms.
    uint32_t symbolUs = (uint32_t)(((uint64_t)1 << cfg.spreadingFactor) * 1000000ULL / cfg.bandwidthHz);
    const uint8_t mod[5] = {
        OP_SET_MOD_PARAMS,
        cfg.spreadingFactor,
        bw,
        (uint8_t)(cfg.codingRate - 4),
        (uint8_t)(symbolUs >= 16000 ? 1 : 0),
    };
    command(mod, 5);
    const uint8_t base[3] = {OP_SET_BUFFER_BASE, 0x00, 0x00};
    command(base, 3);
    const uint8_t irq[9] = {OP_SET_DIO_IRQ, 0x02, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    command(irq, 9);
    setPacketParams(0xFF);

    writeRegister(REG_SYNC_WORD, (cfg.syncWord & 0xF0) | 0x04);
    writeRegister(REG_SYNC_WORD + 1, ((cfg.syncWord & 0x0F) << 4) | 0x04);
    // Errata 15.4: IQ polarity register must match the inverted-IQ setting.
    uint8_t iq = readRegister(REG_IQ_POLARITY);
    writeRegister(REG_IQ_POLARITY, cfg.invertIq ? (iq & ~0x04) : (iq | 0x04));
    // Errata 15.2: better resistance to antenna mismatch.
    writeRegister(REG_TX_CLAMP, readRegister(REG_TX_CLAMP) | 0x1E);

    uint16_t err = deviceErrors();
    if (err & 0x7F) return Result::DeviceError;
    return Result::OK;
}

SX126x::Result SX126x::begin(const Config& conf) {
    end();
    cfg = conf;
    if (!cfg.spi) return Result::InvalidParam;
    settings = SPISettings(cfg.spiFrequency, MSBFIRST, SPI_MODE0);
    pinMode(cfg.nss, OUTPUT);
    digitalWrite(cfg.nss, HIGH);
    if (cfg.nrst >= 0) pinMode(cfg.nrst, OUTPUT);
    if (cfg.rxen >= 0) pinMode(cfg.rxen, OUTPUT);
    if (cfg.txen >= 0) pinMode(cfg.txen, OUTPUT);
    pinMode(cfg.busy, INPUT);
    setRfSwitch(false);

    if (!reset()) return Result::BusyTimeout;
    // Default LoRa sync word after reset is 0x1424: proves the SPI link works.
    if (readRegister(REG_SYNC_WORD) != 0x14 || readRegister(REG_SYNC_WORD + 1) != 0x24) return Result::NoChip;
    const uint8_t clr[3] = {OP_CLEAR_DEVICE_ERRORS, 0, 0};
    command(clr, 3);

    Result r = configure();
    if (r != Result::OK) running = false;
    return r;
}

void SX126x::end() {
    if (!running) return;
    sleep();
    if (cfg.rxen >= 0) digitalWrite(cfg.rxen, LOW);
    if (cfg.txen >= 0) digitalWrite(cfg.txen, LOW);
    running = false;
}

SX126x::Result SX126x::startReceive() {
    if (!running) return Result::InvalidParam;
    setPacketParams(0xFF);
    setRfSwitch(false);
    clearIrq();
    const uint8_t rx[4] = {OP_SET_RX, 0xFF, 0xFF, 0xFF}; // continuous
    command(rx, 4);
    receiving = true;
    return Result::OK;
}

SX126x::Result SX126x::send(const uint8_t* data, uint8_t length, uint32_t timeoutMs, uint32_t* airTimeMs) {
    if (!running) return Result::InvalidParam;
    bool wasReceiving = receiving;
    standby();
    setPacketParams(length);
    writeBuffer(0, data, length);
    clearIrq();
    setRfSwitch(true);
    uint32_t t0 = nowMs();
    const uint8_t tx[4] = {OP_SET_TX, 0x00, 0x00, 0x00}; // no timeout
    command(tx, 4);
    bool done = false;
    while (nowMs() - t0 < timeoutMs) {
        if (irqStatus() & IRQ_TX_DONE) {
            done = true;
            break;
        }
        vTaskDelay(1);
    }
    if (airTimeMs) *airTimeMs = nowMs() - t0;
    setRfSwitch(false);
    if (!done) standby();
    clearIrq();
    if (wasReceiving) startReceive();
    return done ? Result::OK : Result::Timeout;
}

bool SX126x::receive(Packet& packet) {
    if (!running || !receiving) return false;
    uint16_t irq = irqStatus();
    if (!(irq & IRQ_RX_DONE)) {
        if (irq & IRQ_HEADER_ERR) clearIrq();
        return false;
    }
    const uint8_t tx[4] = {OP_GET_RX_BUFFER_STATUS, 0, 0, 0};
    uint8_t rx[4] = {};
    xfer(tx, rx, 4);
    uint8_t len = rx[2], start = rx[3];
    const uint8_t ps[5] = {OP_GET_PACKET_STATUS, 0, 0, 0, 0};
    uint8_t pr[5] = {};
    xfer(ps, pr, 5);
    readBuffer(start, packet.data, len);
    packet.length = len;
    packet.rssi = -pr[2] / 2.0f;
    packet.snr = (int8_t)pr[3] / 4.0f;
    packet.crcError = (irq & IRQ_CRC_ERR) != 0;
    clearIrq();
    return true;
}

float SX126x::rssi() {
    const uint8_t tx[3] = {OP_GET_RSSI_INST, 0, 0};
    uint8_t rx[3] = {};
    xfer(tx, rx, 3);
    return -rx[2] / 2.0f;
}

void SX126x::standby() {
    const uint8_t c[2] = {OP_SET_STANDBY, 0x00};
    command(c, 2);
    receiving = false;
}

void SX126x::sleep() {
    standby();
    const uint8_t c[2] = {OP_SET_SLEEP, 0x04}; // warm start, keep configuration
    command(c, 2);
    asleep = true;
    vTaskDelay(pdMS_TO_TICKS(1));
}

uint32_t SX126x::timeOnAirMs(uint8_t length) const {
    // Semtech AN1200.13 formula, explicit header.
    uint32_t sf = cfg.spreadingFactor;
    uint32_t symbolUs = (uint32_t)(((uint64_t)1 << sf) * 1000000ULL / cfg.bandwidthHz);
    bool ldro = symbolUs >= 16000;
    int32_t num = 8 * length - 4 * sf + 28 + (cfg.crc ? 16 : 0);
    int32_t den = 4 * (sf - (ldro ? 2 : 0));
    int32_t payloadSymbols = 8 + (num > 0 ? ((num + den - 1) / den) * (cfg.codingRate) : 0);
    uint32_t preambleUs = (cfg.preambleLength * 4 + 17) * symbolUs / 4;
    return (preambleUs + payloadSymbols * symbolUs) / 1000;
}

} // namespace mod
} // namespace lilka
