// Unit tests for the board-agnostic device classifier. Pure logic, synthetic
// inputs — no real hardware, no /dev/input, runs identically on host and QEMU.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "deviceDetect.hpp"
#include <linux/input.h>   // EV_*, KEY_*, BTN_*, ABS_*, REL_* codes

using namespace condetect;

// Build an EvdevBits tersely: caps({types}, {keys}, {absAxes}, {relAxes}).
static EvdevBits caps(std::vector<int> types, std::vector<int> keys,
                      std::vector<int> absAxes = {}, std::vector<int> relAxes = {}) {
    return EvdevBits{std::move(types), std::move(keys),
                     std::move(absAxes), std::move(relAxes)};
}

TEST_SUITE("classifier") {
    TEST_CASE("keyboard: reports the alphabetic keys") {
        CHECK(classifyEvdev(caps({EV_KEY}, {KEY_A, KEY_Z, KEY_SPACE})) == "keyboard");
    }
    TEST_CASE("mouse: relative X/Y plus a left button") {
        CHECK(classifyEvdev(caps({EV_KEY, EV_REL}, {BTN_LEFT}, {}, {REL_X, REL_Y}))
              == "mouse");
    }
    TEST_CASE("buttons-only HID is not mistaken for keyboard/mouse") {
        CHECK(classifyEvdev(caps({EV_KEY}, {BTN_0})) == "buttons / HID (other)");
    }
    TEST_CASE("empty capabilities -> unknown") {
        CHECK(classifyEvdev(EvdevBits{}) == "unknown");
    }
}

TEST_SUITE("joystick") {
    // The headline example: a gamepad that reports the "A" button. On Linux
    // BTN_A == BTN_GAMEPAD, so a pad announcing BTN_A (with X/Y axes) must be
    // classified as a joystick/gamepad — not "buttons / HID (other)".
    TEST_CASE("hitTheAButton: gamepad with BTN_A + abs axes is a joystick") {
        CHECK(classifyEvdev(caps({EV_KEY, EV_ABS}, {BTN_A}, {ABS_X, ABS_Y}))
              == "joystick / gamepad");
    }
    TEST_CASE("an explicit BTN_JOYSTICK is a joystick") {
        CHECK(classifyEvdev(caps({EV_KEY}, {BTN_JOYSTICK})) == "joystick / gamepad");
    }
    TEST_CASE("abs axes without any joystick button are not a joystick") {
        // A touchpad-like device (abs X/Y but no gamepad/trigger button).
        CHECK(classifyEvdev(caps({EV_ABS}, {}, {ABS_X, ABS_Y})) != "joystick / gamepad");
    }
}

TEST_SUITE("filter") {
    TEST_CASE("a hub-only device is not actionable") {
        CHECK_FALSE(isActionableInterfaces(":090000:"));
    }
    TEST_CASE("HID (keyboard) is actionable") {
        CHECK(isActionableInterfaces(":030101:"));
    }
    TEST_CASE("audio / MIDIStreaming is actionable") {
        CHECK(isActionableInterfaces(":010100:010200:010300:"));
    }
    TEST_CASE("mass-storage-only is not actionable") {
        CHECK_FALSE(isActionableInterfaces(":080650:"));
    }
    TEST_CASE("composite hub+HID counts (any actionable interface)") {
        CHECK(isActionableInterfaces(":090000:030101:"));
    }
    TEST_CASE("empty / absent is not actionable") {
        CHECK_FALSE(isActionableInterfaces(""));
    }
}

TEST_SUITE("usb") {
    TEST_CASE("Audio/MIDIStreaming is reported as MIDI") {
        CHECK(usbClassName(0x01, 0x03, 0x00) == "Audio / MIDIStreaming (MIDI)");
    }
    TEST_CASE("HID protocols: keyboard and mouse") {
        CHECK(usbClassName(0x03, 0x01, 0x01) == "HID / keyboard");
        CHECK(usbClassName(0x03, 0x01, 0x02) == "HID / mouse");
    }
    TEST_CASE("mass storage and hub") {
        CHECK(usbClassName(0x08, 0x06, 0x50) == "Mass storage");
        CHECK(usbClassName(0x09, 0x00, 0x00) == "Hub");
    }
}
