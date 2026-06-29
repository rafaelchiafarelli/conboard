#include "joystickthread.hpp"
#include "evMatch.hpp"
#include "deviceDetect.hpp"   // condetect::scanInputDevices for node self-discovery

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

std::string Joystick::resolveNode() {
    std::vector<condetect::InputDevice> inputs = condetect::scanInputDevices();
    // 1) match the profile's declared input name (EVIOCGNAME)
    if (!json.DevInput.empty())
        for (std::vector<condetect::InputDevice>::iterator d = inputs.begin(); d != inputs.end(); ++d)
            if (d->name == json.DevInput)
                return d->node;
    // 2) fall back to the first node classified as a joystick/gamepad
    for (std::vector<condetect::InputDevice>::iterator d = inputs.begin(); d != inputs.end(); ++d)
        if (d->type.find("joystick") != std::string::npos)
            return d->node;
    return "";
}

Joystick::Joystick(const std::string &jsonFileName, const std::string &devNode_)
    : DeviceEngine(jsonFileName), devNode(devNode_)
{
    if (devNode.empty())
        devNode = resolveNode();   // self-discover from the profile (launcher passes no -p)

    if (!devNode.empty())
        fd = open(devNode.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0)
        std::cout << "joystick: no usable evdev node ('" << devNode << "') -- reader not started" << std::endl;
    else
        std::cout << "joystick: reading " << devNode << std::endl;

    startEngine();   // output + coms threads, run header, activate initial mode

    if (fd >= 0)
        in_thread = new std::thread(&Joystick::in_func, this);
}

Joystick::~Joystick() { Stop(); }

void Joystick::Stop() {
    stop = true;                                  // shared flag: stops reader + engine loops
    if (in_thread) { in_thread->join(); delete in_thread; in_thread = nullptr; }
    if (fd >= 0)   { close(fd); fd = -1; }
    stopEngine();
}

bool Joystick::hasHoldRule(int code) const {
    for (std::vector<Actions>::const_iterator a = CurrentMode.body_actions.begin();
         a != CurrentMode.body_actions.end(); ++a) {
        if (a->in.tp == joystick &&
            (a->in.evtrig.mode == evmatch::ev_hold || a->in.evtrig.mode == evmatch::ev_hold_once) &&
            a->in.evtrig.code == code)
            return true;
    }
    return false;
}

void Joystick::runRules(const evEvent &e) {
    for (std::vector<Actions>::iterator a = CurrentMode.body_actions.begin();
         a != CurrentMode.body_actions.end(); ++a) {
        if (a->in.tp != joystick) continue;       // only this device's input rules
        if (evmatch::matches(a->in.evtrig, e)) {
            enqueue(evmatch::resolveOutputs(*a, e, a->in.evtrig.mode));
            if (a->change_mode && a->change_to != -1)
                changeMode(a->change_to);
        }
    }
}

void Joystick::processEvent(const evEvent &e, long now) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "[%d,%d,%d]", e.type, e.code, e.value);
    report(buf);

    // Track hold state for buttons that have a hold rule in the current mode.
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

void Joystick::serviceHolds(long now) {
    for (std::vector<Actions>::iterator a = CurrentMode.body_actions.begin();
         a != CurrentMode.body_actions.end(); ++a) {
        if (a->in.tp != joystick) continue;
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

void Joystick::in_func() {
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
                    if (iev[i].type == EV_SYN) continue;   // packet boundary
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
