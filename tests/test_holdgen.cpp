// Unit tests for the hold-event generator (LowLevel/Common/holdGen.*).
// Pure: a synthetic millisecond clock is passed in, no real timers/threads.
#include "doctest.h"          // main() is provided by test_devicedetect.cpp

#include "holdGen.hpp"

using namespace holdgen;

TEST_SUITE("holdgen") {

    TEST_CASE("not held: never due") {
        State s;                       // down == false
        Config cfg{hold_repeat, 100};
        CHECK_FALSE(due(s, cfg, 0));
        CHECK_FALSE(due(s, cfg, 1000));
    }

    TEST_CASE("repeat: first fire one interval after press, then every interval") {
        State s; Config cfg{hold_repeat, 100};
        press(s, 0);
        CHECK_FALSE(due(s, cfg, 50));    // before first interval
        CHECK_FALSE(due(s, cfg, 99));
        CHECK(due(s, cfg, 100));         // first fire
        CHECK_FALSE(due(s, cfg, 150));   // 50 since last fire
        CHECK(due(s, cfg, 200));         // second fire
        CHECK(due(s, cfg, 320));         // >= interval since last fire (200)
    }

    TEST_CASE("repeat: release stops firing; re-press restarts the cycle") {
        State s; Config cfg{hold_repeat, 100};
        press(s, 0);
        CHECK(due(s, cfg, 100));
        release(s);
        CHECK_FALSE(due(s, cfg, 500));   // released: nothing fires
        press(s, 1000);
        CHECK_FALSE(due(s, cfg, 1050));  // fresh cycle
        CHECK(due(s, cfg, 1100));
    }

    TEST_CASE("once: fires exactly once after the delay, then never again") {
        State s; Config cfg{hold_once, 500};
        press(s, 0);
        CHECK_FALSE(due(s, cfg, 100));
        CHECK_FALSE(due(s, cfg, 499));
        CHECK(due(s, cfg, 500));         // the single long-press fire
        CHECK_FALSE(due(s, cfg, 501));   // already fired
        CHECK_FALSE(due(s, cfg, 5000));
    }

    TEST_CASE("once: a fire at the first poll past the delay (not missed)") {
        State s; Config cfg{hold_once, 500};
        press(s, 0);
        CHECK(due(s, cfg, 800));         // first poll happened to be late -> still fires
        CHECK_FALSE(due(s, cfg, 900));
    }

    TEST_CASE("once: release before delay -> never fires; re-press re-arms") {
        State s; Config cfg{hold_once, 500};
        press(s, 0);
        release(s);
        CHECK_FALSE(due(s, cfg, 600));   // released before delay
        press(s, 1000);
        CHECK_FALSE(due(s, cfg, 1100));
        CHECK(due(s, cfg, 1500));        // re-armed, fires once
    }
}
