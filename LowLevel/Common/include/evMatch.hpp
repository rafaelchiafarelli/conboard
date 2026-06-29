// Pure evdev input-matching logic, the joystick/keyboard/mouse counterpart of
// midiMap. All three of those devices are evdev INPUT sources (a physical
// device plugged into the board), so they share ONE matcher and ONE symbol
// table -- they differ only in which /dev/input/eventN is opened and which
// symbols are meaningful.
//
// Like midiMap, this is pure: a single reduced event is matched against a rule's
// resolved trigger, with no state, no timers, no kernel headers, no /dev node.
// (The "fire repeatedly / fire once after a delay" behaviour for non-autorepeat
// devices like gamepads is NOT here -- it lives in holdGen, which synthesizes
// the value==2 events this matcher then matches. Keyboards get value==2 for free
// from the kernel's native autorepeat.)
#pragma once

#include "actions.h"
#include <string>
#include <vector>

namespace evmatch {

// Linux evdev event-type classes (from input-event-codes.h), inlined so this
// module needs no kernel headers -- consistent with deviceDetect using raw ints.
enum { EV_KEY_ = 0x01, EV_REL_ = 0x02, EV_ABS_ = 0x03 };

// One evdev event reduced to what matching needs. The reader fills this from a
// real `struct input_event` (dropping the timestamp); matching never sees the
// kernel struct.
//   key (BTN_*/KEY_*): value 1 = press, 0 = release, 2 = autorepeat/hold
//   abs/rel          : value is the axis magnitude / relative delta
struct evEvent {
    int type = 0;   // EV_KEY_ / EV_ABS_ / EV_REL_
    int code = 0;   // KEY_*/BTN_* | ABS_* | REL_*
    int value = 0;
};

// Match modes. Keys use press/release/hold/hold_once; axes & rel use
// higher/lower/spot. hold and hold_once match IDENTICALLY here (both are a
// value==2 event) -- the distinction is metadata holdGen reads to decide
// repeat-vs-once; the matcher stays mode-simple and pure.
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
};

// Resolve a symbolic name (e.g. "BTN_SOUTH", "ABS_X", "KEY_A", "REL_WHEEL") to
// its evdev (type, code), filling out.type/out.code. The prefix implies the
// EV_ class, so a symbol fully determines the type. Returns false (out
// untouched) for an unknown symbol. mode/threshold are left for the caller.
bool resolveSymbol(const std::string &name, evTrigger &out);

// Does `incoming` trigger this `trigger`? Pure and stateless. Requires type and
// code to match; then applies the mode's value test.
bool matches(const evTrigger &trigger, const evEvent &incoming);

// The outputs to emit for a matched action. For ev_spot the incoming value is
// carried into each output's `spot` (axis/rel pass-through), mirroring midiMap's
// spot/blink handling. `mode` is passed explicitly because the trigger is not
// (yet) stored on the Actions model.
std::vector<devActions> resolveOutputs(const Actions &action,
                                       const evEvent &incoming,
                                       ev_mode mode);

} // namespace evmatch
