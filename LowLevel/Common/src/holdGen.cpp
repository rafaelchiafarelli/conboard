#include "holdGen.hpp"

namespace holdgen {

void press(State &s, long now) {
    s.down      = true;
    s.downSince = now;
    s.lastFire  = now;   // first repeat fires one interval later, not on press
    s.firedOnce = false;
}

void release(State &s) {
    s.down = false;
}

bool due(State &s, const Config &cfg, long now) {
    if (!s.down)
        return false;

    if (cfg.kind == hold_repeat) {
        if (now - s.lastFire >= cfg.ms) {
            s.lastFire = now;
            return true;
        }
        return false;
    }

    // hold_once
    if (!s.firedOnce && now - s.downSince >= cfg.ms) {
        s.firedOnce = true;
        return true;
    }
    return false;
}

} // namespace holdgen
