#include "debounce.hpp"

namespace debounce {

Edge update(State &s, bool rawLevel, long now, const Config &cfg)
{
    if (rawLevel != s.raw) {
        s.raw = rawLevel;
        s.changedAt = now;
        return Edge::None;
    }
    if (s.raw != s.stable && (now - s.changedAt) >= cfg.settleMs) {
        s.stable = s.raw;
        return s.stable ? Edge::Pressed : Edge::Released;
    }
    return Edge::None;
}

}  // namespace debounce
