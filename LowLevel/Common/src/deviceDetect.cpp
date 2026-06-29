// Implementation of the shared, board-agnostic device classifier.
// Kernel headers + std only — no libusb, no ALSA, no zmq — so it compiles into
// libcommon and also stands alone for tools/devprobe.
#include "deviceDetect.hpp"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
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

bool has(const std::vector<int> &v, int code) {
    return std::find(v.begin(), v.end(), code) != v.end();
}

} // namespace

// Mirrors the kernel/udev input_id classification heuristic. Pure: depends only
// on its argument, so it is unit-tested directly with synthetic EvdevBits.
std::string classifyEvdev(const EvdevBits &c) {
    bool is_mouse = has(c.types, EV_REL) && has(c.relAxes, REL_X) &&
                    has(c.relAxes, REL_Y) && has(c.keys, BTN_LEFT);

    bool is_joystick = has(c.keys, BTN_JOYSTICK) || has(c.keys, BTN_GAMEPAD) ||
                       (has(c.types, EV_ABS) && has(c.absAxes, ABS_X) &&
                        has(c.absAxes, ABS_Y) &&
                        (has(c.keys, BTN_TRIGGER) || has(c.keys, BTN_THUMB)));

    bool is_keyboard = has(c.keys, KEY_A) && has(c.keys, KEY_Z) &&
                       has(c.keys, KEY_SPACE);

    if (is_keyboard) return "keyboard";
    if (is_mouse)    return "mouse";
    if (is_joystick) return "joystick / gamepad";
    if (has(c.types, EV_KEY)) return "buttons / HID (other)";
    return "unknown";
}

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

bool isActionableInterfaces(const std::string &ifaces) {
    // Walk colon-separated 6-hex-digit triplets (CCSSPP); the class is the
    // first two hex digits. Actionable = HID (0x03) or Audio/MIDI (0x01).
    size_t i = 0;
    while (i < ifaces.size()) {
        if (ifaces[i] == ':') { i++; continue; }
        size_t end = ifaces.find(':', i);
        std::string tok = ifaces.substr(i, (end == std::string::npos ? ifaces.size() : end) - i);
        if (tok.size() >= 2) {
            int cls = (int) strtol(tok.substr(0, 2).c_str(), nullptr, 16);
            if (cls == 0x01 || cls == 0x03)
                return true;
        }
        i = (end == std::string::npos) ? ifaces.size() : end;
    }
    return false;
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

bool nodeUnderUsbPath(const std::string &nodeSysfsPath, const std::string &usbDevpath) {
    if (nodeSysfsPath.empty() || usbDevpath.empty()) return false;
    std::string::size_type p = nodeSysfsPath.find(usbDevpath);
    if (p == std::string::npos) return false;
    // Require the match to end at a path boundary so a port like "1-1.2" does
    // NOT match a deeper port "1-1.2.3" (usbDevpath's own leading '/' anchors
    // the start).
    std::string::size_type end = p + usbDevpath.size();
    return end == nodeSysfsPath.size() || nodeSysfsPath[end] == '/';
}

static std::string sanitizeToken(const std::string &s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); i++)
        if (!isalnum((unsigned char)r[i]) && r[i] != '-') r[i] = '_';  // keep '-' (port ids, valid in unit names)
    return r;
}

bool isTrustworthySerial(const std::string &serial) {
    if (serial.empty()) return false;
    // Trustworthy iff it has at least one char that isn't a placeholder 'F'/'0'.
    for (size_t i = 0; i < serial.size(); i++) {
        char c = serial[i];
        if (c != 'F' && c != 'f' && c != '0') return true;
    }
    return false;
}

std::string portToken(const std::string &usbDevpath) {
    if (usbDevpath.empty()) return "";
    return sanitizeToken(usbDevpath.substr(usbDevpath.find_last_of('/') + 1));
}

std::string deviceIdentity(const std::string &serial, const std::string &usbDevpath) {
    if (isTrustworthySerial(serial)) return "ser-" + sanitizeToken(serial);
    return "port-" + portToken(usbDevpath);
}

InputDevice probeInput(const std::string &node) {
    InputDevice dev;
    dev.node = node;

    // Resolve the node's /sys path (symlink target); it embeds the USB DEVPATH,
    // which lets callers bind a handler to a specific physical port.
    {
        std::string base = node.substr(node.find_last_of('/') + 1);
        std::string classPath = "/sys/class/input/" + base;
        char resolved[4096];
        if (realpath(classPath.c_str(), resolved)) dev.sysfsPath = resolved;
    }

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

    // Turn the raw ioctl bitmaps into plain code sets, then classify (the same
    // pure function the tests exercise).
    EvdevBits caps;
    if (test_bit(EV_KEY, ev)) caps.types.push_back(EV_KEY);
    if (test_bit(EV_REL, ev)) caps.types.push_back(EV_REL);
    if (test_bit(EV_ABS, ev)) caps.types.push_back(EV_ABS);
    for (int i = 0; i < KEY_MAX; i++) if (test_bit(i, key)) caps.keys.push_back(i);
    for (int i = 0; i < ABS_MAX; i++) if (test_bit(i, abs)) caps.absAxes.push_back(i);
    for (int i = 0; i < REL_MAX; i++) if (test_bit(i, rel)) caps.relAxes.push_back(i);

    dev.type    = classifyEvdev(caps);
    dev.keys    = (int)caps.keys.size();
    dev.absAxes = (int)caps.absAxes.size();
    dev.relAxes = (int)caps.relAxes.size();
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
