// Shared entry-point scaffolding for the per-device handler processes
// (conMIDI / conJoyS / conKeyB / conMouse). Each of those is its own binary
// whose main() was, until now, the same boilerplate: a global stop flag, a
// signal handler that trips it, a spin loop, then Stop() + delete on the
// device handler. That identical part lives here once.
//
// What stays in each device's main() is only what genuinely differs: argument
// parsing and the device-specific discovery/construction. The caller hands
// that work to runDevice() as a factory callable returning the constructed
// handler (or nullptr on usage/construction failure):
//
//   return condev::runDevice([&]{
//       device_list();                          // MIDI-specific discovery
//       return new MIDI(jsonFileName, hw_ports);
//   });
//
// DeviceT only has to expose a Stop() method. There is deliberately NO template
// on the event type here -- each handler is its own process, so the engine never
// needs to vary over the hardware event type (see the design notes); this runner
// is the one piece of main() that is actually common.
#pragma once

#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>

namespace condev {

// Single process-wide termination flag. A C signal handler can't capture state,
// so this is a namespace-scope inline variable (one definition across TUs).
inline std::atomic_bool g_stop{false};
inline void onSignal(int) { g_stop = true; }

// Install termination handlers, build the device via `make`, spin until a
// signal arrives, then tear it down. Returns 1 if construction failed, else 0.
// (SIGKILL is intentionally not registered -- it is uncatchable; the old mains
// "registered" it as a silent no-op.)
template <typename MakeFn>
int runDevice(MakeFn make) {
    g_stop = false;
    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);
    std::signal(SIGQUIT, onSignal);

    auto *dev = make();
    if (!dev)
        return 1;

    while (!g_stop)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    dev->Stop();
    delete dev;
    return 0;
}

} // namespace condev
