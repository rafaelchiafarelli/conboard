// Unit tests for the evdev input-matching logic (LowLevel/Common/evMatch.*).
// Pure: synthetic evEvents + synthetic triggers, no /dev/input, no hardware.
#include "doctest.h"          // main() is provided by test_devicedetect.cpp

#include "evMatch.hpp"

using namespace evmatch;

// --- tiny builders ----------------------------------------------------------
static evEvent ev(int type, int code, int value) {
    evEvent e; e.type = type; e.code = code; e.value = value; return e;
}
static evTrigger trig(int type, int code, ev_mode mode, int threshold = 0) {
    evTrigger t; t.type = type; t.code = code; t.mode = mode; t.threshold = threshold;
    return t;
}
// An Action carrying one keyboard output, to exercise resolveOutputs().
static Actions action(int type, int code, ev_mode mode) {
    Actions a;
    a.in.tp = joystick;
    (void)trig(type, code, mode);   // trigger isn't stored on the model yet
    devActions out; out.tp = keyboard;
    a.out.push_back(out);
    return a;
}

TEST_SUITE("evmatch") {

    TEST_CASE("resolveSymbol: prefix implies the EV class and code") {
        evTrigger t;
        REQUIRE(resolveSymbol("BTN_SOUTH", t));
        CHECK(t.type == EV_KEY_); CHECK(t.code == 0x130);

        REQUIRE(resolveSymbol("KEY_A", t));
        CHECK(t.type == EV_KEY_); CHECK(t.code == 30);

        REQUIRE(resolveSymbol("ABS_X", t));
        CHECK(t.type == EV_ABS_); CHECK(t.code == 0);

        REQUIRE(resolveSymbol("REL_WHEEL", t));
        CHECK(t.type == EV_REL_); CHECK(t.code == 8);
    }

    TEST_CASE("resolveSymbol: unknown symbol fails and leaves out untouched") {
        evTrigger t; t.type = 99; t.code = 99;
        CHECK_FALSE(resolveSymbol("NOPE_X", t));
        CHECK(t.type == 99); CHECK(t.code == 99);
    }

    TEST_CASE("ABS_X and REL_X share code 0 but differ by type") {
        evTrigger abs; REQUIRE(resolveSymbol("ABS_X", abs));
        evTrigger rel; REQUIRE(resolveSymbol("REL_X", rel));
        CHECK(abs.code == rel.code);
        CHECK(abs.type != rel.type);
    }

    TEST_CASE("key press/release/hold map to evdev values 1/0/2") {
        auto press   = trig(EV_KEY_, 0x130, ev_press);
        auto release = trig(EV_KEY_, 0x130, ev_release);
        auto hold    = trig(EV_KEY_, 0x130, ev_hold);

        CHECK(matches(press,   ev(EV_KEY_, 0x130, 1)));
        CHECK_FALSE(matches(press, ev(EV_KEY_, 0x130, 0)));
        CHECK_FALSE(matches(press, ev(EV_KEY_, 0x130, 2)));

        CHECK(matches(release, ev(EV_KEY_, 0x130, 0)));
        CHECK_FALSE(matches(release, ev(EV_KEY_, 0x130, 1)));

        CHECK(matches(hold,    ev(EV_KEY_, 0x130, 2)));
        CHECK_FALSE(matches(hold, ev(EV_KEY_, 0x130, 1)));
    }

    TEST_CASE("hold and hold_once match identically (both value==2)") {
        auto hold     = trig(EV_KEY_, 0x130, ev_hold);
        auto holdOnce = trig(EV_KEY_, 0x130, ev_hold_once);
        auto e = ev(EV_KEY_, 0x130, 2);
        CHECK(matches(hold, e));
        CHECK(matches(holdOnce, e));   // distinction is holdGen's, not the matcher's
    }

    TEST_CASE("wrong code or wrong type never matches") {
        auto press = trig(EV_KEY_, 0x130, ev_press);
        CHECK_FALSE(matches(press, ev(EV_KEY_, 0x131, 1)));  // different button
        CHECK_FALSE(matches(press, ev(EV_ABS_, 0x130, 1)));  // different type
    }

    TEST_CASE("abs higher/lower compare against threshold") {
        auto higher = trig(EV_ABS_, 0, ev_higher, 200);
        CHECK(matches(higher, ev(EV_ABS_, 0, 201)));
        CHECK_FALSE(matches(higher, ev(EV_ABS_, 0, 200)));   // equal is not higher
        CHECK_FALSE(matches(higher, ev(EV_ABS_, 0, 100)));

        auto lower = trig(EV_ABS_, 0, ev_lower, 200);
        CHECK(matches(lower, ev(EV_ABS_, 0, 100)));
        CHECK_FALSE(matches(lower, ev(EV_ABS_, 0, 200)));    // equal is not lower
        CHECK_FALSE(matches(lower, ev(EV_ABS_, 0, 300)));
    }

    TEST_CASE("spot matches the code regardless of value") {
        auto spot = trig(EV_ABS_, 0, ev_spot);
        CHECK(matches(spot, ev(EV_ABS_, 0, 0)));
        CHECK(matches(spot, ev(EV_ABS_, 0, 255)));
        CHECK_FALSE(matches(spot, ev(EV_ABS_, 1, 0)));       // different axis
    }

    TEST_CASE("nomode never matches") {
        CHECK_FALSE(matches(trig(EV_KEY_, 0x130, ev_nomode), ev(EV_KEY_, 0x130, 1)));
    }

    TEST_CASE("resolveOutputs: spot carries incoming value into output spot") {
        auto a = action(EV_ABS_, 0, ev_spot);
        auto outs = resolveOutputs(a, ev(EV_ABS_, 0, 177), ev_spot);
        REQUIRE(outs.size() == 1);
        CHECK(outs[0].spot == 177);
    }

    TEST_CASE("resolveOutputs: non-spot modes leave spot untouched") {
        auto a = action(EV_KEY_, 0x130, ev_press);
        auto outs = resolveOutputs(a, ev(EV_KEY_, 0x130, 1), ev_press);
        REQUIRE(outs.size() == 1);
        CHECK(outs[0].spot == -1);   // devActions default
    }
}
