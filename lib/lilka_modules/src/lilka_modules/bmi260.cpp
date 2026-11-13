#include "bmi260.h"

#include <Arduino.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "bmi260_config.h"

namespace lilka {
namespace mod {

enum : uint8_t {
    REG_CHIP_ID = 0x00,
    REG_ACC_X = 0x0C,
    REG_INTERNAL_STATUS = 0x21,
    REG_TEMP = 0x22,
    REG_ACC_CONF = 0x40,
    REG_ACC_RANGE = 0x41,
    REG_GYR_CONF = 0x42,
    REG_GYR_RANGE = 0x43,
    REG_INIT_CTRL = 0x59,
    REG_INIT_ADDR_0 = 0x5B,
    REG_INIT_DATA = 0x5E,
    REG_PWR_CONF = 0x7C,
    REG_PWR_CTRL = 0x7D,
    REG_CMD = 0x7E,
};

const uint8_t* bmi260DefaultConfig(size_t* length) {
    if (length) *length = sizeof(g_bmi260_config_tbin);
    return g_bmi260_config_tbin;
}

static bool odrCode(uint16_t hz, uint8_t& code) {
    static const uint16_t rates[] = {12, 25, 50, 100, 200, 400, 800, 1600, 3200};
    for (uint8_t i = 0; i < sizeof(rates) / sizeof(rates[0]); i++) {
        if (rates[i] == hz) {
            code = 0x05 + i;
            return true;
        }
    }
    return false;
}

bool Bmi260::writeRegister(uint8_t reg, const uint8_t* data, size_t n) {
    cfg.wire->beginTransmission(cfg.address);
    cfg.wire->write(reg);
    cfg.wire->write(data, n);
    return cfg.wire->endTransmission() == 0;
}

bool Bmi260::writeRegister(uint8_t reg, uint8_t value) {
    return writeRegister(reg, &value, 1);
}

bool Bmi260::readRegister(uint8_t reg, uint8_t* data, size_t n) {
    cfg.wire->beginTransmission(cfg.address);
    cfg.wire->write(reg);
    if (cfg.wire->endTransmission(false) != 0) return false;
    if (cfg.wire->requestFrom(cfg.address, (uint8_t)n) != n) return false;
    for (size_t i = 0; i < n; i++)
        data[i] = cfg.wire->read();
    return true;
}

uint8_t Bmi260::readRegister(uint8_t reg) {
    uint8_t v = 0xFF;
    readRegister(reg, &v, 1);
    return v;
}

uint8_t Bmi260::internalStatus() {
    return readRegister(REG_INTERNAL_STATUS);
}

bool Bmi260::loadConfig() {
    writeRegister(REG_PWR_CONF, 0x00); // advanced power save off
    delayMicroseconds(450);
    writeRegister(REG_INIT_CTRL, 0x00);
    const size_t chunk = 64; // fits the Arduino Wire buffer
    for (size_t off = 0; off < cfg.blobLength; off += chunk) {
        size_t n = cfg.blobLength - off < chunk ? cfg.blobLength - off : chunk;
        const uint8_t a[2] = {(uint8_t)((off / 2) & 0x0F), (uint8_t)((off / 2) >> 4)};
        writeRegister(REG_INIT_ADDR_0, a, 2);
        if (!writeRegister(REG_INIT_DATA, cfg.blob + off, n)) return false;
    }
    writeRegister(REG_INIT_CTRL, 0x01);
    int64_t t0 = esp_timer_get_time();
    while (esp_timer_get_time() - t0 < 150000) {
        if ((internalStatus() & 0x0F) == 0x01) return true;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return false;
}

Bmi260::Result Bmi260::begin(const Config& conf) {
    cfg = conf;
    initialized = false;
    uint8_t accOdr, gyrOdr, accRange, gyrRange;
    switch (cfg.accelRangeG) {
        case 2:
            accRange = 0;
            break;
        case 4:
            accRange = 1;
            break;
        case 8:
            accRange = 2;
            break;
        case 16:
            accRange = 3;
            break;
        default:
            return Result::InvalidParam;
    }
    switch (cfg.gyroRangeDps) {
        case 2000:
            gyrRange = 0;
            break;
        case 1000:
            gyrRange = 1;
            break;
        case 500:
            gyrRange = 2;
            break;
        case 250:
            gyrRange = 3;
            break;
        case 125:
            gyrRange = 4;
            break;
        default:
            return Result::InvalidParam;
    }
    if (!odrCode(cfg.accelOdrHz, accOdr) || accOdr > 0x0C || !odrCode(cfg.gyroOdrHz, gyrOdr) || gyrOdr < 0x06) {
        return Result::InvalidParam;
    }

    if (cfg.address == 0) {
        for (uint8_t a : {0x68, 0x69}) {
            cfg.address = a;
            if (readRegister(REG_CHIP_ID) == CHIP_ID) break;
            cfg.address = 0;
        }
        if (!cfg.address) return Result::NoChip;
    } else if (readRegister(REG_CHIP_ID) != CHIP_ID) {
        return Result::NoChip;
    }

    writeRegister(REG_CMD, 0xB6); // soft reset
    vTaskDelay(pdMS_TO_TICKS(2));
    readRegister(REG_CHIP_ID); // dummy read after reset

    if (cfg.blob && cfg.blobLength) {
        if (!loadConfig()) return Result::ConfigFailed;
        initialized = true;
    } else {
        writeRegister(REG_PWR_CONF, 0x00);
        delayMicroseconds(450);
    }

    writeRegister(REG_PWR_CTRL, 0x0E); // acc + gyr + temp on
    writeRegister(REG_ACC_CONF, 0xA0 | accOdr);
    writeRegister(REG_ACC_RANGE, accRange);
    writeRegister(REG_GYR_CONF, 0xA0 | gyrOdr);
    writeRegister(REG_GYR_RANGE, gyrRange);
    accelScale = cfg.accelRangeG / 32768.0f;
    gyroScale = cfg.gyroRangeDps / 32768.0f;
    vTaskDelay(pdMS_TO_TICKS(50));
    return Result::OK;
}

bool Bmi260::read(Data& d) {
    uint8_t b[12];
    if (!readRegister(REG_ACC_X, b, 12)) return false;
    auto s16 = [&](int i) { return (int16_t)(b[i] | (b[i + 1] << 8)); };
    d.ax = s16(0) * accelScale;
    d.ay = s16(2) * accelScale;
    d.az = s16(4) * accelScale;
    d.gx = s16(6) * gyroScale;
    d.gy = s16(8) * gyroScale;
    d.gz = s16(10) * gyroScale;
    return true;
}

float Bmi260::temperature() {
    uint8_t b[2] = {};
    readRegister(REG_TEMP, b, 2);
    int16_t t = (int16_t)(b[0] | (b[1] << 8));
    return t / 512.0f + 23.0f;
}

} // namespace mod
} // namespace lilka
