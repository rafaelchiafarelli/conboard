#ifndef EVDEVDEVICE_HPP
#define EVDEVDEVICE_HPP

#include "deviceEngine.hpp"
#include "evTypes.hpp"
#include "holdGen.hpp"
#include "actions.h"     // devType

#include <string>
#include <thread>
#include <map>

// Generic evdev INPUT device on top of DeviceEngine. It reads /dev/input/eventN,
// matches each event against the current mode's rules with the pure evMatch
// matcher, and synthesizes hold events via holdGen. ONE engine serves all three
// evdev handlers -- conJoyS (joystick), conKeyB (keyboard) and conMouse (mouse);
// they differ only in which rule type they match (inputType) and which node they
// bind to (classifierMatch). The per-device mains are thin: construct one of
// these with the right type.
//
// Hold model: the kernel's key autorepeat (EV_KEY value==2) is DROPPED and
// holdGen synthesizes ALL hold events uniformly, so `hold` (repeat at the rule's
// interval) and `hold_once` (one fire after the rule's delay) behave identically
// and correctly on every device type, independent of the kernel repeat rate. The
// matcher stays pure -- it only ever compares a single event.
class EvdevDevice : public DeviceEngine {
public:
    // inputType: the rule.in.tp this device matches (joystick/keyboard/mouse).
    // classifierMatch: substring of an InputDevice.type used to self-discover the
    //   node (e.g. "keyboard", "mouse", "joystick").
    // devNode: explicit node (optional). usbDevpath: bind to this physical port
    //   (from the launcher; separates identical devices), optional.
    EvdevDevice(devType inputType, const std::string &classifierMatch,
                const std::string &jsonFileName, const std::string &devNode,
                const std::string &usbDevpath = "");
    ~EvdevDevice() override;

    void Stop();

private:
    std::string resolveNode();
    void in_func();                                          // evdev reader thread
    void runRules(const evmatch::evEvent &e);                // match + enqueue (no report)
    void processEvent(const evmatch::evEvent &e, long now);  // report + holds + rules
    void serviceHolds(long now);                             // emit due synthetic holds
    bool hasHoldRule(int code) const;

    devType      inputType;
    std::string  classifierMatch;
    std::string  usbDevpath;
    std::string  devNode;
    int          fd = -1;
    std::thread *in_thread = nullptr;

    std::map<int, holdgen::State> holdState;
};

#endif
