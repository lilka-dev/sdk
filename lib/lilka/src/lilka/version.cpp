#include "version.h"

namespace lilka {

int SDK_get_curr_version_type() {
    return SDK_VERSION_TYPE;
}

version_t SDK_get_curr_version() {
    version_t version = {
        .major = SDK_VERSION_MAJOR,
        .minor = SDK_VERSION_MINOR,
        .patch = SDK_VERSION_PATCH,
        .version_type = SDK_VERSION_TYPE
    };
    return version;
}
} // namespace lilka
