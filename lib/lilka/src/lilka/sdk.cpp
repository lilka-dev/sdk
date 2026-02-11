#include "sdk.h"

namespace lilka {
LilkaSDK::LilkaSDK() {
    // Prepare all data at once and reuse
    version = {
        .major = SDK_VERSION_MAJOR, .minor = SDK_VERSION_MINOR, .patch = SDK_VERSION_PATCH, .vtype = SDK_VERSION_TYPE
    };

    char tmpBuffer[260] = {};
    sprintf(
        tmpBuffer,
        "LilkaSDK v%d.%d.%d %s",
        version.major,
        version.minor,
        version.patch,
        SDK_VERSION_TYPE_ACSTR[version.vtype]
    );
    versionStr = tmpBuffer;
}

version_t LilkaSDK::getVersion() {
    return version;
}

const String& LilkaSDK::getVersionStr() {
    return versionStr;
}

version_type_t LilkaSDK::getVersionType() {
    return versionType;
}

char SDK_VERSION_TYPE_ACSTR[][12] = {"Dev", "Pre-Release", "Release"};
LilkaSDK sdk;
} // namespace lilka
