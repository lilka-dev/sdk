#pragma once

#include <stdint.h>
#include <stddef.h>

namespace lilka {
namespace mod {

/// ІЧ-протоколи, які вміє кодувати і розпізнавати IrCodec.
enum class IrProtocol : uint8_t {
    Unknown,
    NEC, ///< 8-бітна адреса (або 16-бітна "extended"), 8-бітна команда
    Samsung, ///< 8-бітна адреса, 8-бітна команда
    Sony, ///< SIRC: 7-бітна команда, 5/8/13-бітна адреса (12/15/20 біт)
};

struct IrCode {
    IrProtocol protocol;
    uint32_t address;
    uint32_t command;
    uint8_t bits; ///< для Sony: 12, 15 або 20
    bool repeat; ///< NEC: кадр повторення (кнопку утримують)
};

/// Кодування/декодування ІЧ-кадрів у вигляді тривалостей імпульсів (мкс).
///
/// Тривалості чергуються: імпульс (несуча увімкнена), пауза, імпульс, ...; перший елемент - імпульс.
namespace IrCodec {

/// Закодувати кадр. Повертає кількість тривалостей або 0, якщо не вистачило місця / протокол невідомий.
size_t encode(const IrCode& code, uint16_t* out, size_t max);
/// Закодувати кадр повторення NEC.
size_t encodeNecRepeat(uint16_t* out, size_t max);
/// Розпізнати кадр. Повертає false, якщо жоден протокол не підійшов.
bool decode(const uint16_t* durations, size_t count, IrCode& out);
/// Рекомендована пауза між повтореннями кадру, мс.
uint32_t repeatGapMs(IrProtocol protocol);
/// Назва протоколу ("NEC", "SAMSUNG", "SONY" або "UNKNOWN").
const char* name(IrProtocol protocol);

} // namespace IrCodec

} // namespace mod
} // namespace lilka
