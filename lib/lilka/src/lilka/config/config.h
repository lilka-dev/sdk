#pragma once

#include "ini.h"

#define LILKA_CONFIG_DIR "/sd/.lilka/config"

namespace lilka {

/// Налаштування заліза з INI-файлів.
///
/// Спершу завантажуються вбудовані значення для поточної плати (для Лілки v2 - контакти роз'єму розширення
/// та модулів: секції ``ext``, ``gps``, ``can``, ``ir``, ``lora``, ``rf``, ``gyro_magnet``), потім усі файли
/// ``*.ini`` з ``/sd/.lilka/config`` в алфавітному порядку. Значення з файлів перекривають вбудовані,
/// тому досить вказати лише те, що відрізняється:
///
/// .. code-block:: ini
///
///     ; /sd/.lilka/config/modules.ini
///     [lora]
///     nss = 21
///
/// Читання значень:
///
/// .. code-block:: cpp
///
///     int nss = lilka::config.getInt("lora", "nss", -1);
///     if (lilka::config.has_key("gps", "baud")) { ... }
class Config : public IniTable {
public:
    Config();
    /// (Пере)завантажити вбудовані значення і файли з SD-картки. Викликається з lilka::begin() після
    /// монтування SD-картки; можна викликати повторно після зміни файлів.
    ///
    /// (Не ``begin()``: ``begin()`` / ``end()`` - ітератори таблиці.)
    void reload();
    /// Скільки файлів з SD-картки завантажено при останньому reload().
    int fileCount() const {
        return files;
    }

private:
    int files;
};

/// Екземпляр налаштувань.
extern Config config;

} // namespace lilka
