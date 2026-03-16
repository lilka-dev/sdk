# Lilka SDK

[![License: GPL-2.0](https://img.shields.io/badge/License-GPL%202.0-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html)
[![Platform: ESP32-S3](https://img.shields.io/badge/Platform-ESP32--S3-green.svg)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-teal.svg)](https://www.arduino.cc/)
[![PlatformIO](https://img.shields.io/badge/Build-PlatformIO-orange.svg)](https://platformio.org/)

The official SDK for **[Lilka](https://docs.lilka.dev/)** — a handheld device powered by ESP32-S3 with a 240×280 TFT display, 10 buttons, a piezo buzzer, I2S audio, microSD card slot, battery, and an expansion connector.

The `lilka` library simplifies working with all hardware components and lets you quickly build custom firmware in C++. It is also available in the [PlatformIO library registry](https://registry.platformio.org/libraries/and3rson/Lilka/installation).

📖 **[Official Documentation](https://docs.lilka.dev/uk/latest/library/index.html)** · 💬 **[Discord](https://discord.gg/HU68TaKCu6)** · 🐙 **[GitHub](https://github.com/lilka-dev/sdk)**

---

## Quick Start

### Prerequisites

- [PlatformIO](https://platformio.org/install) (CLI or IDE plugin)

### Minimal Example

```cpp
#include <lilka.h>

void setup() {
    lilka::begin();
    // All hardware is initialized and ready!
}

void loop() {
    lilka::display.fillScreen(lilka::colors::Black);

    while (1) {
        lilka::State state = lilka::controller.getState();

        if (state.a.justPressed) {
            lilka::buzzer.play(lilka::NOTE_A4);
            lilka::display.fillScreen(lilka::colors::Red);
        } else if (state.a.justReleased) {
            lilka::buzzer.stop();
            lilka::display.fillScreen(lilka::colors::Green);
        }
    }
}
```

Call `lilka::begin()` once in `setup()` — it initializes the display, buttons, SD card, battery, buzzer, audio, and all other subsystems.

---

## Hardware Specs (Lilka v2)

| Component       | Details                                           |
| --------------- | ------------------------------------------------- |
| **MCU**         | ESP32-S3, 240 MHz, PSRAM                          |
| **Display**     | ST7789 TFT, 240×280 px, 16-bit color (RGB565)    |
| **Buttons**     | UP, DOWN, LEFT, RIGHT, A, B, C, D, SELECT, START  |
| **Audio**       | I2S DAC output                                    |
| **Buzzer**      | Piezo buzzer (PWM)                                |
| **Storage**     | microSD card + SPIFFS                             |
| **Battery**     | LiPo with ADC monitoring                         |
| **Connectivity**| Wi-Fi, Bluetooth (NimBLE)                         |
| **Expansion**   | 6-pin expansion connector with GPIO/ADC           |

---

## SDK Modules

### Display

Full-featured 2D graphics API built on top of Arduino GFX and U8g2:

- Drawing primitives: pixels, lines, rectangles, circles, triangles, ellipses, arcs
- Image rendering: load BMP/PNG from SD card with transparency and pivot points
- Text rendering with Cyrillic font support (multiple sizes from 4×6 to 10×20)
- **Canvas** — off-screen framebuffer for flicker-free double buffering
- **Image** — 16-bit sprite with rotation, flipping, and transparency
- **Transform** — 2×2 affine transformation matrix (rotate, scale, multiply)
- HSV color conversion, text alignment helpers

```cpp
lilka::Canvas canvas;
canvas.fillScreen(lilka::colors::Black);
canvas.setCursor(10, 50);
canvas.setTextColor(lilka::colors::White);
canvas.print("Hello, Lilka!");
lilka::display.drawCanvas(&canvas);
```

### Controller

Input handling with debouncing and auto-repeat support:

- 10 hardware buttons + virtual `ANY` button
- `ButtonState` tracks `pressed`, `justPressed`, `justReleased`
- `getState()` consumes press/release flags; `peekState()` does not
- Per-button and global event handlers
- Runs input polling in a FreeRTOS task with 10ms debounce

```cpp
lilka::State state = lilka::controller.getState();
if (state.up.pressed) { /* moving up */ }
if (state.b.justPressed) { /* B was just pressed */ }
```

### Buzzer

Non-blocking piezo buzzer driver (runs in a FreeRTOS task):

- `play(frequency)` / `play(frequency, duration)` — play a tone
- `playMelody(melody, length, tempo)` — play a sequence of notes with dotted note support
- Full chromatic scale constants from `NOTE_B0` (31 Hz) to `NOTE_B8` (7902 Hz)
- Built-in melodies (e.g., `playDoom()`)

### Audio

I2S audio output with volume control:

- Configurable volume level (stored in NVS)
- Startup sound toggle
- `adjustVolume()` for PCM buffer amplitude scaling

### Resources

File and image loading utilities:

- `loadImage(path, transparentColor)` — loads BMP or PNG from SD/SPIFFS into an `Image*`
- `readFile(path, content)` / `writeFile(path, content)` — simple file I/O

### UI Widgets

Ready-made UI components for interactive applications:

| Widget             | Description                                          |
| ------------------ | ---------------------------------------------------- |
| **Menu**           | Scrollable list with icons, colors, and callbacks    |
| **Alert**          | Notification dialog with title and message           |
| **ProgressDialog** | Progress bar (0–100%) with title and message         |
| **InputDialog**    | On-screen keyboard for text input (supports masking) |

### Battery

LiPo battery monitoring through ADC voltage divider:

- `readLevel()` — returns charge percentage (0–100%) or -1 if no battery
- Configurable empty/full voltage thresholds (default 3.2V–4.2V)

### File Utilities

SD card and SPIFFS filesystem helpers:

- Initialize and check availability of SD and SPIFFS
- Directory operations: `listDir()`, `getEntryCount()`, `makePath()` (recursive mkdir)
- Path conversion between local and canonical VFS paths
- SD card formatting and partition table creation
- Human-friendly file size formatting

### MultiBoot

Load and run firmware binaries from SD card via ESP32 OTA mechanism:

- Reads `.bin` firmware from SD, writes to OTA partition, and reboots
- Auto-rollback: original firmware stays active, loaded firmware runs once
- Command-line parameter passing support

### Dynamic Loader

ELF shared object loader for running `.so` files from SD card:

- Loads 32-bit Xtensa ELF binaries into PSRAM
- Host symbol table registration for dynamic linking (up to 32 tables)
- C API (`lilka_dynloader_run()`) and C++ wrapper (`lilka::DynLoader`)
- Symbol export macros: `LILKA_DYNSYM_EXPORT()`, `LILKA_DYNSYM_END`

### Fast Math

Lookup-table-based trigonometric functions for performance-critical code:

- `fSin360(deg)` / `fCos360(deg)` — integer degree sin/cos (0–359)
- `fSin32(fract)` / `fCos32(fract)` — 32-sector circle sin/cos

### Board & SPI

- **Board**: expansion connector GPIO mapping, power saving mode (display/backlight/I2S off for deep sleep)
- **SPI**: two SPI buses — `SPI1` (HSPI) for display/SD, `SPI2` (FSPI) for user peripherals on the expansion connector

### Serial

Enhanced serial logging with colored ANSI output:

- `log()` / `err()` / `idf()` — formatted logging (green INFO, red ERROR, blue IDF)
- Runs in a separate FreeRTOS task with mutex protection
- Custom STDIO VFS so `printf`/`stdin`/`stdout` route through serial

---

## Lua Scripting

The SDK includes a **Lua 5.4** addon (`addons/lualilka/`) that provides scripting bindings for Lilka hardware:

| Module          | Purpose                    |
| --------------- | -------------------------- |
| `display`       | Display drawing API        |
| `controller`    | Button input               |
| `buzzer`        | Buzzer control             |
| `resources`     | Image/file loading         |
| `geometry`      | Geometric primitives       |
| `transforms`    | Affine transforms          |
| `math`          | Math utilities             |
| `UI`            | UI widgets                 |
| `gpio`          | GPIO access                |
| `http`          | HTTP networking            |
| `wifi`          | Wi-Fi connectivity         |
| `sdcard`        | SD card access             |
| `serial`        | Serial communication       |
| `state`         | State management           |
| `util`          | Utility functions          |

---

## Project Structure

```
sdk/
├── Makefile                  # Build targets (check, format, icons, etc.)
├── boards/
│   └── lilka_v2.json         # PlatformIO board definition for Lilka v2
├── lib/lilka/
│   ├── library.json          # PlatformIO library metadata
│   ├── platformio.ini        # Build configuration (v1/v2 environments)
│   ├── src/
│   │   ├── lilka.h           # Main include header
│   │   ├── lilka.cpp         # lilka::begin() initialization
│   │   └── lilka/            # Module source files
│   │       ├── display.{h,cpp}
│   │       ├── controller.{h,cpp}
│   │       ├── buzzer.{h,cpp}
│   │       ├── audio.{h,cpp}
│   │       ├── resources.{h,cpp}
│   │       ├── ui.h, menu.cpp, alert.cpp, inputdialog.cpp, progressdialog.cpp
│   │       ├── battery.{h,cpp}
│   │       ├── fileutils.{h,cpp}
│   │       ├── multiboot.{h,cpp}
│   │       ├── dynloader.{h,cpp}
│   │       ├── fmath.{h,cpp}
│   │       ├── board.{h,cpp}
│   │       ├── serial.{h,cpp}
│   │       ├── spi.{h,cpp}
│   │       └── config.h          # Pin definitions for v0/v1/v2
│   └── examples/
│       └── main.cpp
├── addons/lualilka/          # Lua 5.4 scripting addon
├── docs/                     # Sphinx + Doxygen documentation sources
└── tools/image2code/         # PNG-to-header conversion tool
```

---

## Development

### Makefile Targets

| Target          | Description                                               |
| --------------- | --------------------------------------------------------- |
| `make help`     | Show available targets                                    |
| `make todo`     | Find all `TODO`/`FIXME`/`XXX` comments in source files   |
| `make check`    | Run clang-format and cppcheck                             |
| `make clang-format` | Verify code formatting (clang-format-17)             |
| `make cppcheck` | Static analysis (performance + style)                     |
| `make icons`    | Convert PNG images to C headers                           |
| `make check-docker` | Run all checks in a Docker container (Ubuntu 24.04)  |

### Building Documentation

```bash
cd docs
pip install -r requirements.txt
make html
```

---

## Dependencies

| Library                          | Version | Purpose                         |
| -------------------------------- | ------- | ------------------------------- |
| GFX Library for Arduino          | 1.6.0   | Display driver (ST7789)         |
| U8g2                             | 2.35.9  | Font rendering (Cyrillic)       |
| NimBLE-Arduino                   | 1.4.3   | Bluetooth Low Energy            |

---

## License

This project is licensed under the **[GNU General Public License v2.0](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html)**.

---

## Links

- [Official Documentation](https://docs.lilka.dev)
- [SDK Library Docs](https://docs.lilka.dev/uk/latest/library/index.html)
- [KeiraOS Documentation](https://docs.lilka.dev/projects/keira/)
- [PlatformIO Registry](https://registry.platformio.org/libraries/lilka/Lilka/installation)
- [Discord Community](https://discord.gg/HU68TaKCu6)
