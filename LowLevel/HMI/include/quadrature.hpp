// Pure quadrature decode for a mechanical rotary encoder's A/B pins. Two
// adjacent GPIO edges (a, b) form a 2-bit "phase"; consecutive valid phases in
// a gray-code sequence differ by exactly one bit, so an invalid (skipped)
// transition is a debounce glitch and is ignored rather than mis-counted.
//
// Pure and hardware-free (no gpiod, no threads) so it is unit-tested with
// synthetic pin readings, mirroring holdGen's design in LowLevel/Common.
// rotary_encoder.cpp is the thin libgpiod wrapper that feeds real pin edges
// into update().
#pragma once

#include <cstdint>

namespace quadrature {

struct State {
    uint8_t phase = 0;   // last-seen 2-bit phase (bit1=A, bit0=B)
    int8_t  accum = 0;   // sub-detent step accumulator
};

enum class Step { None, Clockwise, CounterClockwise };

// Feed one new (a, b) pin reading. Most mechanical encoders produce 4
// quadrature edges per physical detent, so a full Step is only returned once
// accum has crossed a full cycle; every other edge (including an invalid,
// skipped-phase one) returns None.
Step update(State &s, bool a, bool b);

} // namespace quadrature
