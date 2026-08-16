// HMI data endpoints -- GET <base>/hmi/{console-url,wifi/networks,activation,
// radio/stations}.
//
// This is the ENTIRE data surface LowLevel/HMI (conHMI, the screen/buttons/
// encoders local UI) talks to: that module deliberately contains no nmcli
// shell-outs, no network-state reads, no activation logic of its own -- it is
// a thin REST client + renderer, and every piece of domain data it shows
// comes from here (see the plan doc's architecture constraints). All routes
// are read-only for this pass; a WiFi-connect action is a later, mutating
// addition once phase 4 needs it.
//
// Hand-written (not harpia-generated), like devices.cpp/deploy.cpp. Runs as
// root (backend.service), which is what lets console-url read network
// interfaces and wifi/networks shell out to nmcli.
#include "conboard_entities.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace conboard {
namespace {

const char* const HASH = "9f20d5d43738774941f9898b22cf2cf2";

void sendJson(crow::response& res, const rapidjson::StringBuffer& sb) {
    res.set_header("Content-Type", "application/json");
    res.body = std::string(sb.GetString(), sb.GetSize());
    res.end();
}

// The board's own LAN URL: first non-loopback IPv4 interface, nginx's port
// (the console/API are served on :80, see backend/assets/interface.conf).
// Returns "" if no such interface is up (e.g. not yet connected to any LAN).
std::string laneUrl() {
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0) return "";

    std::string url;
    for (struct ifaddrs* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;
        if (!(ifa->ifa_flags & IFF_UP)) continue;

        char buf[INET_ADDRSTRLEN] = {};
        auto* sin = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
        if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
            url = std::string("http://") + buf + "/";
            break;
        }
    }
    freeifaddrs(ifaddr);
    return url;
}

// Runs a shell command and returns its stdout. Used only for the fixed,
// argument-free nmcli invocation below -- no user input is ever interpolated
// into the command string.
std::string runCommand(const char* cmd) {
    std::array<char, 256> chunk{};
    std::string out;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return out;
    size_t n;
    while ((n = fread(chunk.data(), 1, chunk.size(), pipe)) > 0)
        out.append(chunk.data(), n);
    pclose(pipe);
    return out;
}

// nmcli terse output is ':'-separated, e.g. "MyWifi:80:WPA2". Doesn't unescape
// nmcli's own '\:' escaping of literal colons in SSIDs -- acceptable for a
// first read-only scan; a real WiFi-connect flow (later, mutating) would need
// nmcli's -t -x-safe machine parsing instead.
std::vector<std::vector<std::string>> parseTerseLines(const std::string& text) {
    std::vector<std::vector<std::string>> rows;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        std::vector<std::string> fields;
        std::istringstream ls(line);
        std::string field;
        while (std::getline(ls, field, ':'))
            fields.push_back(field);
        rows.push_back(std::move(fields));
    }
    return rows;
}

}  // namespace

void register_hmi(crow::SimpleApp& app, const std::string& base) {
    app.route_dynamic(base + "/hmi/console-url").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req, crow::response& res) {
            if (req.get_header_value("X-Pswd") != HASH) { res.code = 401; res.end(); return; }

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("url"); w.String(laneUrl().c_str());
            w.EndObject();
            sendJson(res, sb);
        });

    app.route_dynamic(base + "/hmi/wifi/networks").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req, crow::response& res) {
            if (req.get_header_value("X-Pswd") != HASH) { res.code = 401; res.end(); return; }

            const std::string out = runCommand("nmcli -t -f SSID,SIGNAL,SECURITY dev wifi list 2>/dev/null");

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartArray();
            for (const auto& fields : parseTerseLines(out)) {
                if (fields.empty() || fields[0].empty()) continue;   // hidden/blank SSID
                w.StartObject();
                w.Key("ssid");     w.String(fields[0].c_str());
                w.Key("signal");   w.String(fields.size() > 1 ? fields[1].c_str() : "");
                w.Key("security"); w.String(fields.size() > 2 ? fields[2].c_str() : "");
                w.EndObject();
            }
            w.EndArray();
            sendJson(res, sb);
        });

    app.route_dynamic(base + "/hmi/activation").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req, crow::response& res) {
            if (req.get_header_value("X-Pswd") != HASH) { res.code = 401; res.end(); return; }

            // Documented STUB contract, not the real feature: the power-password
            // login flow (backend/README.md) is unimplemented on the backend
            // today. This gives a future activation screen a stable shape to
            // build against without promising security behavior that isn't built.
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("activated");      w.Bool(false);
            w.Key("power_password"); w.Null();
            w.Key("failed_attempts"); w.Int(0);
            w.EndObject();
            sendJson(res, sb);
        });

    app.route_dynamic(base + "/hmi/radio/stations").methods(crow::HTTPMethod::GET)(
        [](const crow::request& req, crow::response& res) {
            if (req.get_header_value("X-Pswd") != HASH) { res.code = 401; res.end(); return; }

            // Stub: the station source is explicitly TBD (see the plan doc).
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartArray();
            w.EndArray();
            sendJson(res, sb);
        });
}

}  // namespace conboard
