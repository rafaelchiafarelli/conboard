// Dev-only HTTP endpoint for simulating physical HMI events without real
// GPIO hardware wired up, e.g.
//   curl -d '{"control":"hc_button1_press"}' http://board:9090/simulate
// Off by default -- main.cpp only starts it when CONHMI_SIM_PORT is set.
// This is a debug/testing surface, not part of conHMI's normal
// REST-client-only design (see main.cpp's header comment), so it stays
// isolated in its own module and never touches LVGL directly: it only
// queues the raw control name string off the wire, the LVGL-owning thread
// drains and applies it. No new dependency -- hand-rolled over POSIX
// sockets, one route, no keep-alive, deliberately minimal.
#pragma once

#include <string>
#include <vector>

namespace sim_server {

// Starts a background thread listening on `port` for
// POST /simulate {"control":"<hmi_control name>"}. Returns false if the
// socket could not be bound/listened on (port already in use, etc).
bool start(int port);

// Pops every control name queued since the last call, FIFO order. Call
// this only from the thread that owns the LVGL group (the same thread
// running lv_timer_handler) -- this file has no lvgl.h dependency on
// purpose, it just hands back plain strings for the caller to interpret.
std::vector<std::string> drain();

} // namespace sim_server
