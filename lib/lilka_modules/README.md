# LilkaModules

Drivers for the Lilka v2 extension modules ([lilka-dev/lilka_modules](https://github.com/lilka-dev)):

| Module | Class | Bus |
|---|---|---|
| 002 IR (KY-005 + KY-022) | `lilka::mod::Ir`, `IrCodec` (NEC, Samsung, Sony SIRC) | RMT |
| 003 LORA (SX1262 / SX1268) | `lilka::mod::SX126x` | SPI |
| 004 RF (nRF24L01+) | `lilka::mod::Nrf24` | SPI |
| 004 RF (CC1101) | `lilka::mod::Cc1101` | SPI |
| 005 GPS (NEO-M8N) | `lilka::mod::Gps`, `Nmea` | UART |
| 006 GYRO MAGNET | `lilka::mod::Bmi260`, `lilka::mod::Tmag5273` | I2C |
| 008 CAN (SN65HVD232) | `lilka::mod::Can` | TWAI |

Rules:

- Plain classes, not singletons: create an instance when the module is plugged in, call `begin()`.
  Nothing is started by `lilka::begin()`.
- Default pins come from `lilka_modules/pins.h`; every pin is a `Config` field.
- SPI and I2C drivers take the bus (`SPIClass*`, `TwoWire*`) and never start or stop it: call
  `lilka::SPI2.begin(pins::SPI_SCK, pins::SPI_MISO, pins::SPI_MOSI)` / `Wire.begin(pins::I2C_SDA, pins::I2C_SCL)` first.
- `lilka::mod::resources` tracks which pins/peripherals are in use by whom (used by Keira to keep an app
  and a service off each other's hardware). Drivers don't use it themselves.
- `Nmea` and `IrCodec` have no hardware access and build on a PC.

`bmi260_config.h` is Bosch's BMI260 configuration blob (BSD-3-Clause, see `BMI260_CONFIG_LICENSE`).

Hardware status: SX126x, nRF24 and CC1101 drivers are ported from code tested on the boards; the GPS, IR
and CAN boards haven't been tested on hardware yet.
