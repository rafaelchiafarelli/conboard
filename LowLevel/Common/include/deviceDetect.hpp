// Shared, board-agnostic device detection for conboard.
//
// Classifies attached devices by *type* and *capabilities* using only Linux
// kernel ABIs (USB sysfs + evdev ioctls), so it behaves identically on any
// board/distro (Orange Pi, Raspberry Pi, x86…). See NOTES.md (ADR-1, ADR-3).
//
// VID/PID is exposed for *identity* only (matching a profile in boards/*.json);
// it is never used to infer capability.
//
// Used by both the launcher (runtime) and tools/devprobe (diagnostic), so the
// thing you debug with and the thing the launcher runs share one implementation.
#pragma once

#include <string>
#include <vector>

namespace condetect {

// One USB interface as the kernel parsed it from the descriptors.
struct UsbInterface {
    std::string iface;        // sysfs name, e.g. "1-1:1.0"
    int cls = -1, sub = -1, proto = -1;
    std::string className;    // human-readable (e.g. "Audio / MIDIStreaming (MIDI)")
    int vid = 0, pid = 0;     // identity only
    std::string manufacturer, product;
};

// One input device as classified from its evdev capability bitmaps.
struct InputDevice {
    std::string node;         // /dev/input/eventN
    std::string name;         // EVIOCGNAME
    std::string type;         // "keyboard" | "mouse" | "joystick / gamepad" | ...
    int keys = 0, absAxes = 0, relAxes = 0;
};

// Map a USB class/subclass/protocol triplet to a human-readable name.
std::string usbClassName(int cls, int sub, int proto);

// Scan /sys/bus/usb/devices for interfaces (empty if sysfs is unavailable).
std::vector<UsbInterface> scanUsbInterfaces();

// Scan /dev/input/event* and classify each (empty if none / no permission).
std::vector<InputDevice> scanInputDevices();

// Probe a single evdev node (node.name empty if it cannot be opened).
InputDevice probeInput(const std::string &node);

} // namespace condetect
