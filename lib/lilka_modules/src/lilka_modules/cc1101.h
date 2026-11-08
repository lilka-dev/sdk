#pragma once

#include <SPI.h>
#include <stdint.h>

#include "pins.h"

namespace lilka {
namespace mod {

/// Субгігагерцове радіо TI CC1101 (модуль 004 RF з CC1101 у розкладці nRF24).
///
/// Діапазони: 300-348, 387-464, 779-928 МГц. Кварц 26 МГц.
/// Пакети змінної довжини до 61 байта. Шина SPI має бути вже запущена.
class Cc1101 {
public:
    static constexpr uint8_t MAX_PACKET = 61;

    enum class Modulation : uint8_t {
        FSK2 = 0,
        GFSK = 1,
        ASK = 3, ///< ASK/OOK
        FSK4 = 4,
        MSK = 7,
    };

    struct Config {
        SPIClass* spi = nullptr;
        int csn = pins::SPI_CS;
        int miso = pins::SPI_MISO; ///< для очікування CHIP_RDYn
        int gdo0 = pins::RF_CE;
        int gdo2 = pins::RF_IRQ;
        float frequencyMhz = 433.92f;
        Modulation modulation = Modulation::GFSK;
        uint32_t dataRate = 38400; ///< бод, 600 ... 500000
        uint32_t deviationHz = 20000; ///< для FSK
        uint32_t bandwidthHz = 100000; ///< 58 кГц ... 812 кГц
        int8_t powerDbm = 0; ///< -30 ... 10
        uint16_t syncWord = 0xD391;
        bool crc = true;
        uint32_t xtalHz = 26000000;
        uint32_t spiFrequency = 4000000;
    };

    struct Packet {
        uint8_t data[MAX_PACKET];
        uint8_t length;
        float rssi;
        uint8_t lqi;
        bool crcOk;
    };

    ~Cc1101();
    /// false - чип не відповідає або параметри поза межами.
    bool begin(const Config& config);
    void end();

    bool setFrequency(float mhz);
    void setPower(int8_t dbm);

    bool send(const uint8_t* data, uint8_t length, uint32_t timeoutMs = 500);
    void startReceive();
    bool receive(Packet& packet);
    void idle();
    bool isReceiving() const {
        return receiving;
    }

    /// RSSI, дБм (у режимі прийому).
    float rssi();

    uint8_t partNumber();
    uint8_t version();
    uint8_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint8_t value);

private:
    uint8_t xfer(uint8_t header, const uint8_t* tx, uint8_t* rx, size_t n);
    uint8_t strobe(uint8_t s);
    uint8_t readStatus(uint8_t reg);
    uint8_t marcState();
    bool waitMarc(uint8_t state, uint32_t ms);
    void reset();
    bool configure();

    Config cfg;
    SPISettings settings;
    bool running = false;
    bool receiving = false;
};

} // namespace mod
} // namespace lilka
