#include "evdevDevice.hpp"
#include "evMatch.hpp"
#include "deviceDetect.hpp"   // condetect::scanInputDevices / nodeUnderUsbPath

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#include <chrono>
#include <cstdio>
#include <iostream>

using evmatch::evEvent;

// Monotonic millisecond clock for holdGen (steady, unaffected by wall-clock).
static long nowMs() {
    using namespace std::chrono;
    return (long) duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

std::string EvdevDevice::resolveNode() {
    std::vector<condetect::InputDevice> inputs = condetect::scanInputDevices();
    // 1) the device UNDER the bound physical port (separates identical units)
    if (!usbDevpath.empty())
        for (std::vector<condetect::InputDevice>::iterator d = inputs.begin(); d != inputs.end(); ++d)
            if (d->type.find(classifierMatch) != std::string::npos &&
                condetect::nodeUnderUsbPath(d->sysfsPath, usbDevpath))
                return d->node;
    // 2) the node whose name matches the profile's declared input (EVIOCGNAME)
    if (!json.DevInput.empty())
        for (std::vector<condetect::InputDevice>::iterator d = inputs.begin(); d != inputs.end(); ++d)
            if (d->name == json.DevInput)
                return d->node;
    // 3) the first node of this device's class
    for (std::vector<condetect::InputDevice>::iterator d = inputs.begin(); d != inputs.end(); ++d)
        if (d->type.find(classifierMatch) != std::string::npos)
            return d->node;
    return "";
}

EvdevDevice::EvdevDevice(devType inputType_, const std::string &classifierMatch_,
                         const std::string &jsonFileName, const std::string &devNode_,
                         const std::string &usbDevpath_)
    : DeviceEngine(jsonFileName),
      inputType(inputType_), classifierMatch(classifierMatch_),
      usbDevpath(usbDevpath_), devNode(devNode_)
{
    if (devNode.empty())
        devNode = resolveNode();

    if (!devNode.empty())
        fd = open(devNode.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
        std::cout << "evdev: no usable node ('" << devNode << "') -- reader not started" << std::endl;
    else
        std::cout << "evdev: reading " << devNode << std::endl;

    startEngine();

    if (fd >= 0)
        in_thread = new std::thread(&EvdevDevice::in_func, this);
}

EvdevDevice::~EvdevDevice() { Stop(); }

void EvdevDevice::Stop() {
    stop = true;
    if (in_thread) { in_thread->join(); delete in_thread; in_thread = nullptr; }
    if (fd >= 0)   { close(fd); fd = -1; }
    stopEngine();
}

bool EvdevDevice::hasHoldRule(int code) const {
    for (std::vector<Actions>::const_iterator a = CurrentMode.body_actions.begin();
         a != CurrentMode.body_actions.end(); ++a) {
        if (a->in.tp == inputType &&
            (a->in.evtrig.mode == evmatch::ev_hold || a->in.evtrig.mode == evmatch::ev_hold_once) &&
            a->in.evtrig.code == code)
            return true;
    }
    return false;
}

void EvdevDevice::runRules(const evEvent &e) {
    for (std::vector<Actions>::iterator a = CurrentMode.body_actions.begin();
         a != CurrentMode.body_actions.end(); ++a) {
        if (a->in.tp != inputType) continue;     // only this device's input rules
        if (evmatch::matches(a->in.evtrig, e)) {
            enqueue(evmatch::resolveOutputs(*a, e, a->in.evtrig.mode));
            if (a->change_mode && a->change_to != -1)
                changeMode(a->change_to);
        }
    }
}

void EvdevDevice::processEvent(const evEvent &e, long now) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "[%d,%d,%d]", e.type, e.code, e.value);
    report(buf);

    // Track hold state for keys/buttons that have a hold rule in this mode.
    if (e.type == evmatch::EV_KEY_) {
        if (e.value == 1 && hasHoldRule(e.code))
            holdgen::press(holdState[e.code], now);
        else if (e.value == 0) {
            std::map<int, holdgen::State>::iterator it = holdState.find(e.code);
            if (it != holdState.end())
                holdgen::release(it->second);
        }
    }

    runRules(e);
}

void EvdevDevice::serviceHolds(long now) {
    for (std::vector<Actions>::iterator a = CurrentMode.body_actions.begin();
         a != CurrentMode.body_actions.end(); ++a) {
        if (a->in.tp != inputType) continue;
        evmatch::ev_mode m = a->in.evtrig.mode;
        if (m != evmatch::ev_hold && m != evmatch::ev_hold_once) continue;

        int code = a->in.evtrig.code;
        std::map<int, holdgen::State>::iterator it = holdState.find(code);
        if (it == holdState.end()) continue;

        holdgen::Config cfg{ m == evmatch::ev_hold ? holdgen::hold_repeat : holdgen::hold_once,
                             a->in.evtrig.holdMs };
        if (holdgen::due(it->second, cfg, now)) {
            evEvent syn;
            syn.type = evmatch::EV_KEY_;
            syn.code = code;
            syn.value = 2;
            runRules(syn);     // synthetic hold: enqueue outputs, but do NOT report it
        }
    }
}

void EvdevDevice::in_func() {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    while (!stop) {
        int pr = poll(&pfd, 1, 10);   // 10ms tick so holds are serviced even when idle
        long now = nowMs();

        if (pr > 0 && (pfd.revents & POLLIN)) {
            struct input_event iev[64];
            ssize_t n = read(fd, iev, sizeof(iev));
            if (n > 0) {
                size_t count = (size_t) n / sizeof(struct input_event);
                for (size_t i = 0; i < count; ++i) {
                    if (iev[i].type == EV_SYN) continue;                 // packet boundary
                    if (iev[i].type == EV_KEY && iev[i].value == 2)      // kernel autorepeat:
                        continue;                                        // dropped -- holdGen owns holds
                    evEvent e;
                    e.type  = iev[i].type;
                    e.code  = iev[i].code;
                    e.value = iev[i].value;
                    processEvent(e, now);
                }
            }
        }
        serviceHolds(now);
    }
}
