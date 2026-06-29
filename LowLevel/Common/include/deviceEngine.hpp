// Generic per-device orchestration shared by all conboard handlers
// (conMIDI, conJoyS, conKeyB, conMouse). It owns everything that does NOT depend
// on the hardware-event type:
//   - the OUTPUT side: a queue + an executor thread that emits keyboard/mouse/
//     joystick HID reports (via oActions) and device-native feedback,
//   - the backend COMS: the zmq heartbeat/reporting loop (reload/file commands),
//   - the MODE model and mode switching.
//
// A device subclass adds only its INPUT layer: a reader thread that reads the
// hardware, runs its own pure matcher (midiMap / evMatch), and feeds results
// back through report() + enqueue() + changeMode(). The raw hardware-event type
// therefore never crosses into this base (see the design notes) -- which is why
// this is a plain base, not a template.
//
// Lives in Common and needs zmq but NOT ALSA (ALSA is purely MIDI's reader), so
// it builds as part of libcommon.
#pragma once

#include "actions.h"
#include "jsonParser.h"
#include "zmq_coms.hpp"
#include "oActions.hpp"

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <string>
#include <fstream>

class DeviceEngine : protected oActions {
public:
    explicit DeviceEngine(const std::string &jsonFileName);
    virtual ~DeviceEngine();

    // Backend control loop (runs on its own thread): reacts to reload / file /
    // outstop commands from the dispatcher. Shared across devices.
    void coms_handler();

    void Reload();
    bool outFile(const std::string &name);
    void outStop();

protected:
    // ---- lifecycle (the subclass ctor/dtor drives these) ----
    // Activate the initial mode, run the header, then start the output + coms
    // threads. The subclass starts its OWN input thread after this returns.
    void startEngine();
    // Signal stop, then join + tear down the engine threads and coms. The
    // subclass must stop + join its input thread BEFORE calling this.
    void stopEngine();

    // ---- API the subclass's reader thread calls ----
    // Report an input event to the backend/UI. Returns false if the reporting
    // queue overflowed (the event is dropped from the report only; the action
    // path below is unaffected).
    bool report(const std::string &msg);
    // Queue a resolved set of outputs for the executor thread.
    void enqueue(std::vector<devActions> outs);
    // Switch to the mode whose index == id (no-op if not found).
    void changeMode(int id);

    // ---- device-specific hook ----
    // Emit one "native" feedback output back to the device itself (e.g. MIDI
    // LED). Default: nothing. Keyboard/mouse/joystick outputs are HID and handled
    // generically; only the device's own protocol output needs this override.
    virtual void emitNative(const devActions &out) {}

    // Run a set of header/mode-header actions (startup + on mode change).
    void runActions(const std::vector<Actions> &actions);

    // ---- shared state (protected: the reader reads CurrentMode) ----
    std::atomic_bool   stop{false};
    std::mutex         locking_mechanism;

    std::vector<Actions>   header;
    std::vector<ModeType>  modes;
    ModeType               CurrentMode;
    unsigned int           SelectedMode = 0;

    jsonParser   json;
    std::string  jsonFileName;

private:
    void out_func();                       // output executor thread body
    void executeOutput(devActions &out);   // dispatch one output by its type

    std::queue<std::vector<devActions>> oQueue;
    std::atomic_bool                    send{false};

    zmq_coms     *com        = nullptr;
    std::thread  *out_thread = nullptr;
    std::thread  *thcoms     = nullptr;

    bool          outToFile = false;
    std::string   outFileName;
    std::ofstream outFileStream;
};
