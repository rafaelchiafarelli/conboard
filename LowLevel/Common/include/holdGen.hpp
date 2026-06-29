// Pure hold-event generation for non-autorepeating evdev devices (gamepads,
// mouse buttons). The kernel emits value==2 autorepeat only for keyboards, so a
// `hold` / `hold_once` rule on a gamepad button would otherwise never fire.
// holdGen decides WHEN the engine should synthesize a value==2 event so that the
// pure evMatch matcher can then match it exactly as it matches a keyboard's
// native autorepeat.
//
// It is pure: time is a caller-supplied parameter (milliseconds), so it is
// unit-tested with a synthetic clock -- no real timers, no threads, mirroring
// the classifier's synthetic-input design. The engine calls press()/release()
// on the matching edges and polls due() with its real clock.
#pragma once

namespace holdgen {

// Which hold behaviour a rule asked for.
enum hold_kind {
    hold_repeat,  // fire repeatedly while held, every `ms`
    hold_once     // fire once, after the button has been held for `ms`
};

struct Config {
    hold_kind kind = hold_repeat;
    long      ms   = 0;   // repeat: interval between fires; once: delay before the one fire
};

// Per-button bookkeeping the engine keeps for each held button with a hold rule.
struct State {
    bool down      = false;
    long downSince  = 0;
    long lastFire   = 0;
    bool firedOnce  = false;
};

// Button edges. press resets the per-hold bookkeeping; release stops firing.
void press(State &s, long now);
void release(State &s);

// Should a synthetic hold (value==2) event be emitted at time `now`?
// Pure given the state; updates lastFire/firedOnce when it returns true.
//   repeat: true once per `ms` elapsed since the last fire (first fire one
//           interval after press, so it never double-fires with the press rule).
//   once  : true exactly once, the first poll at/after downSince + ms.
bool due(State &s, const Config &cfg, long now);

} // namespace holdgen
