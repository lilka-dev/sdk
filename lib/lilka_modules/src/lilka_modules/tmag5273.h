#pragma once

#include <Wire.h>
#include <stdint.h>

namespace lilka {
namespace mod {

/// 3D-датчик Холла TI TMAG5273 (модуль 006 GYRO MAGNET, TMAG5273A1).
///
/// Шина I2C має бути вже запущена.
class Tmag5273 {
public:
    static constexpr uint8_t DEFAULT_ADDRESS = 0x35;

    struct Config {
        TwoWire* wire = &Wire;
        uint8_t address = DEFAULT_ADDRESS;
        bool highRange = false; ///< A1: ±40 / ±80 мТл, A2: ±133 / ±266 мТл
        uint8_t averaging = 5; ///< 0-5: 1x, 2x, 4x, 8x, 16x, 32x
    };

    struct Data {
        float x, y, z; ///< мкТл
    };

    bool begin(const Config& conf);
    bool read(Data& data);
    float temperature();
    /// Повний діапазон по осях, мТл.
    float rangeMilliTesla() const {
        return rangeMt;
    }

private:
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t* data, size_t n);

    Config cfg;
    float rangeMt = 40;
};

} // namespace mod
} // namespace lilka
