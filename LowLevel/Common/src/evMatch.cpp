#include "evMatch.hpp"

namespace evmatch {

namespace {
struct Symbol { const char *name; int type; int code; };

// Curated symbol table. Values are the standard codes from Linux
// input-event-codes.h. Not exhaustive (especially KEY_*) -- extend as devices
// need it; an unknown symbol simply fails resolveSymbol().
const Symbol kSymbols[] = {
    // --- gamepad buttons (EV_KEY) ---
    {"BTN_SOUTH",  EV_KEY_, 0x130}, {"BTN_EAST",   EV_KEY_, 0x131},
    {"BTN_C",      EV_KEY_, 0x132}, {"BTN_NORTH",  EV_KEY_, 0x133},
    {"BTN_WEST",   EV_KEY_, 0x134}, {"BTN_Z",      EV_KEY_, 0x135},
    {"BTN_TL",     EV_KEY_, 0x136}, {"BTN_TR",     EV_KEY_, 0x137},
    {"BTN_TL2",    EV_KEY_, 0x138}, {"BTN_TR2",    EV_KEY_, 0x139},
    {"BTN_SELECT", EV_KEY_, 0x13a}, {"BTN_START",  EV_KEY_, 0x13b},
    {"BTN_MODE",   EV_KEY_, 0x13c}, {"BTN_THUMBL", EV_KEY_, 0x13d},
    {"BTN_THUMBR", EV_KEY_, 0x13e},
    // --- mouse buttons (EV_KEY) ---
    {"BTN_LEFT",   EV_KEY_, 0x110}, {"BTN_RIGHT",  EV_KEY_, 0x111},
    {"BTN_MIDDLE", EV_KEY_, 0x112},
    // --- keyboard letters (EV_KEY) ---
    {"KEY_A", EV_KEY_, 30}, {"KEY_B", EV_KEY_, 48}, {"KEY_C", EV_KEY_, 46},
    {"KEY_D", EV_KEY_, 32}, {"KEY_E", EV_KEY_, 18}, {"KEY_F", EV_KEY_, 33},
    {"KEY_G", EV_KEY_, 34}, {"KEY_H", EV_KEY_, 35}, {"KEY_I", EV_KEY_, 23},
    {"KEY_J", EV_KEY_, 36}, {"KEY_K", EV_KEY_, 37}, {"KEY_L", EV_KEY_, 38},
    {"KEY_M", EV_KEY_, 50}, {"KEY_N", EV_KEY_, 49}, {"KEY_O", EV_KEY_, 24},
    {"KEY_P", EV_KEY_, 25}, {"KEY_Q", EV_KEY_, 16}, {"KEY_R", EV_KEY_, 19},
    {"KEY_S", EV_KEY_, 31}, {"KEY_T", EV_KEY_, 20}, {"KEY_U", EV_KEY_, 22},
    {"KEY_V", EV_KEY_, 47}, {"KEY_W", EV_KEY_, 17}, {"KEY_X", EV_KEY_, 45},
    {"KEY_Y", EV_KEY_, 21}, {"KEY_Z", EV_KEY_, 44},
    // --- keyboard digits (EV_KEY) ---
    {"KEY_1", EV_KEY_, 2},  {"KEY_2", EV_KEY_, 3},  {"KEY_3", EV_KEY_, 4},
    {"KEY_4", EV_KEY_, 5},  {"KEY_5", EV_KEY_, 6},  {"KEY_6", EV_KEY_, 7},
    {"KEY_7", EV_KEY_, 8},  {"KEY_8", EV_KEY_, 9},  {"KEY_9", EV_KEY_, 10},
    {"KEY_0", EV_KEY_, 11},
    // --- keyboard common/control keys (EV_KEY) ---
    {"KEY_ESC",       EV_KEY_, 1},   {"KEY_BACKSPACE", EV_KEY_, 14},
    {"KEY_TAB",       EV_KEY_, 15},  {"KEY_ENTER",     EV_KEY_, 28},
    {"KEY_SPACE",     EV_KEY_, 57},  {"KEY_LEFTCTRL",  EV_KEY_, 29},
    {"KEY_RIGHTCTRL", EV_KEY_, 97},  {"KEY_LEFTSHIFT", EV_KEY_, 42},
    {"KEY_RIGHTSHIFT",EV_KEY_, 54},  {"KEY_LEFTALT",   EV_KEY_, 56},
    {"KEY_RIGHTALT",  EV_KEY_, 100}, {"KEY_LEFTMETA",  EV_KEY_, 125},
    {"KEY_UP",        EV_KEY_, 103}, {"KEY_DOWN",      EV_KEY_, 108},
    {"KEY_LEFT",      EV_KEY_, 105}, {"KEY_RIGHT",     EV_KEY_, 106},
    // --- absolute axes (EV_ABS) ---
    {"ABS_X",  EV_ABS_, 0}, {"ABS_Y",  EV_ABS_, 1}, {"ABS_Z",  EV_ABS_, 2},
    {"ABS_RX", EV_ABS_, 3}, {"ABS_RY", EV_ABS_, 4}, {"ABS_RZ", EV_ABS_, 5},
    {"ABS_HAT0X", EV_ABS_, 16}, {"ABS_HAT0Y", EV_ABS_, 17},
    // --- relative axes (EV_REL) ---
    {"REL_X", EV_REL_, 0}, {"REL_Y", EV_REL_, 1},
    {"REL_HWHEEL", EV_REL_, 6}, {"REL_WHEEL", EV_REL_, 8},
};

bool isKey(int type) { return type == EV_KEY_; }
} // namespace

bool resolveSymbol(const std::string &name, evTrigger &out) {
    for (const auto &s : kSymbols) {
        if (name == s.name) {
            out.type = s.type;
            out.code = s.code;
            return true;
        }
    }
    return false;
}

bool matches(const evTrigger &trigger, const evEvent &incoming) {
    if (trigger.type != incoming.type || trigger.code != incoming.code)
        return false;

    switch (trigger.mode) {
        case ev_press:
            return isKey(incoming.type) && incoming.value == 1;
        case ev_release:
            return isKey(incoming.type) && incoming.value == 0;
        case ev_hold:
        case ev_hold_once:
            // Both fire on a value==2 event (kernel autorepeat for keyboards,
            // holdGen-synthesized for gamepads). repeat-vs-once is holdGen's job.
            return isKey(incoming.type) && incoming.value == 2;

        case ev_higher:
            return incoming.value > trigger.threshold;
        case ev_lower:
            return incoming.value < trigger.threshold;
        case ev_spot:
            // value is a position/delta carried to the output, not matched.
            return true;

        case ev_nomode:
        default:
            return false;
    }
}

std::vector<devActions> resolveOutputs(const Actions &action,
                                       const evEvent &incoming,
                                       ev_mode mode) {
    std::vector<devActions> outs = action.out;
    if (mode == ev_spot) {
        for (auto &o : outs) o.spot = incoming.value;
    }
    return outs;
}

} // namespace evmatch
