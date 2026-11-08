#pragma once

#include <stdint.h>
#include <stddef.h>

namespace lilka {
namespace mod {

/// Дані про розташування, зібрані з NMEA-речень (GGA, RMC, GSA, GSV).
struct GpsFix {
    bool valid; ///< RMC status = A (є фіксація)
    uint8_t quality; ///< GGA: 0 - немає, 1 - GPS, 2 - DGPS, 4/5 - RTK, 6 - оцінка
    uint8_t fixType; ///< GSA: 1 - немає, 2 - 2D, 3 - 3D
    double latitude; ///< градуси, південь < 0
    double longitude; ///< градуси, захід < 0
    float altitude; ///< метри над рівнем моря
    float speedKmh; ///< км/год
    float course; ///< градуси від півночі
    float hdop, vdop, pdop;
    uint8_t satellites; ///< супутники, що використовуються (GGA)
    uint16_t year; ///< UTC
    uint8_t month, day, hour, minute, second;
    uint16_t millis;
    uint32_t updates; ///< кількість розібраних GGA/RMC з моменту старту
};

/// Супутник у полі зору (GSV).
struct GpsSatellite {
    char system; ///< 'P' - GPS, 'L' - ГЛОНАСС, 'A' - Galileo, 'B' - BeiDou, 'N' - інші
    uint8_t prn;
    uint8_t elevation; ///< градуси
    uint16_t azimuth; ///< градуси
    int8_t snr; ///< дБГц, -1 якщо не відстежується
};

/// Розбирач NMEA 0183 без доступу до заліза. Байти подаються по одному через feed().
class Nmea {
public:
    static constexpr size_t MAX_SENTENCE = 96;
    static constexpr size_t MAX_SATELLITES = 48;

    Nmea();
    void reset();

    /// Подати один байт. Повертає true, коли розібрано повне речення з правильною контрольною сумою.
    bool feed(char c);

    const GpsFix& fix() const {
        return fixData;
    }
    size_t satelliteCount() const {
        return satCount;
    }
    const GpsSatellite& satellite(size_t i) const {
        return sats[i];
    }
    /// Останнє коректне речення (без CR/LF).
    const char* lastSentence() const {
        return last;
    }
    uint32_t checksumErrors() const {
        return badChecksums;
    }

private:
    bool finish();
    void parse(char** f, int n);
    void parseGga(char** f, int n);
    void parseRmc(char** f, int n);
    void parseGsa(char** f, int n);
    void parseGsv(char talker, char** f, int n);
    void parseTime(const char* s);

    char buf[MAX_SENTENCE + 1];
    size_t len;
    bool inSentence;
    char last[MAX_SENTENCE + 1];
    uint32_t badChecksums;

    GpsFix fixData;
    GpsSatellite sats[MAX_SATELLITES];
    size_t satCount;
};

} // namespace mod
} // namespace lilka
