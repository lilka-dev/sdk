#pragma once

#include <stdint.h>
namespace lilka {

/// Коди для спеціальних пінів роз'єму розширення.
typedef enum {
    /// Недійсний пін.
    INVALID = 255,
    /// Земля.
    GND = 254,
    /// Живлення.
    VCC = 253,
} ExtPin;

/// Клас для керування платою.
///
/// Ініціалізує роз'єм розширення та режим енергозбереження.
///
/// Приклад використання:
///
/// @code
/// #include <lilka.h>
///
/// void setup() {
///     lilka::begin();
/// }
///
/// void loop() {
///     lilka.board.enablePowerSavingMode(); // Вимкнути дисплей та I2S-модуль
///     ESP.deepSleep(1000000); // Перейти в режим глибокого сну на 1 секунду
///     lilka.board.disablePowerSavingMode(); // Увімкнути дисплей та I2S-модуль
///     delay(1000);
/// }
/// @endcode
class Board {
public:
    Board();
    /// Налаштувати плату.
    /// \warning Цей метод викликається автоматично при виклику `lilka::begin()`.
    void begin();
    /// Увімкнути режим енергозбереження.
    ///
    /// Цей метод вимикає дисплей, підсвітку дисплея та I2S-модуль. Його варто викликати перед входом в режим сну або глибокого сну.
    void enablePowerSavingMode();
    /// Вимкнути режим енергозбереження.
    ///
    /// Цей метод вмикає дисплей, підсвітку дисплея та I2S-модуль. Його варто викликати після виходу з режиму сну.
    void disablePowerSavingMode();
    /// Отримати номер GPIO, що відповідає пінові з роз'єму розширення за індексом.
    ///
    /// Повертає номер піна роз'єму розширення за індексом. Нульовий індекс - це пін, що має квадратну форму.
    ///
    /// @param index Індекс піна роз'єму розширення.
    /// @return Номер GPIO, що відповідає даному піну з роз'єму розширення. Якщо цей пін - спеціальний (наприклад, земля або живлення), повертається відповідний код з переліку `lilka::ExtPin`.
    /// @see ExtPin
    uint8_t getExtPinGPIO(uint8_t index);
};

/// Екземпляр класу `Board`, який можна використовувати для керування платою.
/// Вам не потрібно інстанціювати `Board` вручну.
extern Board board;

} // namespace lilka
