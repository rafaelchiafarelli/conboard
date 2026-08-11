// Unit tests for the push-button debounce state machine (LowLevel/HMI/debounce.*).
// Pure: a synthetic millisecond clock is passed in, no real timers/threads.
#include "doctest.h"          // main() is provided by test_devicedetect.cpp

#include "debounce.hpp"

using namespace debounce;

TEST_SUITE("debounce") {

    TEST_CASE("idle: no reading changes, nothing fires") {
        State s;
        Config cfg{20};
        CHECK(update(s, false, 0, cfg) == Edge::None);
        CHECK(update(s, false, 1000, cfg) == Edge::None);
    }

    TEST_CASE("a level held long enough settles into Pressed") {
        State s;
        Config cfg{20};
        CHECK(update(s, true, 0, cfg) == Edge::None);    // raw changes, timer starts
        CHECK(update(s, true, 10, cfg) == Edge::None);   // still settling
        CHECK(update(s, true, 19, cfg) == Edge::None);
        CHECK(update(s, true, 20, cfg) == Edge::Pressed); // settleMs elapsed
        CHECK(update(s, true, 21, cfg) == Edge::None);    // already stable
    }

    TEST_CASE("a bounce shorter than settleMs never fires") {
        State s;
        Config cfg{20};
        CHECK(update(s, true, 0, cfg) == Edge::None);
        CHECK(update(s, true, 10, cfg) == Edge::None);
        CHECK(update(s, false, 15, cfg) == Edge::None);  // bounced back before settling
        CHECK(update(s, false, 34, cfg) == Edge::None);  // 19ms since the bounce -- not yet
        CHECK(s.stable == false);                        // never left the original level
    }

    TEST_CASE("press then release, each debounced independently") {
        State s;
        Config cfg{20};
        CHECK(update(s, true, 0, cfg) == Edge::None);
        CHECK(update(s, true, 20, cfg) == Edge::Pressed);
        CHECK(update(s, false, 100, cfg) == Edge::None);
        CHECK(update(s, false, 119, cfg) == Edge::None);
        CHECK(update(s, false, 120, cfg) == Edge::Released);
    }

    TEST_CASE("default Config settleMs is used when none is supplied") {
        State s;
        CHECK(update(s, true, 0) == Edge::None);
        CHECK(update(s, true, 19) == Edge::None);
        CHECK(update(s, true, 20) == Edge::Pressed);   // default settleMs == 20
    }
}
