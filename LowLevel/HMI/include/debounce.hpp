// Pure debounce for one GPIO line (a plain pushbutton or an encoder's own
// pushbutton). Time is caller-supplied (milliseconds) rather than read from a
// clock, so it is unit-tested with a synthetic clock -- no real timers, no
// threads -- mirroring holdGen's design in LowLevel/Common. push_button.cpp is
// the thin libgpiod wrapper that feeds real pin readings + a real clock in.
#pragma once

namespace debounce {

struct State {
    bool raw       = false;  // last raw level fed in
    bool stable    = false;  // last accepted (debounced) level
    long changedAt = 0;      // when `raw` last changed
};

struct Config {
    long settleMs = 20;      // raw level must hold this long before it's accepted
};

enum class Edge { None, Pressed, Released };

// Feed one raw reading at time `now`. Returns Pressed/Released the instant a
// new stable level is accepted; None otherwise, including while a change is
// still settling (mid-bounce) or the level simply hasn't changed.
Edge update(State &s, bool rawLevel, long now, const Config &cfg = Config{});

} // namespace debounce
