#pragma once

// INTERFACE.md O2: envelope version token, shared by every delimited wire leg
// that carries structured payload -- io and heartbeat (both directions) on
// ZMQ, and the dispatcher's WS output frames. Frozen at v0 (INTERFACE.md
// section 4, item 3): only one version exists today, this is the seam a
// future v1 would key off of.
//
// Deliberately NOT used on the registration leg (dispatcher::th_unique_number
// / zmq_coms::unique_number_handler) -- that leg is raw, undelimited bytes by
// design (INTERFACE.md S2.1), and retrofitting a version field there means
// changing its wire format, which is a separate decision (see NEXT-SESSION.md
// dispatcher devname-corruption fix, which depended on that raw-bytes shape).
#define CONBOARD_ENVELOPE_VERSION "v0"
