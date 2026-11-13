#include "nmea.h"

#include <stdlib.h>
#include <string.h>

namespace lilka {
namespace mod {

static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// "ddmm.mmmm" + hemisphere -> signed degrees
static double parseCoord(const char* v, const char* hemi) {
    if (!*v) return 0;
    double raw = strtod(v, nullptr);
    int deg = (int)(raw / 100);
    double result = deg + (raw - deg * 100) / 60.0;
    if (*hemi == 'S' || *hemi == 'W') result = -result;
    return result;
}

static int twoDigits(const char* s) {
    return (s[0] - '0') * 10 + (s[1] - '0');
}

Nmea::Nmea() : buf{}, len(0), inSentence(false), last{}, badChecksums(0), fixData{}, sats{}, satCount(0) {
}

void Nmea::reset() {
    len = 0;
    inSentence = false;
    last[0] = 0;
    badChecksums = 0;
    fixData = {};
    satCount = 0;
}

bool Nmea::feed(char c) {
    if (c == '$') {
        inSentence = true;
        len = 0;
        return false;
    }
    if (!inSentence) return false;
    if (c == '\r' || c == '\n') {
        inSentence = false;
        return finish();
    }
    if (len >= MAX_SENTENCE) {
        inSentence = false; // too long, drop
        return false;
    }
    buf[len++] = c;
    return false;
}

bool Nmea::finish() {
    buf[len] = 0;
    char* star = strchr(buf, '*');
    if (!star || star[1] == 0 || star[2] == 0) return false;
    uint8_t sum = 0;
    for (const char* p = buf; p < star; p++)
        sum ^= (uint8_t)*p;
    int hi = hexValue(star[1]), lo = hexValue(star[2]);
    if (hi < 0 || lo < 0 || sum != ((hi << 4) | lo)) {
        badChecksums++;
        return false;
    }
    memcpy(last, buf, len + 1);
    *star = 0;

    // Split in place; empty fields stay as "".
    char* fields[24];
    int n = 0;
    char* p = buf;
    fields[n++] = p;
    while (*p && n < 24) {
        if (*p == ',') {
            *p = 0;
            fields[n++] = p + 1;
        }
        p++;
    }
    parse(fields, n);
    return true;
}

void Nmea::parse(char** f, int n) {
    const char* id = f[0];
    if (strlen(id) != 5) return; // "GPGGA", "GNRMC", ...
    const char* type = id + 2;
    if (!strcmp(type, "GGA")) {
        parseGga(f, n);
    } else if (!strcmp(type, "RMC")) {
        parseRmc(f, n);
    } else if (!strcmp(type, "GSA")) {
        parseGsa(f, n);
    } else if (!strcmp(type, "GSV")) {
        parseGsv(id[1], f, n);
    }
}

void Nmea::parseTime(const char* s) {
    if (strlen(s) < 6) return;
    fixData.hour = twoDigits(s);
    fixData.minute = twoDigits(s + 2);
    fixData.second = twoDigits(s + 4);
    fixData.millis = s[6] == '.' ? (uint16_t)(strtof(s + 6, nullptr) * 1000) : 0;
}

// $GPGGA,time,lat,N,lon,E,quality,sats,hdop,alt,M,geoid,M,age,station
void Nmea::parseGga(char** f, int n) {
    if (n < 10) return;
    parseTime(f[1]);
    fixData.quality = atoi(f[6]);
    fixData.satellites = atoi(f[7]);
    if (*f[8]) fixData.hdop = strtof(f[8], nullptr);
    if (fixData.quality) {
        fixData.latitude = parseCoord(f[2], f[3]);
        fixData.longitude = parseCoord(f[4], f[5]);
        fixData.altitude = strtof(f[9], nullptr);
    }
    fixData.updates++;
}

// $GPRMC,time,status,lat,N,lon,E,speed_kn,course,date,magvar,E,mode
void Nmea::parseRmc(char** f, int n) {
    if (n < 10) return;
    parseTime(f[1]);
    fixData.valid = f[2][0] == 'A';
    if (fixData.valid) {
        fixData.latitude = parseCoord(f[3], f[4]);
        fixData.longitude = parseCoord(f[5], f[6]);
        fixData.speedKmh = strtof(f[7], nullptr) * 1.852f;
        if (*f[8]) fixData.course = strtof(f[8], nullptr);
    }
    if (strlen(f[9]) == 6) {
        fixData.day = twoDigits(f[9]);
        fixData.month = twoDigits(f[9] + 2);
        fixData.year = 2000 + twoDigits(f[9] + 4);
    }
    fixData.updates++;
}

// $GPGSA,mode,fix,sv1..sv12,pdop,hdop,vdop
void Nmea::parseGsa(char** f, int n) {
    if (n < 18) return;
    fixData.fixType = atoi(f[2]);
    if (*f[15]) fixData.pdop = strtof(f[15], nullptr);
    if (*f[16]) fixData.hdop = strtof(f[16], nullptr);
    if (*f[17]) fixData.vdop = strtof(f[17], nullptr);
}

// $GPGSV,total,num,inview,{prn,elev,az,snr}x4
void Nmea::parseGsv(char talker, char** f, int n) {
    if (n < 4) return;
    int msgNum = atoi(f[2]);
    if (msgNum == 1) {
        // New cycle for this constellation: drop its old entries.
        size_t w = 0;
        for (size_t r = 0; r < satCount; r++) {
            if (sats[r].system != talker) sats[w++] = sats[r];
        }
        satCount = w;
    }
    for (int i = 4; i < n; i += 4) {
        if (!*f[i] || satCount >= MAX_SATELLITES) continue;
        GpsSatellite& s = sats[satCount++];
        s.system = talker;
        s.prn = atoi(f[i]);
        s.elevation = i + 1 < n ? atoi(f[i + 1]) : 0;
        s.azimuth = i + 2 < n ? atoi(f[i + 2]) : 0;
        s.snr = (i + 3 < n && *f[i + 3]) ? atoi(f[i + 3]) : -1;
    }
}

} // namespace mod
} // namespace lilka
