#include "tmag5273.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace lilka {
namespace mod {

enum : uint8_t {
    REG_DEVICE_CONFIG_1 = 0x00,
    REG_DEVICE_CONFIG_2 = 0x01,
    REG_SENSOR_CONFIG_1 = 0x02,
    REG_SENSOR_CONFIG_2 = 0x03,
    REG_T_CONFIG = 0x07,
    REG_DEVICE_ID = 0x0D,
    REG_MANUFACTURER_ID_LSB = 0x0E,
    REG_T_MSB = 0x10,
    REG_X_MSB = 0x12,
};

bool Tmag5273::writeRegister(uint8_t reg, uint8_t value) {
    cfg.wire->beginTransmission(cfg.address);
    cfg.wire->write(reg);
    cfg.wire->write(value);
    return cfg.wire->endTransmission() == 0;
}

bool Tmag5273::readRegister(uint8_t reg, uint8_t* data, size_t n) {
    cfg.wire->beginTransmission(cfg.address);
    cfg.wire->write(reg);
    if (cfg.wire->endTransmission(false) != 0) return false;
    if (cfg.wire->requestFrom(cfg.address, (uint8_t)n) != n) return false;
    for (size_t i = 0; i < n; i++)
        data[i] = cfg.wire->read();
    return true;
}

bool Tmag5273::begin(const Config& config) {
    cfg = config;
    uint8_t man[2] = {};
    if (!readRegister(REG_MANUFACTURER_ID_LSB, man, 2)) return false;
    if (man[0] != 0x49 || man[1] != 0x54) return false; // "TI"
    uint8_t dev = 0;
    readRegister(REG_DEVICE_ID, &dev, 1);
    bool a2 = (dev & 0x03) == 2;
    rangeMt = a2 ? (cfg.highRange ? 266.0f : 133.0f) : (cfg.highRange ? 80.0f : 40.0f);

    uint8_t avg = cfg.averaging > 5 ? 5 : cfg.averaging;
    writeRegister(REG_DEVICE_CONFIG_1, (uint8_t)(avg << 2));
    writeRegister(REG_SENSOR_CONFIG_1, 0x70); // X, Y, Z
    writeRegister(REG_SENSOR_CONFIG_2, cfg.highRange ? 0x03 : 0x00); // XY and Z range
    writeRegister(REG_T_CONFIG, 0x01); // temperature on
    writeRegister(REG_DEVICE_CONFIG_2, 0x02); // continuous measurement
    vTaskDelay(pdMS_TO_TICKS(20));
    return true;
}

bool Tmag5273::read(Data& d) {
    uint8_t b[6];
    if (!readRegister(REG_X_MSB, b, 6)) return false;
    auto s16 = [&](int i) { return (int16_t)((b[i] << 8) | b[i + 1]); };
    float k = rangeMt * 1000.0f / 32768.0f;
    d.x = s16(0) * k;
    d.y = s16(2) * k;
    d.z = s16(4) * k;
    return true;
}

float Tmag5273::temperature() {
    uint8_t b[2] = {};
    readRegister(REG_T_MSB, b, 2);
    int16_t t = (int16_t)((b[0] << 8) | b[1]);
    return 25.0f + (t - 17508) / 60.1f;
}

} // namespace mod
} // namespace lilka
