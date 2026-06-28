#include "midiMap.hpp"

namespace midimap {

bool matches(const midiActions &trigger, const midiSignal &incoming) {
    const auto &t = trigger.midi.byte;
    const auto &in = incoming.byte;

    switch (trigger.midi_mode) {
        case midi_normal:
            return t[0] == in[0] && t[1] == in[1] && t[2] == in[2];

        case midi_trigger_higher:
            // fires when the incoming value rises above the trigger threshold
            return t[0] == in[0] && t[1] == in[1] && t[2] < in[2];

        case midi_trigger_lower:
            return t[0] == in[0] && t[1] == in[1] && t[2] > in[2];

        case midi_spot:
            // b2 is a position/value carried to the output, NOT matched.
            // (Previously this case fell through into midi_blink — fixed.)
            return t[0] == in[0] && t[1] == in[1];

        case midi_blink:
            return t[0] == in[0] && t[1] == in[1] && t[2] == in[2];

        case midi_nomode:
        default:
            return false;
    }
}

std::vector<devActions> resolveOutputs(const Actions &action, const midiSignal &incoming) {
    std::vector<devActions> outs = action.out;

    const midi_action_mode m = action.in.mAct.midi_mode;
    if (m == midi_spot || m == midi_blink) {
        for (auto &o : outs) o.spot = (int) incoming.byte[2];
    }
    return outs;
}

} // namespace midimap
