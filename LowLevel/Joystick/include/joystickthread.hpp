#ifndef JOYSTICKTHREAD_HPP
#define JOYSTICKTHREAD_HPP

#include "deviceEngine.hpp"
#include "evTypes.hpp"
#include "holdGen.hpp"

#include <string>
#include <thread>
#include <map>

// The joystick handler: an evdev INPUT device built on the shared DeviceEngine.
// It reads /dev/input/eventN, matches each event against the current mode's
// rules with the pure evMatch matcher, and -- because gamepad buttons do not
// autorepeat -- synthesizes value==2 "hold" events via holdGen. Output, backend
// coms and mode switching are all inherited from DeviceEngine; this class adds
// only the evdev input layer (the conMIDI counterpart of MIDI::in_func).
class Joystick : public DeviceEngine {
public:
    Joystick(const std::string &jsonFileName, const std::string &devNode);
    ~Joystick() override;

    // Stop the reader, then the engine (called by condev::runDevice).
    void Stop();

private:
    void in_func();                                          // evdev reader thread
    void runRules(const evmatch::evEvent &e);                // match + enqueue (no report)
    void processEvent(const evmatch::evEvent &e, long now);  // report + holds + rules
    void serviceHolds(long now);                             // emit due synthetic holds
    bool hasHoldRule(int code) const;

    std::string  devNode;
    int          fd = -1;
    std::thread *in_thread = nullptr;

    // holdGen bookkeeping, keyed by button code, for the current mode's
    // hold / hold_once rules.
    std::map<int, holdgen::State> holdState;
};

#endif
