// Device inventory -- GET <base>/devices.
//
// Lists the USB devices currently attached and, for each, (a) classifies its type
// using the SAME condetect classifier the launcher uses, and (b) reports whether a
// boards/*.json profile already matches it (its required header.identifier.tags are
// all present in the device's udev properties -- the same predicate the launcher's
// deviceMatchesTags uses). The console's add-device flow shows the UNDESIGNATED
// devices so the user can name/type them into a new profile.
//
// Hand-written (not harpia-generated), like deploy.cpp. Runs as root (backend.service)
// so it can read /dev/input/* and the full udev property set. Uses libudev to
// enumerate + read ID_* properties, and links the shared condetect library for the
// classifier so the inventory can never disagree with the runtime.
#include "conboard_entities.h"

#include "deviceDetect.hpp"   // condetect::probeInput / isActionableInterfaces

#include <libudev.h>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace conboard {
namespace {

const char* const HASH = "7fb7af9d1e69abe2d7a6e81c4a2d0c2f";

std::string env_or(const char* k, const std::string& d) {
    const char* v = std::getenv(k);
    return (v && *v) ? std::string(v) : d;
}

std::string read_file(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// condetect's evdev type strings -> the profile DEVICE.type vocabulary. Empty when
// the device is not one of the four handled kinds.
std::string norm_type(const std::string& t) {
    if (t.find("joystick") != std::string::npos || t.find("gamepad") != std::string::npos) return "joystick";
    if (t == "keyboard") return "keyboard";
    if (t == "mouse") return "mouse";
    return "";
}

// ID_USB_INTERFACES is ":CCSSPP:CCSSPP:" (class/subclass/protocol hex triplets).
// A MIDIStreaming interface is class 01 / subclass 03.
bool has_midi_iface(const std::string& ifaces) {
    for (size_t i = 0; i + 6 <= ifaces.size(); ++i) {
        if (ifaces[i] != ':') continue;
        if (i + 7 > ifaces.size() || ifaces[i + 7] != ':') continue;
        const std::string t = ifaces.substr(i + 1, 6);   // "CCSSPP"
        if (t.size() == 6 && t.compare(0, 2, "01") == 0 && t.compare(2, 2, "03") == 0) return true;
    }
    return false;
}

// A board's required match tags (header.identifier.tags), keyed by filename.
struct Profile { std::string file; std::map<std::string, std::string> tags; };

std::vector<Profile> load_profiles(const std::string& dir) {
    std::vector<Profile> out;
    DIR* d = opendir(dir.c_str());
    if (!d) return out;
    for (dirent* e; (e = readdir(d)) != nullptr; ) {
        const std::string fn = e->d_name;
        if (fn.size() < 6 || fn.substr(fn.size() - 5) != ".json") continue;
        rapidjson::Document doc;
        if (doc.Parse(read_file(dir + "/" + fn).c_str()).HasParseError() || !doc.IsObject()) continue;
        Profile p; p.file = fn;
        if (doc.HasMember("header") && doc["header"].IsObject()) {
            const auto& h = doc["header"];
            if (h.HasMember("identifier") && h["identifier"].IsObject()) {
                const auto& id = h["identifier"];
                if (id.HasMember("tags") && id["tags"].IsObject()) {
                    for (auto it = id["tags"].MemberBegin(); it != id["tags"].MemberEnd(); ++it) {
                        if (it->value.IsString()) p.tags[it->name.GetString()] = it->value.GetString();
                    }
                }
            }
        }
        out.push_back(std::move(p));
    }
    closedir(d);
    return out;
}

// The first profile whose (non-empty) tags are all present+equal in the device's
// udev properties -- i.e. the profile the launcher would bind. Empty if none.
std::string matched_profile(const std::vector<Profile>& profiles,
                            const std::map<std::string, std::string>& props) {
    for (const auto& p : profiles) {
        if (p.tags.empty()) continue;
        bool all = true;
        for (const auto& kv : p.tags) {
            auto it = props.find(kv.first);
            if (it == props.end() || it->second != kv.second) { all = false; break; }
        }
        if (all) return p.file;
    }
    return "";
}

std::string prop(const std::map<std::string, std::string>& m, const char* k) {
    auto it = m.find(k);
    return it == m.end() ? std::string() : it->second;
}

// Enumerate a subsystem and map each device's parent usb_device syspath -> a value
// produced by fn(dev). Used to attach input/sound classifications to their USB parent.
using DevFn = std::string (*)(udev_device*);

void map_by_usb_parent(udev* u, const char* subsystem, DevFn fn,
                       std::map<std::string, std::string>& out) {
    udev_enumerate* en = udev_enumerate_new(u);
    if (!en) return;
    udev_enumerate_add_match_subsystem(en, subsystem);
    udev_enumerate_scan_devices(en);
    for (udev_list_entry* le = udev_enumerate_get_list_entry(en); le;
         le = udev_list_entry_get_next(le)) {
        udev_device* dev = udev_device_new_from_syspath(u, udev_list_entry_get_name(le));
        if (!dev) continue;
        udev_device* usb = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (usb) {
            const char* psys = udev_device_get_syspath(usb);
            if (psys) {
                std::string v = fn(dev);
                if (!v.empty() && out.find(psys) == out.end()) out[psys] = v;
            }
        }
        udev_device_unref(dev);   // does not free the borrowed parent
    }
    udev_enumerate_unref(en);
}

// Classify an input device via the shared condetect probe (opens its /dev/input node).
// Only the evdev event* nodes carry the capability bits probeInput reads; js*/mouse*
// siblings under the same input device have no usable node, so skip them.
std::string classify_input(udev_device* dev) {
    const char* node = udev_device_get_devnode(dev);
    if (!node) return "";
    const std::string n = node;
    if (n.rfind("/dev/input/event", 0) != 0) return "";
    return norm_type(condetect::probeInput(n).type);
}

// Any sound device under a USB parent -> treat the parent as MIDI-capable.
std::string mark_midi(udev_device*) { return "midi"; }

void fail(crow::response& res, int code, const std::string& msg) {
    res.code = code;
    res.set_header("Content-Type", "application/json");
    res.body = std::string("{\"error\":\"") + msg + "\"}";
    res.end();
}

}  // namespace

void register_devices(crow::SimpleApp& app, const std::string& base) {
    app.route_dynamic(base + "/devices").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req, crow::response& res) {
            if (req.get_header_value("X-Pswd") != HASH) { res.code = 401; res.end(); return; }

            const std::string dir = env_or("CONBOARD_BOARDS_DIR", "/conboard/boards");
            const std::vector<Profile> profiles = load_profiles(dir);

            udev* u = udev_new();
            if (!u) { fail(res, 500, "udev unavailable"); return; }

            // Input + sound classifications, keyed by their parent usb_device syspath.
            std::map<std::string, std::string> type_by_usb;   // input evdev -> type
            map_by_usb_parent(u, "input", classify_input, type_by_usb);
            std::map<std::string, std::string> midi_by_usb;
            map_by_usb_parent(u, "sound", mark_midi, midi_by_usb);

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartArray();

            udev_enumerate* en = udev_enumerate_new(u);
            udev_enumerate_add_match_subsystem(en, "usb");
            udev_enumerate_scan_devices(en);
            for (udev_list_entry* le = udev_enumerate_get_list_entry(en); le;
                 le = udev_list_entry_get_next(le)) {
                udev_device* dev = udev_device_new_from_syspath(u, udev_list_entry_get_name(le));
                if (!dev) continue;
                const char* dt = udev_device_get_devtype(dev);
                if (!dt || std::strcmp(dt, "usb_device") != 0) { udev_device_unref(dev); continue; }

                const char* sys = udev_device_get_syspath(dev);
                std::map<std::string, std::string> props;
                for (udev_list_entry* pe = udev_device_get_properties_list_entry(dev); pe;
                     pe = udev_list_entry_get_next(pe)) {
                    const char* v = udev_list_entry_get_value(pe);
                    props[udev_list_entry_get_name(pe)] = v ? v : "";
                }

                // Type: evdev classification wins; else a MIDI/audio interface; else none.
                std::string type;
                if (sys) {
                    auto it = type_by_usb.find(sys);
                    if (it != type_by_usb.end()) type = it->second;
                    if (type.empty() && midi_by_usb.count(sys)) type = "midi";
                }
                if (type.empty() && has_midi_iface(prop(props, "ID_USB_INTERFACES"))) type = "midi";

                const bool actionable =
                    !type.empty() || condetect::isActionableInterfaces(prop(props, "ID_USB_INTERFACES"));
                if (!actionable) { udev_device_unref(dev); continue; }   // skip hubs/root

                // Human name: ID_MODEL with underscores as spaces, else serial.
                std::string name = prop(props, "ID_MODEL");
                for (char& c : name) if (c == '_') c = ' ';
                if (name.empty()) name = prop(props, "ID_SERIAL_SHORT");
                if (name.empty()) name = "USB device";

                const std::string profile = matched_profile(profiles, props);

                w.StartObject();
                w.Key("name");       w.String(name.c_str());
                w.Key("type");       w.String(type.c_str());   // "" when unclassified-but-actionable
                w.Key("vid");        w.String(prop(props, "ID_VENDOR_ID").c_str());
                w.Key("pid");        w.String(prop(props, "ID_MODEL_ID").c_str());
                w.Key("serial");     w.String(prop(props, "ID_SERIAL_SHORT").c_str());
                w.Key("devpath");    w.String(prop(props, "DEVPATH").c_str());
                w.Key("designated"); w.Bool(!profile.empty());
                w.Key("profile");    w.String(profile.c_str());
                // Candidate identity tags for building a new profile (the stable subset).
                w.Key("tags"); w.StartObject();
                for (const char* k : {"ID_MODEL", "ID_VENDOR_ID", "ID_MODEL_ID", "ID_BUS", "ID_SERIAL_SHORT"}) {
                    const std::string v = prop(props, k);
                    if (!v.empty()) { w.Key(k); w.String(v.c_str()); }
                }
                w.EndObject();
                w.EndObject();

                udev_device_unref(dev);
            }
            udev_enumerate_unref(en);
            udev_unref(u);

            w.EndArray();
            res.set_header("Content-Type", "application/json");
            res.body = sb.GetString();
            res.code = 200;
            res.end();
        });
}

}  // namespace conboard
