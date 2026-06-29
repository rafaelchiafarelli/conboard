// Shared evdev value types, kept separate from evMatch.hpp so the core data
// model (actions.h) can embed an evTrigger on a rule WITHOUT pulling in the
// matcher (evMatch.hpp itself includes actions.h, which would otherwise be a
// circular include). This header includes nothing of ours -- it is leaf data.
#pragma once

namespace evmatch {

// Linux evdev event-type classes (from input-event-codes.h), inlined so this
// needs no kernel headers -- consistent with deviceDetect using raw ints.
enum { EV_KEY_ = 0x01, EV_REL_ = 0x02, EV_ABS_ = 0x03 };

// One evdev event reduced to what matching needs. The reader fills this from a
// real `struct input_event` (dropping the timestamp).
//   key (BTN_*/KEY_*): value 1 = press, 0 = release, 2 = autorepeat/hold
//   abs/rel          : value is the axis magnitude / relative delta
struct evEvent {
    int type = 0;   // EV_KEY_ / EV_ABS_ / EV_REL_
    int code = 0;   // KEY_*/BTN_* | ABS_* | REL_*
    int value = 0;
};

// Match modes. Keys use press/release/hold/hold_once; axes & rel use
// higher/lower/spot. hold and hold_once match IDENTICALLY in the matcher (both
// are a value==2 event) -- the distinction is metadata holdGen reads to decide
// repeat-vs-once.
enum ev_mode {
    ev_press,      // EV_KEY value == 1
    ev_release,    // EV_KEY value == 0
    ev_hold,       // EV_KEY value == 2 (native autorepeat or holdGen-synthesized)
    ev_hold_once,  // EV_KEY value == 2 (matched like hold; fired once by holdGen)
    ev_higher,     // EV_ABS/EV_REL value > threshold
    ev_lower,      // EV_ABS/EV_REL value < threshold
    ev_spot,       // EV_ABS/EV_REL: matches the code; value is passed through
    ev_nomode
};

// A rule's input trigger after the symbol has been resolved to (type, code).
struct evTrigger {
    int     type = 0;
    int     code = 0;
    ev_mode mode = ev_nomode;
    int     threshold = 0;   // the JSON "value", for ev_higher / ev_lower
    long    holdMs = 0;      // ev_hold: repeat interval; ev_hold_once: delay
};

} // namespace evmatch
