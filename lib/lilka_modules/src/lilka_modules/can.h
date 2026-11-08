#pragma once

#include <driver/twai.h>
#include <esp_err.h>

#include "pins.h"

namespace lilka {
namespace mod {

/// Шина CAN на вбудованому контролері TWAI (модуль 008 CAN, трансивер SN65HVD232).
class Can {
public:
    enum class Mode {
        Normal,
        ListenOnly, ///< лише прийом, без ACK - безпечно для чужої шини
        NoAck, ///< передача без очікування ACK (самотест)
    };

    struct Config {
        int tx = pins::UART_TX;
        int rx = pins::UART_RX;
        uint32_t bitrate = 500000; ///< 25k, 50k, 100k, 125k, 250k, 500k, 800k або 1M
        Mode mode = Mode::Normal;
        uint32_t filterCode = 0;
        uint32_t filterMask = 0xFFFFFFFF; ///< 1 = біт не перевіряється (за замовчуванням приймаємо все)
        bool singleFilter = true;
        uint32_t rxQueue = 32;
        uint32_t txQueue = 8;
    };

    ~Can();
    esp_err_t begin(const Config& config);
    void end();
    bool isRunning() const {
        return running;
    }

    esp_err_t send(const twai_message_t& msg, uint32_t timeoutMs);
    /// ESP_ERR_TIMEOUT, якщо за timeoutMs нічого не прийшло.
    esp_err_t receive(twai_message_t& msg, uint32_t timeoutMs);
    esp_err_t status(twai_status_info_t& info);
    /// Вийти з BUS_OFF або перезапустити зупинений контролер.
    esp_err_t recover();

    static bool timingFor(uint32_t bitrate, twai_timing_config_t& out);

private:
    bool running = false;
};

} // namespace mod
} // namespace lilka
