#pragma once

#include <SPI.h>
#include <stdint.h>

#include "pins.h"

namespace lilka {
namespace mod {

/// Радіо 2.4 ГГц Nordic nRF24L01+ (модуль 004 RF).
///
/// Шина SPI має бути вже запущена - драйвер її не запускає і не зупиняє.
class Nrf24 {
public:
    static constexpr uint8_t CHANNELS = 126;
    static constexpr uint8_t MAX_PAYLOAD = 32;

    enum class DataRate : uint8_t {
        Rate250K,
        Rate1M,
        Rate2M,
    };

    struct Config {
        SPIClass* spi = nullptr;
        int csn = pins::SPI_CS;
        int ce = pins::RF_CE;
        int irq = pins::RF_IRQ; ///< лише читається; NC - не використовується
        uint8_t channel = 76; ///< 0-125, частота = 2400 + channel МГц
        DataRate dataRate = DataRate::Rate1M;
        uint8_t paLevel = 3; ///< 0 (-18 дБм) ... 3 (0 дБм)
        uint8_t addressWidth = 5; ///< 3-5 байт
        uint8_t payloadSize = 32; ///< 1-32, 0 - динамічна довжина
        bool autoAck = true;
        uint8_t retries = 15; ///< 0-15
        uint16_t retryDelayUs = 1500; ///< 250-4000, крок 250
        uint8_t crcBytes = 2; ///< 0, 1 або 2 (з autoAck CRC обов'язковий)
        uint32_t spiFrequency = 4000000;
    };

    ~Nrf24();
    /// false - чип не відповідає (або це не nRF24L01+).
    bool begin(const Config& config);
    void end();

    void setChannel(uint8_t ch);
    uint8_t channel() const {
        return cfg.channel;
    }

    /// Адреса передачі (і RX-канал 0 для автопідтвердження). Довжина - addressWidth.
    void openWritingPipe(const uint8_t* address);
    /// Канали 0-1 - повна адреса, 2-5 - лише молодший байт (старші як у каналу 1).
    bool openReadingPipe(uint8_t pipe, const uint8_t* address);
    void closeReadingPipe(uint8_t pipe);

    void startListening();
    void stopListening();
    bool isListening() const {
        return listening;
    }

    /// Надіслати пакет. З autoAck повертає true лише після підтвердження приймачем.
    bool send(const uint8_t* data, uint8_t length, uint32_t timeoutMs = 100);
    /// Чи є прийнятий пакет; pipe - номер каналу (може бути nullptr).
    bool available(uint8_t* pipe = nullptr);
    /// Прочитати пакет. Повертає довжину (0 - нічого немає).
    uint8_t read(uint8_t* buffer, uint8_t max);

    /// RPD: на поточному каналі є сигнал сильніший за -64 дБм (у режимі прийому).
    bool carrier();
    /// Послухати канал dwellUs мкс і повернути RPD. Режим прийому вмикається і вимикається тут.
    bool probeChannel(uint8_t ch, uint32_t dwellUs = 260);

    uint8_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint8_t value);

private:
    uint8_t xfer(uint8_t command, const uint8_t* tx, uint8_t* rx, size_t n);
    void writeRegister(uint8_t reg, const uint8_t* data, size_t n);
    uint8_t status();
    void powerUp(bool rx);

    Config cfg;
    SPISettings settings;
    bool running = false;
    bool listening = false;
    uint8_t txAddress[5] = {};
    uint8_t pipe0Address[5] = {};
    bool pipe0Reading = false;
};

} // namespace mod
} // namespace lilka
