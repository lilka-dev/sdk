#include "nrf24.h"

#include <Arduino.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

namespace lilka {
namespace mod {

enum : uint8_t {
    REG_CONFIG = 0x00,
    REG_EN_AA = 0x01,
    REG_EN_RXADDR = 0x02,
    REG_SETUP_AW = 0x03,
    REG_SETUP_RETR = 0x04,
    REG_RF_CH = 0x05,
    REG_RF_SETUP = 0x06,
    REG_STATUS = 0x07,
    REG_RPD = 0x09,
    REG_RX_ADDR_P0 = 0x0A,
    REG_TX_ADDR = 0x10,
    REG_RX_PW_P0 = 0x11,
    REG_FIFO_STATUS = 0x17,
    REG_DYNPD = 0x1C,
    REG_FEATURE = 0x1D,

    CMD_R_REGISTER = 0x00,
    CMD_W_REGISTER = 0x20,
    CMD_R_RX_PAYLOAD = 0x61,
    CMD_W_TX_PAYLOAD = 0xA0,
    CMD_FLUSH_TX = 0xE1,
    CMD_FLUSH_RX = 0xE2,
    CMD_R_RX_PL_WID = 0x60,
    CMD_NOP = 0xFF,

    CONFIG_PRIM_RX = 0x01,
    CONFIG_PWR_UP = 0x02,
    CONFIG_CRCO = 0x04,
    CONFIG_EN_CRC = 0x08,
    STATUS_MAX_RT = 0x10,
    STATUS_TX_DS = 0x20,
    STATUS_RX_DR = 0x40,
    FIFO_RX_EMPTY = 0x01,
};

static uint32_t nowMs() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

Nrf24::~Nrf24() {
    end();
}

uint8_t Nrf24::xfer(uint8_t command, const uint8_t* tx, uint8_t* rx, size_t n) {
    cfg.spi->beginTransaction(settings);
    digitalWrite(cfg.csn, LOW);
    uint8_t st = cfg.spi->transfer(command);
    for (size_t i = 0; i < n; i++) {
        uint8_t r = cfg.spi->transfer(tx ? tx[i] : 0xFF);
        if (rx) rx[i] = r;
    }
    digitalWrite(cfg.csn, HIGH);
    cfg.spi->endTransaction();
    return st;
}

uint8_t Nrf24::readRegister(uint8_t reg) {
    uint8_t v = 0;
    xfer(CMD_R_REGISTER | (reg & 0x1F), nullptr, &v, 1);
    return v;
}

void Nrf24::writeRegister(uint8_t reg, uint8_t value) {
    xfer(CMD_W_REGISTER | (reg & 0x1F), &value, nullptr, 1);
}

void Nrf24::writeRegister(uint8_t reg, const uint8_t* data, size_t n) {
    xfer(CMD_W_REGISTER | (reg & 0x1F), data, nullptr, n);
}

uint8_t Nrf24::status() {
    return xfer(CMD_NOP, nullptr, nullptr, 0);
}

bool Nrf24::begin(const Config& config) {
    end();
    cfg = config;
    if (!cfg.spi) return false;
    if (cfg.addressWidth < 3 || cfg.addressWidth > 5) cfg.addressWidth = 5;
    if (cfg.payloadSize > MAX_PAYLOAD) cfg.payloadSize = MAX_PAYLOAD;
    if (cfg.channel >= CHANNELS) cfg.channel = CHANNELS - 1;
    if (cfg.autoAck && cfg.crcBytes == 0) cfg.crcBytes = 1;
    settings = SPISettings(cfg.spiFrequency, MSBFIRST, SPI_MODE0);

    pinMode(cfg.csn, OUTPUT);
    digitalWrite(cfg.csn, HIGH);
    pinMode(cfg.ce, OUTPUT);
    digitalWrite(cfg.ce, LOW);
    if (cfg.irq >= 0) pinMode(cfg.irq, INPUT);
    vTaskDelay(pdMS_TO_TICKS(5));

    // RF_SETUP is writable only on a real chip; 250 kbps (RF_DR_LOW) exists only on the L01+.
    uint8_t rfSetup = (cfg.paLevel & 0x03) << 1;
    if (cfg.dataRate == DataRate::Rate250K) rfSetup |= 0x20;
    if (cfg.dataRate == DataRate::Rate2M) rfSetup |= 0x08;
    writeRegister(REG_RF_SETUP, rfSetup);
    if ((readRegister(REG_RF_SETUP) & 0x2E) != rfSetup) return false;

    writeRegister(REG_CONFIG, 0x00); // powered down while configuring
    uint8_t ard = (uint8_t)((cfg.retryDelayUs / 250) ? (cfg.retryDelayUs / 250 - 1) : 0);
    if (ard > 15) ard = 15;
    writeRegister(REG_SETUP_RETR, (uint8_t)((ard << 4) | (cfg.retries & 0x0F)));
    writeRegister(REG_SETUP_AW, cfg.addressWidth - 2);
    writeRegister(REG_RF_CH, cfg.channel);
    writeRegister(REG_EN_AA, cfg.autoAck ? 0x3F : 0x00);
    writeRegister(REG_EN_RXADDR, 0x00);
    bool dynamic = cfg.payloadSize == 0;
    writeRegister(REG_FEATURE, dynamic ? 0x04 : 0x00);
    writeRegister(REG_DYNPD, dynamic ? 0x3F : 0x00);
    for (uint8_t p = 0; p < 6; p++)
        writeRegister(REG_RX_PW_P0 + p, dynamic ? 0 : cfg.payloadSize);
    xfer(CMD_FLUSH_TX, nullptr, nullptr, 0);
    xfer(CMD_FLUSH_RX, nullptr, nullptr, 0);
    writeRegister(REG_STATUS, STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT);
    powerUp(false);
    running = true;
    listening = false;
    pipe0Reading = false;
    return true;
}

void Nrf24::end() {
    if (!running) return;
    digitalWrite(cfg.ce, LOW);
    writeRegister(REG_CONFIG, 0x00); // power down
    running = false;
    listening = false;
}

void Nrf24::powerUp(bool rx) {
    uint8_t c = CONFIG_PWR_UP;
    if (cfg.crcBytes) c |= CONFIG_EN_CRC;
    if (cfg.crcBytes == 2) c |= CONFIG_CRCO;
    if (rx) c |= CONFIG_PRIM_RX;
    uint8_t before = readRegister(REG_CONFIG);
    writeRegister(REG_CONFIG, c);
    if (!(before & CONFIG_PWR_UP)) vTaskDelay(pdMS_TO_TICKS(5)); // Tpd2stby
}

void Nrf24::setChannel(uint8_t ch) {
    if (ch >= CHANNELS) ch = CHANNELS - 1;
    cfg.channel = ch;
    writeRegister(REG_RF_CH, ch);
}

void Nrf24::openWritingPipe(const uint8_t* address) {
    memcpy(txAddress, address, cfg.addressWidth);
    writeRegister(REG_TX_ADDR, address, cfg.addressWidth);
    // Auto-ack arrives on pipe 0, so it must listen on the TX address while transmitting.
    writeRegister(REG_RX_ADDR_P0, address, cfg.addressWidth);
}

bool Nrf24::openReadingPipe(uint8_t pipe, const uint8_t* address) {
    if (pipe > 5) return false;
    if (pipe == 0) {
        memcpy(pipe0Address, address, cfg.addressWidth);
        pipe0Reading = true;
    }
    if (pipe < 2) {
        writeRegister(REG_RX_ADDR_P0 + pipe, address, cfg.addressWidth);
    } else {
        writeRegister(REG_RX_ADDR_P0 + pipe, address[0]);
    }
    writeRegister(REG_EN_RXADDR, readRegister(REG_EN_RXADDR) | (1 << pipe));
    return true;
}

void Nrf24::closeReadingPipe(uint8_t pipe) {
    if (pipe > 5) return;
    if (pipe == 0) pipe0Reading = false;
    writeRegister(REG_EN_RXADDR, readRegister(REG_EN_RXADDR) & ~(1 << pipe));
}

void Nrf24::startListening() {
    if (pipe0Reading) writeRegister(REG_RX_ADDR_P0, pipe0Address, cfg.addressWidth);
    writeRegister(REG_STATUS, STATUS_RX_DR | STATUS_TX_DS | STATUS_MAX_RT);
    powerUp(true);
    digitalWrite(cfg.ce, HIGH);
    delayMicroseconds(130);
    listening = true;
}

void Nrf24::stopListening() {
    digitalWrite(cfg.ce, LOW);
    delayMicroseconds(100);
    powerUp(false);
    // Restore pipe 0 for auto-ack of our own transmissions.
    writeRegister(REG_RX_ADDR_P0, txAddress, cfg.addressWidth);
    listening = false;
}

bool Nrf24::send(const uint8_t* data, uint8_t length, uint32_t timeoutMs) {
    if (!running) return false;
    bool wasListening = listening;
    if (wasListening) stopListening();
    if (length > MAX_PAYLOAD) length = MAX_PAYLOAD;
    uint8_t buf[MAX_PAYLOAD] = {};
    memcpy(buf, data, length);
    uint8_t size = cfg.payloadSize ? cfg.payloadSize : length;

    xfer(CMD_FLUSH_TX, nullptr, nullptr, 0);
    writeRegister(REG_STATUS, STATUS_TX_DS | STATUS_MAX_RT);
    xfer(CMD_W_TX_PAYLOAD, buf, nullptr, size);
    digitalWrite(cfg.ce, HIGH);
    delayMicroseconds(15);
    digitalWrite(cfg.ce, LOW);

    uint8_t st = 0;
    uint32_t t0 = nowMs();
    while (nowMs() - t0 < timeoutMs) {
        st = status();
        if (st & (STATUS_TX_DS | STATUS_MAX_RT)) break;
        delayMicroseconds(100);
    }
    writeRegister(REG_STATUS, STATUS_TX_DS | STATUS_MAX_RT);
    if (!(st & STATUS_TX_DS)) xfer(CMD_FLUSH_TX, nullptr, nullptr, 0);
    if (wasListening) startListening();
    return st & STATUS_TX_DS;
}

bool Nrf24::available(uint8_t* pipe) {
    if (!running) return false;
    if (readRegister(REG_FIFO_STATUS) & FIFO_RX_EMPTY) return false;
    if (pipe) *pipe = (status() >> 1) & 0x07;
    return true;
}

uint8_t Nrf24::read(uint8_t* buffer, uint8_t max) {
    if (!available()) return 0;
    uint8_t width = cfg.payloadSize;
    if (!width) {
        xfer(CMD_R_RX_PL_WID, nullptr, &width, 1);
        if (width > MAX_PAYLOAD) {
            xfer(CMD_FLUSH_RX, nullptr, nullptr, 0); // corrupted
            return 0;
        }
    }
    uint8_t tmp[MAX_PAYLOAD];
    xfer(CMD_R_RX_PAYLOAD, nullptr, tmp, width);
    writeRegister(REG_STATUS, STATUS_RX_DR);
    uint8_t n = width < max ? width : max;
    memcpy(buffer, tmp, n);
    return n;
}

bool Nrf24::carrier() {
    return readRegister(REG_RPD) & 0x01;
}

bool Nrf24::probeChannel(uint8_t ch, uint32_t dwellUs) {
    digitalWrite(cfg.ce, LOW);
    setChannel(ch);
    powerUp(true);
    digitalWrite(cfg.ce, HIGH);
    delayMicroseconds(dwellUs);
    bool hit = carrier();
    if (listening) {
        startListening(); // keeps listening, now on the probed channel
    } else {
        digitalWrite(cfg.ce, LOW);
        powerUp(false);
    }
    return hit;
}

} // namespace mod
} // namespace lilka
