#pragma once

#include <lilka/config/config.h>

namespace lilka {
namespace mod {
namespace pins {

/// Контакт не під'єднано.
constexpr int NC = -1;

/// Контакт модуля з lilka::config (секція - плата модуля, ключ - сигнал), або NC, якщо його не задано.
///
/// Вбудовані значення для Лілки v2 описані в lilka/config/defaults.h; їх можна перекрити файлом
/// у ``/sd/.lilka/config``, наприклад ``[lora] nss = 21``.
inline int get(const char* section, const char* key) {
    return (int)lilka::config.getInt(section, key, NC);
}

} // namespace pins
} // namespace mod
} // namespace lilka
