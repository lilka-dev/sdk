#pragma once

#include <lilka/config.h>

/// Типові контакти модулів розширення Лілки v2.
///
/// Нумерація - GPIO ESP32-S3. Контакти 3-10 роз'єму розширення (J2 на схемах модулів):
///
/// | Контакт | GPIO | Контакт | GPIO |
/// |---|---|---|---|
/// | 3 | 44 (U0RXD) | 7 | 21 |
/// | 4 | 43 (U0TXD) | 8 | 14 |
/// | 5 | 48         | 9 | 13 |
/// | 6 | 47         | 10 | 12 |

// Older SDK releases don't define the UART pins of the extension header yet.
#ifndef LILKA_EXT_RX
#    define LILKA_EXT_RX 44
#endif
#ifndef LILKA_EXT_TX
#    define LILKA_EXT_TX 43
#endif

namespace lilka {
namespace mod {
namespace pins {

/// Контакт роз'єму не під'єднано.
constexpr int NC = -1;

// UART-модулі (GPS, IR, CAN)
constexpr int UART_RX = LILKA_EXT_RX; ///< Вхід у Лілку (контакт 3)
constexpr int UART_TX = LILKA_EXT_TX; ///< Вихід з Лілки (контакт 4)

// SPI-модулі (LORA, RF)
constexpr int SPI_SCK = LILKA_EXT_RX; ///< Контакт 3
constexpr int SPI_MISO = LILKA_EXT_TX; ///< Контакт 4
constexpr int SPI_MOSI = LILKA_P3; ///< Контакт 5
constexpr int SPI_CS = LILKA_P4; ///< Контакт 6

// 003 LORA
constexpr int LORA_RXEN = LILKA_P5; ///< Контакт 7
constexpr int LORA_TXEN = LILKA_P6; ///< Контакт 8
constexpr int LORA_NRST = LILKA_P7; ///< Контакт 9
constexpr int LORA_BUSY = LILKA_P8; ///< Контакт 10

// 004 RF
constexpr int RF_CE = LILKA_P5; ///< nRF24 CE / CC1101 GDO0 (контакт 7)
constexpr int RF_IRQ = LILKA_P6; ///< nRF24 IRQ / CC1101 GDO2 (контакт 8)

// I2C-модулі (GYRO MAGNET)
constexpr int I2C_SDA = LILKA_EXT_RX; ///< Контакт 3
constexpr int I2C_SCL = LILKA_EXT_TX; ///< Контакт 4

// 006 GYRO MAGNET
constexpr int MAG_INT = LILKA_P3; ///< Контакт 5
constexpr int GYR_INT1 = LILKA_P4; ///< Контакт 6
constexpr int GYR_INT2 = LILKA_P5; ///< Контакт 7

} // namespace pins
} // namespace mod
} // namespace lilka
