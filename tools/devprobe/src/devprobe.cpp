// devprobe — portable device classifier for conboard.
//
// Prints the *type* and *capabilities* of attached input/USB devices using only
// Linux kernel ABIs (evdev ioctls + USB sysfs). Nothing here is specific to
// Orange Pi, Allwinner, or any distro, so the same binary classifies devices
// identically on a Zero 3, a Raspberry Pi, or an x86 box. (See NOTES.md ADR-3.)
//
// This is the foundation for the launcher's device detection: it deliberately
// does NOT look at VID/PID to decide capability (VID/PID is identity only) — it
// reads what the kernel already parsed from the USB/HID descriptors.
//
// Build (standalone, no dependencies):
//     g++ -std=c++17 -O2 src/devprobe.cpp -o devprobe
// Run:
//     sudo ./devprobe              # scan USB interfaces + all /dev/input/event*
//     sudo ./devprobe /dev/input/event3   # probe specific evdev node(s)
//
// Scope of this increment: USB interface class (incl. MIDI at the descriptor
// level) + evdev (keyboard / mouse / joystick). ALSA rawmidi port enumeration
// is a planned follow-up increment.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>

// ----- small helpers --------------------------------------------------------

static std::string read_trim(const std::string &path) {
    std::ifstream f(path);
    if (!f) return "";
    std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' ||
                          s.back() == ' '  || s.back() == '\0'))
        s.pop_back();
    return s;
}

#define LONG_BITS (sizeof(unsigned long) * 8)
#define NLONGS(x) (((x) + LONG_BITS - 1) / LONG_BITS)

static inline int test_bit(int bit, const unsigned long *arr) {
    return (arr[bit / LONG_BITS] >> (bit % LONG_BITS)) & 1UL;
}
static int count_bits(const unsigned long *arr, int max) {
    int n = 0;
    for (int i = 0; i < max; i++) n += test_bit(i, arr);
    return n;
}

// ----- USB interface scan (sysfs, no libusb) --------------------------------

static std::string usb_class_name(int cls, int sub, int proto) {
    char buf[48];
    switch (cls) {
        case 0x01: return (sub == 0x03) ? "Audio / MIDIStreaming (MIDI)" : "Audio";
        case 0x02: return "Communications (CDC)";
        case 0x03:
            if (proto == 0x01) return "HID / keyboard";
            if (proto == 0x02) return "HID / mouse";
            return "HID";
        case 0x08: return "Mass storage";
        case 0x09: return "Hub";
        case 0x0a: return "CDC data";
        case 0x0b: return "Smart card";
        case 0xe0: return "Wireless";
        case 0xff: return "Vendor-specific";
        default:
            snprintf(buf, sizeof(buf), "class 0x%02x", cls);
            return buf;
    }
}

static int hex_file(const std::string &path) {
    std::string v = read_trim(path);
    if (v.empty()) return -1;
    return (int)strtol(v.c_str(), nullptr, 16);
}

static void scan_usb() {
    printf("== USB interfaces (from sysfs descriptors) ==\n");
    const char *root = "/sys/bus/usb/devices";
    DIR *d = opendir(root);
    if (!d) { printf("  (cannot open %s)\n\n", root); return; }

    std::vector<std::string> entries;
    for (dirent *e; (e = readdir(d));) {
        std::string n = e->d_name;
        // Interface directories look like "1-1:1.0" (contain a ':').
        if (n.find(':') != std::string::npos) entries.push_back(n);
    }
    closedir(d);
    std::sort(entries.begin(), entries.end());

    if (entries.empty()) { printf("  (no USB interfaces found)\n\n"); return; }

    for (const auto &iface : entries) {
        std::string ipath = std::string(root) + "/" + iface;
        int cls   = hex_file(ipath + "/bInterfaceClass");
        int sub   = hex_file(ipath + "/bInterfaceSubClass");
        int proto = hex_file(ipath + "/bInterfaceProtocol");
        if (cls < 0) continue;

        // Parent device = interface name up to the ':'.
        std::string parent = iface.substr(0, iface.find(':'));
        std::string ppath  = std::string(root) + "/" + parent;
        std::string vid   = read_trim(ppath + "/idVendor");
        std::string pid   = read_trim(ppath + "/idProduct");
        std::string man   = read_trim(ppath + "/manufacturer");
        std::string prod  = read_trim(ppath + "/product");

        printf("  %-10s %-30s [%04x:%04x] %s %s\n",
               iface.c_str(),
               usb_class_name(cls, sub, proto).c_str(),
               vid.empty() ? 0 : (int)strtol(vid.c_str(), nullptr, 16),
               pid.empty() ? 0 : (int)strtol(pid.c_str(), nullptr, 16),
               man.c_str(), prod.c_str());
    }
    printf("\n");
}

// ----- evdev scan (input subsystem) -----------------------------------------

struct EvCaps {
    std::string name;
    unsigned long ev[NLONGS(EV_MAX)]   = {0};
    unsigned long key[NLONGS(KEY_MAX)] = {0};
    unsigned long abs[NLONGS(ABS_MAX)] = {0};
    unsigned long rel[NLONGS(REL_MAX)] = {0};
};

// Mirrors the kernel/udev input_id classification heuristic.
static std::string classify(const EvCaps &c) {
    bool has_rel = test_bit(EV_REL, c.ev);
    bool has_abs = test_bit(EV_ABS, c.ev);

    bool is_mouse = has_rel && test_bit(REL_X, c.rel) && test_bit(REL_Y, c.rel) &&
                    test_bit(BTN_LEFT, c.key);

    bool is_joystick = test_bit(BTN_JOYSTICK, c.key) || test_bit(BTN_GAMEPAD, c.key) ||
                       (has_abs && test_bit(ABS_X, c.abs) && test_bit(ABS_Y, c.abs) &&
                        (test_bit(BTN_TRIGGER, c.key) || test_bit(BTN_THUMB, c.key)));

    // Keyboard heuristic: has the alphabetic keys + space.
    bool is_keyboard = test_bit(KEY_A, c.key) && test_bit(KEY_Z, c.key) &&
                       test_bit(KEY_SPACE, c.key);

    if (is_keyboard) return "keyboard";
    if (is_mouse)    return "mouse";
    if (is_joystick) return "joystick / gamepad";
    if (test_bit(EV_KEY, c.ev)) return "buttons / HID (other)";
    return "unknown";
}

static bool read_caps(const std::string &node, EvCaps &c) {
    int fd = open(node.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) return false;
    char nm[256] = {0};
    if (ioctl(fd, EVIOCGNAME(sizeof(nm) - 1), nm) >= 0) c.name = nm;
    ioctl(fd, EVIOCGBIT(0,      sizeof(c.ev)),  c.ev);
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(c.key)), c.key);
    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(c.abs)), c.abs);
    ioctl(fd, EVIOCGBIT(EV_REL, sizeof(c.rel)), c.rel);
    close(fd);
    return true;
}

static void report_evdev(const std::string &node) {
    EvCaps c;
    if (!read_caps(node, c)) {
        printf("  %s: cannot open (try sudo)\n", node.c_str());
        return;
    }
    printf("  %s — \"%s\"\n", node.c_str(), c.name.c_str());
    printf("      type        : %s\n", classify(c).c_str());
    printf("      capabilities: %d keys/buttons, %d abs axes, %d rel axes\n",
           count_bits(c.key, KEY_MAX),
           count_bits(c.abs, ABS_MAX),
           count_bits(c.rel, REL_MAX));
}

static void scan_evdev() {
    printf("== evdev input devices ==\n");
    const char *root = "/dev/input";
    DIR *d = opendir(root);
    if (!d) { printf("  (cannot open %s)\n\n", root); return; }

    std::vector<std::string> nodes;
    for (dirent *e; (e = readdir(d));) {
        std::string n = e->d_name;
        if (n.rfind("event", 0) == 0) nodes.push_back(std::string(root) + "/" + n);
    }
    closedir(d);
    std::sort(nodes.begin(), nodes.end());

    if (nodes.empty()) { printf("  (no /dev/input/event* nodes)\n\n"); return; }
    for (const auto &n : nodes) report_evdev(n);
    printf("\n");
}

// ----- main -----------------------------------------------------------------

int main(int argc, char **argv) {
    printf("devprobe — conboard portable device classifier (kernel ABIs only)\n\n");

    if (argc > 1) {
        // Specific evdev node(s) given on the command line.
        printf("== evdev input devices ==\n");
        for (int i = 1; i < argc; i++) report_evdev(argv[i]);
        return 0;
    }

    scan_usb();
    scan_evdev();
    return 0;
}
