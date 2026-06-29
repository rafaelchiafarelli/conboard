// conJoyS -- the joystick (evdev) handler process. A thin main over the shared
// EvdevDevice; the launcher spawns it for a gamepad with the device's json
// profile and (for separation) its USB port via -d.
#include "evdevDevice.hpp"
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
        {"port",    1, NULL, 'p'},   // explicit evdev node (optional)
        {"devpath", 1, NULL, 'd'},   // USB DEVPATH of the physical port (launcher sets this)
        { }
    };

    if (argc < 2) {
        cout << "usage: ./conJoyS -x \"/path/file.json\" [-d <usb-devpath>] [-p /dev/input/eventN]" << endl;
        return -1;
    }

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

    return condev::runDevice([&]{
        return new EvdevDevice(joystick, "joystick", jsonFileName, devNode, usbDevpath);
    });
}
