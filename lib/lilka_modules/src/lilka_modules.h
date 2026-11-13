#pragma once

/// Драйвери модулів розширення Лілки v2.
///
/// Кожен модуль - звичайний клас: створіть екземпляр, коли модуль потрібен, і викличте begin().
/// Нічого не ініціалізується автоматично в lilka::begin().

#include "lilka_modules/pins.h"
#include "lilka_modules/resources.h"
#include "lilka_modules/nmea.h"
#include "lilka_modules/gps.h"
#include "lilka_modules/can.h"
#include "lilka_modules/ir_codec.h"
#include "lilka_modules/ir.h"
#include "lilka_modules/sx126x.h"
#include "lilka_modules/nrf24.h"
#include "lilka_modules/cc1101.h"
#include "lilka_modules/bmi260.h"
#include "lilka_modules/tmag5273.h"
