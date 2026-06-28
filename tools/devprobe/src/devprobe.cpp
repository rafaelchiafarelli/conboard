// devprobe — portable device classifier for conboard (diagnostic front-end).
//
// All the classification logic lives in the shared, board-agnostic detector
// (LowLevel/Common/deviceDetect.{hpp,cpp}) that the launcher also uses, so this
// tool and the launcher's runtime detection can never disagree. devprobe just
// prints the results in human-readable form.
//
// Build (standalone, no dependencies — compiles the shared source directly).
// See this tool's README for the exact g++ one-liner, or use CMake
// (cmake -S . -B build && cmake --build build).
//
// Run:
//     sudo ./devprobe              # scan USB interfaces + all /dev/input/event*
//     sudo ./devprobe /dev/input/event3   # probe specific evdev node(s)

#include "deviceDetect.hpp"

#include <cstdio>

static void print_input(const condetect::InputDevice &d) {
    if (d.name.empty()) {
        printf("  %s: cannot open (try sudo)\n", d.node.c_str());
        return;
    }
    printf("  %s — \"%s\"\n", d.node.c_str(), d.name.c_str());
    printf("      type        : %s\n", d.type.c_str());
    printf("      capabilities: %d keys/buttons, %d abs axes, %d rel axes\n",
           d.keys, d.absAxes, d.relAxes);
}

int main(int argc, char **argv) {
    printf("devprobe — conboard portable device classifier (kernel ABIs only)\n\n");

    if (argc > 1) {
        printf("== evdev input devices ==\n");
        for (int i = 1; i < argc; i++) print_input(condetect::probeInput(argv[i]));
        return 0;
    }

    printf("== USB interfaces (from sysfs descriptors) ==\n");
    auto ifaces = condetect::scanUsbInterfaces();
    if (ifaces.empty()) {
        printf("  (none found / sysfs unavailable)\n");
    } else {
        for (const auto &u : ifaces)
            printf("  %-10s %-30s [%04x:%04x] %s %s\n",
                   u.iface.c_str(), u.className.c_str(), u.vid, u.pid,
                   u.manufacturer.c_str(), u.product.c_str());
    }
    printf("\n");

    printf("== evdev input devices ==\n");
    auto inputs = condetect::scanInputDevices();
    if (inputs.empty()) printf("  (none found / no permission)\n");
    else for (const auto &d : inputs) print_input(d);
    printf("\n");
    return 0;
}
