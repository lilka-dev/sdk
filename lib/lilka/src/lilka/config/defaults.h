#pragma once

#include "lilka/config.h"

// Built-in hardware configuration, loaded before the files in /sd/.lilka/config.
// Values come from lilka/config.h, so they always match the board the SDK is built for.

#define LILKA_CONFIG_STR_(x) #x
#define LILKA_CONFIG_STR(x)  LILKA_CONFIG_STR_(x)

#if LILKA_VERSION == 2

// Extension header: pins 3-10 (J2 on the module schematics). Sections below are per module board.
// clang-format off
#    define LILKA_CONFIG_DEFAULTS                                                                                 \
        "[ext]\n"                                                                                                 \
        "rx = " LILKA_CONFIG_STR(LILKA_EXT_RX) "\n"                                                               \
        "tx = " LILKA_CONFIG_STR(LILKA_EXT_TX) "\n"                                                               \
        "p3 = " LILKA_CONFIG_STR(LILKA_P3) "\n"                                                                   \
        "p4 = " LILKA_CONFIG_STR(LILKA_P4) "\n"                                                                   \
        "p5 = " LILKA_CONFIG_STR(LILKA_P5) "\n"                                                                   \
        "p6 = " LILKA_CONFIG_STR(LILKA_P6) "\n"                                                                   \
        "p7 = " LILKA_CONFIG_STR(LILKA_P7) "\n"                                                                   \
        "p8 = " LILKA_CONFIG_STR(LILKA_P8) "\n"                                                                   \
        "[gps]\n"                                                                                                 \
        "rx = " LILKA_CONFIG_STR(LILKA_EXT_RX) "\n"                                                               \
        "tx = " LILKA_CONFIG_STR(LILKA_EXT_TX) "\n"                                                               \
        "baud = 9600\n"                                                                                           \
        "[can]\n"                                                                                                 \
        "rx = " LILKA_CONFIG_STR(LILKA_EXT_RX) "\n"                                                               \
        "tx = " LILKA_CONFIG_STR(LILKA_EXT_TX) "\n"                                                               \
        "[ir]\n"                                                                                                  \
        "rx = " LILKA_CONFIG_STR(LILKA_EXT_RX) "\n"                                                               \
        "tx = " LILKA_CONFIG_STR(LILKA_EXT_TX) "\n"                                                               \
        "carrier = 38000\n"                                                                                       \
        "[lora]\n"                                                                                                \
        "sck = " LILKA_CONFIG_STR(LILKA_EXT_RX) "\n"                                                              \
        "miso = " LILKA_CONFIG_STR(LILKA_EXT_TX) "\n"                                                             \
        "mosi = " LILKA_CONFIG_STR(LILKA_P3) "\n"                                                                 \
        "nss = " LILKA_CONFIG_STR(LILKA_P4) "\n"                                                                  \
        "rxen = " LILKA_CONFIG_STR(LILKA_P5) "\n"                                                                 \
        "txen = " LILKA_CONFIG_STR(LILKA_P6) "\n"                                                                 \
        "nrst = " LILKA_CONFIG_STR(LILKA_P7) "\n"                                                                 \
        "busy = " LILKA_CONFIG_STR(LILKA_P8) "\n"                                                                 \
        "[rf]\n"                                                                                                  \
        "sck = " LILKA_CONFIG_STR(LILKA_EXT_RX) "\n"                                                              \
        "miso = " LILKA_CONFIG_STR(LILKA_EXT_TX) "\n"                                                             \
        "mosi = " LILKA_CONFIG_STR(LILKA_P3) "\n"                                                                 \
        "cs = " LILKA_CONFIG_STR(LILKA_P4) "\n"                                                                   \
        "ce = " LILKA_CONFIG_STR(LILKA_P5) "\n"                                                                   \
        "irq = " LILKA_CONFIG_STR(LILKA_P6) "\n"                                                                  \
        "[gyro_magnet]\n"                                                                                         \
        "sda = " LILKA_CONFIG_STR(LILKA_EXT_RX) "\n"                                                              \
        "scl = " LILKA_CONFIG_STR(LILKA_EXT_TX) "\n"                                                              \
        "mag_int = " LILKA_CONFIG_STR(LILKA_P3) "\n"                                                              \
        "gyr_int1 = " LILKA_CONFIG_STR(LILKA_P4) "\n"                                                             \
        "gyr_int2 = " LILKA_CONFIG_STR(LILKA_P5) "\n"
// clang-format on

#else
#    define LILKA_CONFIG_DEFAULTS ""
#endif
