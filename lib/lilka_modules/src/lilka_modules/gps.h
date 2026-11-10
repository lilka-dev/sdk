#pragma once

#include <driver/uart.h>
#include <esp_err.h>

#include "nmea.h"
#include "pins.h"

namespace lilka {
namespace mod {

/// GNSS-приймач з NMEA-виходом на UART (модуль 005 GPS, u-blox NEO-M8N).
class Gps {
public:
    struct Config {
        uart_port_t port = UART_NUM_2;
        int rx = pins::get("gps", "rx"); ///< Вхід у Лілку (TX модуля)
        int tx = pins::get("gps", "tx"); ///< Вихід з Лілки (RX модуля), NC - лише прийом
        uint32_t baud = (uint32_t)lilka::config.getInt("gps", "baud", 9600);
    };

    ~Gps();
    esp_err_t begin(const Config& conf);
    void end();
    bool isRunning() const {
        return running;
    }

    /// Прочитати все, що прийшло з UART, і розібрати. Повертає кількість нових речень.
    int update();

    const Nmea& nmea() const {
        return parser;
    }
    const GpsFix& fix() const {
        return parser.fix();
    }

    /// Надіслати сирі байти (наприклад, NMEA-команду PUBX або UBX).
    esp_err_t write(const uint8_t* data, size_t len);
    /// Надіслати UBX-повідомлення (контрольна сума рахується тут).
    esp_err_t sendUbx(uint8_t cls, uint8_t id, const uint8_t* payload, uint16_t len);

private:
    Config cfg;
    bool running = false;
    Nmea parser;
};

} // namespace mod
} // namespace lilka
