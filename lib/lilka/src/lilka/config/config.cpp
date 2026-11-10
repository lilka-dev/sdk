#include "config.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "defaults.h"
#include "lilka/serial.h"

namespace lilka {

Config config;

static const int MAX_FILES = 32;

Config::Config() : files(0) {
    // Defaults are available even before begin() (and in programs that never mount the SD card).
    parse(LILKA_CONFIG_DEFAULTS);
}

static int compareNames(const void* a, const void* b) {
    return strcmp(*static_cast<char* const*>(a), *static_cast<char* const*>(b));
}

void Config::reload() {
    clear();
    parse(LILKA_CONFIG_DEFAULTS);
    files = 0;

    DIR* dir = opendir(LILKA_CONFIG_DIR);
    if (!dir) return; // no SD card or no config directory: defaults only

    // Sorted, so that overrides don't depend on the directory order.
    char* names[MAX_FILES] = {};
    int n = 0;
    const struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr && n < MAX_FILES) {
        size_t len = strlen(entry->d_name);
        if (len > 4 && !strcasecmp(entry->d_name + len - 4, ".ini")) {
            names[n] = strdup(entry->d_name);
            if (names[n]) n++;
        }
    }
    closedir(dir);
    qsort(names, n, sizeof(char*), compareNames);

    for (int i = 0; i < n; i++) {
        char path[300];
        snprintf(path, sizeof(path), "%s/%s", LILKA_CONFIG_DIR, names[i]);
        int errors = 0;
        if (load(path, &errors)) {
            files++;
            if (errors) serial.err("config: %s: %d invalid line(s) skipped", path, errors);
        } else {
            serial.err("config: can't read %s", path);
        }
        free(names[i]);
    }
}

} // namespace lilka
