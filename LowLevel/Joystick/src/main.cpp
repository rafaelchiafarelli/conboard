// conJoyS -- the joystick (evdev) handler process. Launched by the launcher for
// a gamepad/joystick, given the device's json profile and its /dev/input node.
#include "joystickthread.hpp"
#include "runDevice.hpp"

#include <getopt.h>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
    static const char short_options[] = "x:p:d:";
    static const struct option long_options[] = {
        {"json",    1, NULL, 'x'},
        {"port",    1, NULL, 'p'},   // explicit evdev node, e.g. /dev/input/event5
        {"devpath", 1, NULL, 'd'},   // USB DEVPATH of the physical port (launcher sets this)
        { }
    };

    if (argc < 2) {
        cout << "usage: ./conJoyS -x \"/path/file.json\" [-d <usb-devpath>] [-p /dev/input/eventN]" << endl;
        return -1;
    }

    // -x (json) is required. -p (explicit node) and -d (bind to a USB port) are
    // optional; with neither, the handler self-discovers a gamepad node like
    // conMIDI finds its ALSA port. The launcher passes -d so two identical pads
    // on different ports stay separate.
    string jsonFileName, devNode, usbDevpath;
    int c;
    while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (c) {
            case 'x': jsonFileName = optarg; break;
            case 'p': devNode      = optarg; break;
            case 'd': usbDevpath   = optarg; break;
            default:  cout << "Try more information." << endl; return 1;
        }
    }

    if (jsonFileName.empty()) {
        cout << "usage: ./conJoyS -x \"/path/file.json\" [-d <usb-devpath>] [-p /dev/input/eventN]" << endl;
        return -1;
    }

    // Shared scaffolding (signals, spin loop, Stop/delete); device-specific
    // construction is the factory.
    return condev::runDevice([&]{ return new Joystick(jsonFileName, devNode, usbDevpath); });
}
