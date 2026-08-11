// Unit tests for the rotary-encoder quadrature decoder (LowLevel/HMI/quadrature.*).
// Pure: synthetic (a, b) pin readings, no gpiod, no threads.
#include "doctest.h"          // main() is provided by test_devicedetect.cpp

#include "quadrature.hpp"

using namespace quadrature;

namespace {
// One clean clockwise quadrature cycle: 00 -> 10 -> 11 -> 01 -> 00.
void feedClockwiseCycle(State &s) {
    update(s, true, false);   // 10
    update(s, true, true);    // 11
    update(s, false, true);   // 01
    update(s, false, false);  // 00
}
void feedCounterClockwiseCycle(State &s) {
    update(s, false, true);   // 01
    update(s, true, true);    // 11
    update(s, true, false);   // 10
    update(s, false, false);  // 00
}
}  // namespace

TEST_SUITE("quadrature") {

    TEST_CASE("no edges: idle stays None") {
        State s;
        CHECK(update(s, false, false) == Step::None);
    }

    TEST_CASE("one full clockwise detent -> exactly one Clockwise step") {
        State s;
        CHECK(update(s, true, false) == Step::None);
        CHECK(update(s, true, true)  == Step::None);
        CHECK(update(s, false, true) == Step::None);
        CHECK(update(s, false, false) == Step::Clockwise);
    }

    TEST_CASE("one full counter-clockwise detent -> exactly one CounterClockwise step") {
        State s;
        CHECK(update(s, false, true) == Step::None);
        CHECK(update(s, true, true)  == Step::None);
        CHECK(update(s, true, false) == Step::None);
        CHECK(update(s, false, false) == Step::CounterClockwise);
    }

    TEST_CASE("helper cycles agree with the inline sequences above") {
        State cw, ccw;
        feedClockwiseCycle(cw);
        CHECK(cw.accum == 0);       // completed cycle resets the accumulator
        feedCounterClockwiseCycle(ccw);
        CHECK(ccw.accum == 0);
    }

    TEST_CASE("repeated clockwise cycles keep firing one step each") {
        State s;
        feedClockwiseCycle(s);
        int8_t prevAccum = s.accum;
        CHECK(prevAccum == 0);

        CHECK(update(s, true, false) == Step::None);
        CHECK(update(s, true, true)  == Step::None);
        CHECK(update(s, false, true) == Step::None);
        CHECK(update(s, false, false) == Step::Clockwise);
    }

    TEST_CASE("a skipped (invalid) phase transition never itself reports a step") {
        State s;
        update(s, true, false);      // valid edge, phase 10
        // Jump straight to phase 01 (both bits flipped) -- not a valid single-bit
        // gray-code step; a debounce glitch, not a real edge. Must not resolve to
        // a step on its own (regardless of how the accumulator is affected).
        CHECK(update(s, false, true) == Step::None);
    }

    TEST_CASE("reversing direction mid-cycle does not fire a spurious step") {
        State s;
        update(s, true, false);   // 10 (accum +1)
        update(s, false, false);  // back to 00 (accum -1) -> net 0
        CHECK(s.accum == 0);
    }
}
