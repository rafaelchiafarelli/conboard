// Unit tests for the MIDI action-mapping logic (LowLevel/Common/midiMap.*).
// Pure: synthetic MIDI signals + synthetic Actions, no hardware/ALSA/threads.
#include "doctest.h"          // main() is provided by test_devicedetect.cpp

#include "midiMap.hpp"

using namespace midimap;

// --- tiny builders ----------------------------------------------------------
static midiSignal sig(int b0, int b1, int b2) {
    midiSignal s;
    s.asInt = 0;
    s.byte[0] = (char)b0; s.byte[1] = (char)b1; s.byte[2] = (char)b2;
    return s;
}
static midiActions trig(midi_action_mode mode, int b0, int b1, int b2) {
    midiActions t;
    t.midi_mode = mode;
    t.midi = sig(b0, b1, b2);
    return t;
}
// An Action with one MIDI output, so we can check resolveOutputs() too.
static Actions action(midi_action_mode mode, int b0, int b1, int b2) {
    Actions a;
    a.in.mAct = trig(mode, b0, b1, b2);
    a.in.tp = midi;
    devActions out;
    out.tp = midi;
    a.out.push_back(out);
    return a;
}

TEST_SUITE("midi") {
    TEST_CASE("normal: matches only on exact b0/b1/b2") {
        auto t = trig(midi_normal, 0x90, 10, 100);
        CHECK(matches(t, sig(0x90, 10, 100)));
        CHECK_FALSE(matches(t, sig(0x90, 10, 99)));   // b2 differs
        CHECK_FALSE(matches(t, sig(0x90, 11, 100)));  // b1 differs
        CHECK_FALSE(matches(t, sig(0x80, 10, 100)));  // b0 differs
    }

    TEST_CASE("trigger_higher: fires when incoming b2 rises above threshold") {
        auto t = trig(midi_trigger_higher, 0xB0, 7, 64);
        CHECK(matches(t, sig(0xB0, 7, 65)));
        CHECK_FALSE(matches(t, sig(0xB0, 7, 64)));    // equal is not higher
        CHECK_FALSE(matches(t, sig(0xB0, 7, 10)));    // lower
    }

    TEST_CASE("trigger_lower: fires when incoming b2 falls below threshold") {
        auto t = trig(midi_trigger_lower, 0xB0, 7, 64);
        CHECK(matches(t, sig(0xB0, 7, 10)));
        CHECK_FALSE(matches(t, sig(0xB0, 7, 64)));    // equal is not lower
        CHECK_FALSE(matches(t, sig(0xB0, 7, 100)));   // higher
    }

    // The regression this feature fixes: spot must NOT require b2 to match, and
    // must NOT secretly behave like blink (the old code fell through).
    TEST_CASE("spot: matches on b0/b1 regardless of b2") {
        auto t = trig(midi_spot, 0x90, 42, 0);
        CHECK(matches(t, sig(0x90, 42, 0)));
        CHECK(matches(t, sig(0x90, 42, 127)));        // different b2 still matches
        CHECK_FALSE(matches(t, sig(0x90, 43, 0)));    // b1 differs -> no match
    }

    TEST_CASE("blink: still requires exact b0/b1/b2 (distinct from spot)") {
        auto t = trig(midi_blink, 0x90, 42, 100);
        CHECK(matches(t, sig(0x90, 42, 100)));
        CHECK_FALSE(matches(t, sig(0x90, 42, 99)));   // unlike spot, b2 matters
    }

    TEST_CASE("nomode never matches") {
        CHECK_FALSE(matches(trig(midi_nomode, 0x90, 1, 1), sig(0x90, 1, 1)));
    }

    TEST_CASE("resolveOutputs: spot/blink carry incoming b2 into output spot") {
        auto spotAct  = action(midi_spot,  0x90, 42, 0);
        auto outs = resolveOutputs(spotAct, sig(0x90, 42, 77));
        REQUIRE(outs.size() == 1);
        CHECK(outs[0].spot == 77);

        auto blinkAct = action(midi_blink, 0x90, 42, 5);
        CHECK(resolveOutputs(blinkAct, sig(0x90, 42, 5))[0].spot == 5);
    }

    TEST_CASE("resolveOutputs: normal mode leaves spot untouched") {
        auto normalAct = action(midi_normal, 0x90, 42, 100);
        CHECK(resolveOutputs(normalAct, sig(0x90, 42, 100))[0].spot == -1); // devActions default
    }

    // --- SysEx: exact byte-for-byte match only, matchesSysex() not matches() ---

    TEST_CASE("matches() never matches a midi_sysex trigger, even with a lookalike b0/b1/b2") {
        midiActions t;
        t.midi_mode = midi_sysex;
        t.sysex = {0xF0, 0x7E, 0x7F, 0xF7};
        CHECK_FALSE(matches(t, sig(0, 0, 0)));
    }

    TEST_CASE("matchesSysex: exact match required, any byte difference fails") {
        midiActions t;
        t.midi_mode = midi_sysex;
        t.sysex = {0xF0, 0x43, 0x12, 0x00, 0xF7};

        CHECK(matchesSysex(t, {0xF0, 0x43, 0x12, 0x00, 0xF7}));
        CHECK_FALSE(matchesSysex(t, {0xF0, 0x43, 0x12, 0x01, 0xF7}));  // one byte differs
        CHECK_FALSE(matchesSysex(t, {0xF0, 0x43, 0x12, 0x00}));       // missing terminator
        CHECK_FALSE(matchesSysex(t, {0xF0, 0x43, 0x12, 0x00, 0xF7, 0x00})); // extra trailing byte
    }

    TEST_CASE("matchesSysex: a non-sysex trigger never matches, regardless of payload") {
        midiActions t;
        t.midi_mode = midi_normal;
        t.sysex = {0xF0, 0x43, 0xF7};   // stray data, should be irrelevant
        CHECK_FALSE(matchesSysex(t, {0xF0, 0x43, 0xF7}));
    }

    TEST_CASE("resolveOutputs: sysex mode leaves spot untouched, like normal") {
        Actions a;
        a.in.mAct.midi_mode = midi_sysex;
        a.in.mAct.sysex = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};
        a.in.tp = midi;
        devActions out;
        out.tp = midi;
        a.out.push_back(out);

        auto outs = resolveOutputs(a, midiSignal{});
        REQUIRE(outs.size() == 1);
        CHECK(outs[0].spot == -1);
    }
}
