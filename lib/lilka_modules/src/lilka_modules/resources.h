#pragma once

#include <stdint.h>
#include <initializer_list>
#include <freertos/FreeRTOS.h>
#include <soc/soc_caps.h>

namespace lilka {
namespace mod {

/// Периферія, яку може захопити лише один власник.
enum class Periph : uint8_t {
    UART1,
    UART2,
    SPI2,
    I2C0,
    TWAI,
    RMT_TX,
    RMT_RX,
    COUNT,
};

/// Облік контактів і периферії, які зараз використовуються.
///
/// Власник - будь-який вказівник, що ідентифікує користувача (наприклад, стан Lua-скрипта).
/// Один власник може захоплювати той самий ресурс кілька разів (лічильник посилань), інші власники - ні.
/// Номери контактів < 0 (``pins::NC``) пропускаються.
class Resources {
public:
    Resources();

    /// Захопити контакти. Якщо хоча б один зайнятий іншим власником, нічого не захоплюється.
    /// @param conflict Якщо не nullptr, сюди записується номер зайнятого контакту.
    bool claimPins(std::initializer_list<int> gpios, const void* who, int* conflict = nullptr);
    /// Звільнити контакти (по одному посиланню на кожен).
    void releasePins(std::initializer_list<int> gpios, const void* who);
    bool claimPin(int gpio, const void* who);
    void releasePin(int gpio, const void* who);

    /// Захопити периферію.
    bool claim(Periph periph, const void* who);
    /// Звільнити периферію. Повертає кількість посилань, що залишились у цього власника.
    uint16_t release(Periph periph, const void* who);

    /// Звільнити все, що захопив власник.
    void releaseAll(const void* who);

    /// Власник контакту або nullptr.
    const void* pinOwner(int gpio);
    /// Власник периферії або nullptr.
    const void* owner(Periph periph);

private:
    struct Slot {
        const void* owner;
        uint16_t refs;
    };
    static bool take(Slot& slot, const void* who);
    static uint16_t give(Slot& slot, const void* who);

    Slot pins[SOC_GPIO_PIN_COUNT];
    Slot periphs[static_cast<int>(Periph::COUNT)];
    portMUX_TYPE mux;
};

/// Спільний облік ресурсів для всіх модулів.
extern Resources resources;

} // namespace mod
} // namespace lilka
