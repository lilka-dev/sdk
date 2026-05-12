#include <Arduino.h>

#include "board.h"
#include "display.h"
#include "config.h"
#include <esp32-hal-gpio.h>
#include <esp32-hal.h>
#include <lilka/serial.h>

namespace lilka {

Board::Board() {
}

void Board::begin() {
#define PIN_AWAIT_TIME 60
#if LILKA_VERSION == 2
    //////////////////////////////////////////////////////////////////////////
    // Perform soldering test
    //////////////////////////////////////////////////////////////////////////
    // For V2:
    // GPIO19-20                     USB
    // GPIO0, GPIO3, GPIO45, GPIO 46 Strap pins
    // GPIO26-32                     Reserved for PSRAM
    // GPIO33-37                     Reserved for Octal PSRAM
    //////////////////////////////////////////////////////////////////////////
    const int testPins[] = {1,  2,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
                            18, 21, 22, 23, 24, 25, 38, 39, 40, 41, 42, 43, 44, 47, 48};
    // Configure pin mode for all pins
    for (auto pin : testPins) {
        pinMode(pin, INPUT_PULLDOWN);
    }
    // Await pin to stabilize it's state(voltage, etc.)
    delayMicroseconds(PIN_AWAIT_TIME);

    // Test connections based on a fact
    // that pins shouldn't be shorted
    for (int i = 0; i < sizeof(testPins) / sizeof(testPins[0]); i++) {
        // Setup high on a testPin
        pinMode(testPins[i], OUTPUT);
        digitalWrite(testPins[i], HIGH);
        delayMicroseconds(PIN_AWAIT_TIME);

        // Read other testPins to check on shorts
        for (int j = i + 1; j < sizeof(testPins) / sizeof(testPins[0]); j++) {
            bool shorted = digitalRead(testPins[j]) == HIGH;
            if (shorted) {
                lilka::serial.log("Found connection between GPIOs %d and %d\n", testPins[i], testPins[j]);
            }
        }
        // Restore testPin state
        digitalWrite(testPins[i], LOW);
        pinMode(testPins[i], INPUT_PULLDOWN);
    }

#endif

    // Iterate over the pins, set them as inputs
#if LILKA_VERSION >= 2
    for (int pin : LILKA_EXT_PINS) {
        pinMode(pin, INPUT);
    }
    pinMode(LILKA_SLEEP, OUTPUT);
    digitalWrite(LILKA_SLEEP, HIGH);
#endif
}

void Board::enablePowerSavingMode() {
#if LILKA_VERSION >= 2
    digitalWrite(LILKA_SLEEP, LOW);
#endif
    display.displayOff();
}

void Board::disablePowerSavingMode() {
#if LILKA_VERSION >= 2
    digitalWrite(LILKA_SLEEP, HIGH);
#endif
    display.displayOn();
}

uint8_t Board::getExtPinGPIO(uint8_t index) {
#if LILKA_VERSION == 1
    return ExtPin::INVALID;
#elif LILKA_VERSION == 2
    // TODO: hardcoded values
    if (index == 0 || index == 11) {
        return ExtPin::GND;
    } else if (index == 1 || index == 10) {
        return ExtPin::VCC;
    } else if (index <= 7) {
        return LILKA_EXT_PINS[index - 2];
    } else if (index == 8) {
        return TX;
    } else if (index == 9) {
        return RX;
    } else {
        return ExtPin::INVALID;
    }
#else
#    error "getExtPin is not defined for this board version"
#endif
}

Board board;

} // namespace lilka
