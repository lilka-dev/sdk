#pragma once

#include <SPI.h>
#include <stdint.h>

#include "pins.h"

namespace lilka {
namespace mod {

/// LoRa-радіо Semtech SX1262 / SX1268 (модуль 003 LORA, Ebyte E22-900M / E22-400M).
///
/// DIO1 на модулі не під'єднаний, тому стан переривань опитується по SPI.
/// Перемикач антени (RXEN/TXEN) керується з Лілки. TCXO живиться з DIO3 (1.8 В).
/// Шина SPI має бути вже запущена (``spi.begin(sck, miso, mosi)``) - драйвер її не запускає і не зупиняє.
class SX126x {
public:
    enum class Chip {
        SX1262, ///< 850-930 МГц (E22-900M)
        SX1268, ///< 410-493 МГц (E22-400M)
    };

    enum class Result {
        OK,
        NoChip, ///< немає відповіді по SPI
        BusyTimeout, ///< BUSY не відпускається
        DeviceError, ///< помилка калібрування / TCXO (див. deviceErrors())
        InvalidParam, ///< частота, SF, BW, CR або потужність поза межами
        Timeout,
    };

    struct Config {
        SPIClass* spi = nullptr;
        int nss = pins::SPI_CS;
        int busy = pins::LORA_BUSY;
        int nrst = pins::LORA_NRST;
        int rxen = pins::LORA_RXEN;
        int txen = pins::LORA_TXEN;
        Chip chip = Chip::SX1262;
        uint32_t frequencyHz = 868100000;
        uint8_t spreadingFactor = 7; ///< 5-12
        uint32_t bandwidthHz = 125000; ///< 7800 ... 500000
        uint8_t codingRate = 5; ///< 5-8 (4/5 ... 4/8)
        int8_t powerDbm = 0; ///< -9 ... 22
        uint8_t syncWord = 0x12; ///< 0x12 - приватна мережа, 0x34 - LoRaWAN
        uint16_t preambleLength = 8;
        bool crc = true;
        bool invertIq = false;
        uint32_t spiFrequency = 2000000;
    };

    struct Packet {
        uint8_t data[255];
        uint8_t length;
        float rssi; ///< дБм
        float snr; ///< дБ
        bool crcError;
    };

    ~SX126x();
    Result begin(const Config& config);
    void end();

    /// Надіслати пакет і дочекатись TxDone. Після передачі повертається в прийом, якщо він був увімкнений.
    Result send(const uint8_t* data, uint8_t length, uint32_t timeoutMs = 3000, uint32_t* airTimeMs = nullptr);
    /// Перейти в безперервний прийом.
    Result startReceive();
    /// Перевірити, чи прийшов пакет (без очікування). Повертає true і заповнює packet.
    bool receive(Packet& packet);

    /// Миттєвий RSSI каналу, дБм (у режимі прийому).
    float rssi();
    void standby();
    void sleep();

    Result setFrequency(uint32_t hz);
    Result setTxPower(int8_t dbm);

    uint16_t deviceErrors();
    bool isReceiving() const {
        return receiving;
    }
    /// Оцінка часу в ефірі для пакета заданої довжини, мс.
    uint32_t timeOnAirMs(uint8_t length) const;

private:
    bool waitBusy(uint32_t timeoutMs = 100);
    bool xfer(const uint8_t* tx, uint8_t* rx, size_t n);
    bool command(const uint8_t* bytes, size_t n);
    uint8_t readRegister(uint16_t addr);
    void writeRegister(uint16_t addr, uint8_t value);
    uint8_t status();
    uint16_t irqStatus();
    void clearIrq();
    void writeBuffer(uint8_t offset, const uint8_t* data, size_t n);
    void readBuffer(uint8_t offset, uint8_t* data, size_t n);
    void setRfSwitch(bool tx);
    void setPacketParams(uint8_t length);
    bool reset();
    Result configure();

    Config cfg;
    bool running = false;
    bool receiving = false;
    bool asleep = false;
    SPISettings settings;
};

} // namespace mod
} // namespace lilka
