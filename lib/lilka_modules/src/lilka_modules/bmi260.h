#pragma once

#include <Wire.h>
#include <stdint.h>

namespace lilka {
namespace mod {

/// Акселерометр і гіроскоп Bosch BMI260 (модуль 006 GYRO MAGNET).
///
/// Шина I2C має бути вже запущена. Для повноцінної роботи чипу потрібен конфігураційний блоб (8 КБ):
/// його можна взяти з bmi260_config.h (bmi260DefaultConfig()) або завантажити з файлу.
class Bmi260 {
public:
    static constexpr uint8_t CHIP_ID = 0x27;

    struct Config {
        TwoWire* wire = &Wire;
        uint8_t address = 0; ///< 0 - знайти автоматично (0x68 або 0x69)
        const uint8_t* blob = nullptr; ///< nullptr - не завантажувати конфігурацію
        size_t blobLength = 0;
        uint8_t accelRangeG = 8; ///< 2, 4, 8, 16
        uint16_t gyroRangeDps = 2000; ///< 125, 250, 500, 1000, 2000
        uint16_t accelOdrHz = 100; ///< 12 (12.5), 25, 50, 100, 200, 400, 800, 1600
        uint16_t gyroOdrHz = 200; ///< 25 ... 3200
    };

    struct Data {
        float ax, ay, az; ///< g
        float gx, gy, gz; ///< °/с
    };

    enum class Result {
        OK,
        NoChip,
        ConfigFailed, ///< блоб не прийнятий (INTERNAL_STATUS != init_ok)
        InvalidParam,
    };

    Result begin(const Config& config);
    bool read(Data& data);
    float temperature();
    /// Блоб завантажено і чип повідомив init_ok.
    bool isInitialized() const {
        return initialized;
    }
    uint8_t address() const {
        return cfg.address;
    }
    uint8_t internalStatus();

private:
    bool writeRegister(uint8_t reg, const uint8_t* data, size_t n);
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t* data, size_t n);
    uint8_t readRegister(uint8_t reg);
    bool loadConfig();

    Config cfg;
    bool initialized = false;
    float accelScale = 1, gyroScale = 1;
};

/// Вбудований конфігураційний блоб Bosch (BSD-3-Clause, див. BMI260_CONFIG_LICENSE).
const uint8_t* bmi260DefaultConfig(size_t* length);

} // namespace mod
} // namespace lilka
