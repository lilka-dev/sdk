#pragma once

#ifndef VERSION_SDK_H
#    define VERSION_SDK_H

#    include <lilka.h>
#    include "version_auto_gen.h"

namespace lilka {

enum SDK_VERSION_TYPE : uint8_t {
    SDK_VERSION_TYPE_DEV = 0,
    SDK_VERSION_TYPE_PRE_RELEASE = 1,
    SDK_VERSION_TYPE_RELEASE = 2
};

typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint8_t version_type;
} version_t;

int SDK_get_curr_version_type();
version_t SDK_get_curr_version();
} // namespace lilka

#endif