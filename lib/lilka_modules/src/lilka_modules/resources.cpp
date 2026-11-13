#include "resources.h"

#include <string.h>

namespace lilka {
namespace mod {

Resources resources;

Resources::Resources() {
    memset(pins, 0, sizeof(pins));
    memset(periphs, 0, sizeof(periphs));
    mux = portMUX_INITIALIZER_UNLOCKED;
}

bool Resources::take(Slot& slot, const void* who) {
    if (slot.refs && slot.owner != who) return false;
    slot.owner = who;
    slot.refs++;
    return true;
}

uint16_t Resources::give(Slot& slot, const void* who) {
    if (!slot.refs || slot.owner != who) return 0;
    slot.refs--;
    if (!slot.refs) slot.owner = nullptr;
    return slot.refs;
}

bool Resources::claimPins(std::initializer_list<int> gpios, const void* who, int* conflict) {
    bool ok = true;
    portENTER_CRITICAL(&mux);
    for (int gpio : gpios) {
        if (gpio < 0 || gpio >= SOC_GPIO_PIN_COUNT) continue;
        if (pins[gpio].refs && pins[gpio].owner != who) {
            if (conflict) *conflict = gpio;
            ok = false;
            break;
        }
    }
    if (ok) {
        for (int gpio : gpios) {
            if (gpio < 0 || gpio >= SOC_GPIO_PIN_COUNT) continue;
            take(pins[gpio], who);
        }
    }
    portEXIT_CRITICAL(&mux);
    return ok;
}

void Resources::releasePins(std::initializer_list<int> gpios, const void* who) {
    portENTER_CRITICAL(&mux);
    for (int gpio : gpios) {
        if (gpio < 0 || gpio >= SOC_GPIO_PIN_COUNT) continue;
        give(pins[gpio], who);
    }
    portEXIT_CRITICAL(&mux);
}

bool Resources::claimPin(int gpio, const void* who) {
    return claimPins({gpio}, who);
}

void Resources::releasePin(int gpio, const void* who) {
    releasePins({gpio}, who);
}

bool Resources::claim(Periph periph, const void* who) {
    portENTER_CRITICAL(&mux);
    bool ok = take(periphs[static_cast<int>(periph)], who);
    portEXIT_CRITICAL(&mux);
    return ok;
}

uint16_t Resources::release(Periph periph, const void* who) {
    portENTER_CRITICAL(&mux);
    uint16_t left = give(periphs[static_cast<int>(periph)], who);
    portEXIT_CRITICAL(&mux);
    return left;
}

void Resources::releaseAll(const void* who) {
    portENTER_CRITICAL(&mux);
    for (Slot& slot : pins) {
        if (slot.owner == who) slot = {nullptr, 0};
    }
    for (Slot& slot : periphs) {
        if (slot.owner == who) slot = {nullptr, 0};
    }
    portEXIT_CRITICAL(&mux);
}

const void* Resources::pinOwner(int gpio) {
    if (gpio < 0 || gpio >= SOC_GPIO_PIN_COUNT) return nullptr;
    portENTER_CRITICAL(&mux);
    const void* o = pins[gpio].owner;
    portEXIT_CRITICAL(&mux);
    return o;
}

const void* Resources::owner(Periph periph) {
    portENTER_CRITICAL(&mux);
    const void* o = periphs[static_cast<int>(periph)].owner;
    portEXIT_CRITICAL(&mux);
    return o;
}

} // namespace mod
} // namespace lilka
