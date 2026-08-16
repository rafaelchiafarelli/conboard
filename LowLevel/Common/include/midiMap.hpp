// Pure MIDI action-mapping logic, split out of the MIDI thread so it can be
// unit-tested with synthetic signals (no ALSA, no device, no threads).
//
// The MIDI thread receives a 3-byte MIDI signal and must decide which of the
// current mode's Actions it triggers, and what outputs to emit. That decision
// is pure: it depends only on the incoming signal and the configured Actions.
// MIDI::processInput() uses these functions, so the tested logic and the
// runtime logic are identical.
#pragma once

#include "actions.h"
#include <vector>

namespace midimap {

// Does `incoming` trigger this MIDI `trigger`, according to its midi_mode?
//   midi_normal          : b0,b1,b2 all equal
//   midi_trigger_higher  : b0,b1 equal AND incoming b2 > trigger b2
//   midi_trigger_lower   : b0,b1 equal AND incoming b2 < trigger b2
//   midi_spot            : b0,b1 equal (b2 is a position/value, not matched)
//   midi_blink           : b0,b1,b2 all equal
//   midi_sysex           : never matches here -- see matchesSysex() below,
//                          a SysEx message isn't a midiSignal at all
//   midi_nomode / other  : never matches
bool matches(const midiActions &trigger, const midiSignal &incoming);

// SysEx trigger: exact byte-for-byte match only (see docs/next-sessions/
// 08-midi-sysex.md -- no prefix/wildcard matching in this pass). A trigger
// not in midi_sysex mode never matches here, same as matches() never
// matches a midi_sysex trigger.
bool matchesSysex(const midiActions &trigger, const std::vector<uint8_t> &incoming);

// The outputs to emit for a matched action. For spot/blink modes the incoming
// third byte is carried into each output's `spot`.
std::vector<devActions> resolveOutputs(const Actions &action, const midiSignal &incoming);

} // namespace midimap
