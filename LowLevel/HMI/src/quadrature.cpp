#include "quadrature.hpp"

namespace quadrature {

namespace {
// Indexed by (oldPhase << 2 | newPhase); +1/-1 for a valid single-bit gray-code
// step in each direction, 0 for a repeat or an invalid (skipped/bounced) edge.
// Standard mechanical-encoder decode table.
const int8_t kTransitionTable[16] = {
    0, -1,  1,  0,
    1,  0,  0, -1,
   -1,  0,  0,  1,
    0,  1, -1,  0,
};
}  // namespace

Step update(State &s, bool a, bool b)
{
    uint8_t newPhase = (a ? 0b10 : 0) | (b ? 0b01 : 0);
    int8_t delta = kTransitionTable[(s.phase << 2) | newPhase];
    s.phase = newPhase;
    if (delta == 0)
        return Step::None;

    s.accum += delta;
    if (s.accum >= 4) {
        s.accum = 0;
        return Step::Clockwise;
    }
    if (s.accum <= -4) {
        s.accum = 0;
        return Step::CounterClockwise;
    }
    return Step::None;
}

}  // namespace quadrature
