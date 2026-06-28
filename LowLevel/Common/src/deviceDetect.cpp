// Implementation of the shared, board-agnostic device classifier.
// Kernel headers + std only — no libusb, no ALSA, no zmq — so it compiles into
// libcommon and also stands alone for tools/devprobe.
#include "deviceDetect.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <algorithm>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>

namespace condetect {

namespace {

std::string read_trim(const std::string &path) {
    std::ifstream f(path);
    if (!f) return "";
    std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' ||
                          s.back() == ' '  || s.back() == '\0'))
        s.pop_back();
    return s;
}

int hex_file(const std::string &path) {
    std::string v = read_trim(path);
    if (v.empty()) return -1;
    return (int)strtol(v.c_str(), nullptr, 16);
}

constexpr int kLongBits = (int)(sizeof(unsigned long) * 8);
constexpr int nlongs(int bits) { return (bits + kLongBits - 1) / kLongBits; }

inline int test_bit(int bit, const unsigned long *arr) {
    return (arr[bit / kLongBits] >> (bit % kLongBits)) & 1UL;
}
int count_bits(const unsigned long *arr, int max) {
    int n = 0;
    for (int i = 0; i < max; i++) n += test_bit(i, arr);
    return n;
}

// Mirrors the kernel/udev input_id classification heuristic.
std::string classify(const unsigned long *ev, const unsigned long *key,
                     const unsigned long *abs, const unsigned long *rel) {
    bool is_mouse = test_bit(EV_REL, ev) && test_bit(REL_X, rel) &&
                    test_bit(REL_Y, rel) && test_bit(BTN_LEFT, key);

    bool is_joystick = test_bit(BTN_JOYSTICK, key) || test_bit(BTN_GAMEPAD, key) ||
                       (test_bit(EV_ABS, ev) && test_bit(ABS_X, abs) &&
                        test_bit(ABS_Y, abs) &&
                        (test_bit(BTN_TRIGGER, key) || test_bit(BTN_THUMB, key)));

    bool is_keyboard = test_bit(KEY_A, key) && test_bit(KEY_Z, key) &&
                       test_bit(KEY_SPACE, key);

    if (is_keyboard) return "keyboard";
    if (is_mouse)    return "mouse";
    if (is_joystick) return "joystick / gamepad";
    if (test_bit(EV_KEY, ev)) return "buttons / HID (other)";
    return "unknown";
}

} // namespace

std::string usbClassName(int cls, int sub, int proto) {
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

std::vector<UsbInterface> scanUsbInterfaces() {
    std::vector<UsbInterface> out;
    const char *root = "/sys/bus/usb/devices";
    DIR *d = opendir(root);
    if (!d) return out;

    std::vector<std::string> names;
    for (dirent *e; (e = readdir(d));) {
        std::string n = e->d_name;
        if (n.find(':') != std::string::npos) names.push_back(n);  // interface dirs
    }
    closedir(d);
    std::sort(names.begin(), names.end());

    for (const auto &iface : names) {
        std::string ipath = std::string(root) + "/" + iface;
        int cls = hex_file(ipath + "/bInterfaceClass");
        if (cls < 0) continue;

        UsbInterface u;
        u.iface = iface;
        u.cls   = cls;
        u.sub   = hex_file(ipath + "/bInterfaceSubClass");
        u.proto = hex_file(ipath + "/bInterfaceProtocol");
        u.className = usbClassName(u.cls, u.sub, u.proto);

        std::string parent = iface.substr(0, iface.find(':'));
        std::string ppath  = std::string(root) + "/" + parent;
        std::string vid = read_trim(ppath + "/idVendor");
        std::string pid = read_trim(ppath + "/idProduct");
        u.vid = vid.empty() ? 0 : (int)strtol(vid.c_str(), nullptr, 16);
        u.pid = pid.empty() ? 0 : (int)strtol(pid.c_str(), nullptr, 16);
        u.manufacturer = read_trim(ppath + "/manufacturer");
        u.product      = read_trim(ppath + "/product");
        out.push_back(std::move(u));
    }
    return out;
}

InputDevice probeInput(const std::string &node) {
    InputDevice dev;
    dev.node = node;

    int fd = open(node.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) return dev;  // name stays empty => caller knows it failed

    unsigned long ev[nlongs(EV_MAX)]   = {0};
    unsigned long key[nlongs(KEY_MAX)] = {0};
    unsigned long abs[nlongs(ABS_MAX)] = {0};
    unsigned long rel[nlongs(REL_MAX)] = {0};

    char nm[256] = {0};
    if (ioctl(fd, EVIOCGNAME(sizeof(nm) - 1), nm) >= 0) dev.name = nm;
    ioctl(fd, EVIOCGBIT(0,      sizeof(ev)),  ev);
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key)), key);
    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs)), abs);
    ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel)), rel);
    close(fd);

    dev.type    = classify(ev, key, abs, rel);
    dev.keys    = count_bits(key, KEY_MAX);
    dev.absAxes = count_bits(abs, ABS_MAX);
    dev.relAxes = count_bits(rel, REL_MAX);
    return dev;
}

std::vector<InputDevice> scanInputDevices() {
    std::vector<InputDevice> out;
    const char *root = "/dev/input";
    DIR *d = opendir(root);
    if (!d) return out;

    std::vector<std::string> nodes;
    for (dirent *e; (e = readdir(d));) {
        std::string n = e->d_name;
        if (n.rfind("event", 0) == 0) nodes.push_back(std::string(root) + "/" + n);
    }
    closedir(d);
    std::sort(nodes.begin(), nodes.end());

    for (const auto &n : nodes) out.push_back(probeInput(n));
    return out;
}

} // namespace condetect
