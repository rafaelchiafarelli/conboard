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

#include "evTypes.hpp"       // evEvent / ev_mode / evTrigger / EV_* (shared with actions.h)
#include "actions.h"
#include <string>
#include <vector>

namespace evmatch {

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
