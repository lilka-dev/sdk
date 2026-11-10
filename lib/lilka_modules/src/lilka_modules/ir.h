#pragma once

#include <sdkconfig.h>

// RMT channel layout below is for the ESP32-S3 (Lilka v2).
#if CONFIG_IDF_TARGET_ESP32S3

#    include <driver/rmt.h>
#    include <esp_err.h>
#    include <freertos/ringbuf.h>

#    include "ir_codec.h"
#    include "pins.h"

namespace lilka {
namespace mod {

/// ІЧ-передавач і приймач на периферії RMT (модуль 002 IR: KY-005 + KY-022).
///
/// Використовує канали RMT_CHANNEL_3 (передача) і RMT_CHANNEL_4 (прийом, займає пам'ять каналів 4-7),
/// щоб не заважати драйверам Arduino, які беруть канали з початку.
class Ir {
public:
    static constexpr size_t MAX_DURATIONS = 380;

    struct Config {
        int tx = pins::get("ir", "tx"); ///< NC - без передавача
        int rx = pins::get("ir", "rx"); ///< NC - без приймача
        uint32_t carrierHz = (uint32_t)lilka::config.getInt("ir", "carrier", 38000);
        uint8_t dutyPercent = 33;
        bool rxActiveLow = true; ///< KY-022 / VS1838B тримає 0, поки бачить несучу
        uint16_t rxIdleUs = 12000; ///< пауза, що завершує кадр
        rmt_channel_t txChannel = RMT_CHANNEL_3;
        rmt_channel_t rxChannel = RMT_CHANNEL_4;
    };

    ~Ir();
    esp_err_t begin(const Config& conf);
    void end();

    /// Надіслати сирі тривалості (мкс, починаючи з імпульсу). carrierHz = 0 - взяти з конфігурації.
    esp_err_t sendRaw(const uint16_t* durations, size_t count, uint32_t carrierHz = 0);
    /// Закодувати й надіслати кадр (repeats - скільки разів повторити, для Sony мінімум 3 загалом).
    esp_err_t send(const IrCode& code, uint8_t repeats = 0);

    /// Забрати один прийнятий кадр (без очікування). Повертає кількість тривалостей, 0 - нічого немає.
    size_t receiveRaw(uint16_t* out, size_t max);

private:
    Config cfg;
    bool txReady = false;
    bool rxReady = false;
    RingbufHandle_t rxBuffer = nullptr;
    uint32_t currentCarrier = 0;
    rmt_item32_t* txItems = nullptr;
};

} // namespace mod
} // namespace lilka

#endif // CONFIG_IDF_TARGET_ESP32S3
