// Axis C -- deploy an authored board profile to the device's realtime path.
//
// The rules DB is the authoring LIBRARY; the device's realtime handlers run from
// /conboard/boards/*.json, NOT from the DB (see memory conboard-rules-db-architecture).
// This endpoint bridges the two: it takes an edited profile (the boards/*.json shape,
// which the frontend already produces from its Board model) and:
//   1. writes it into the on-device boards dir -- OVERWRITING the file whose
//      header.identifier.tags match, because the launcher matches a device to a
//      profile by those tags (LowLevel/Common/src/launcherMatch.cpp), not by filename,
//      so a second file with the same tags would double-match; and
//   2. triggers the same udev coldplug replay the installer uses, so the launcher
//      restarts that device's handler with the new profile.
//
// It is hand-written (not harpia-generated): harpia emits JSON CRUD, not a file/reload
// action. Runs as root (backend.service), so it can write the dir + drive udev.
#include "conboard_entities.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace conboard {
namespace {

const char* const HASH = "69421342752d53d5f274b99a6a0c123e";

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

// Order-independent signature of header.identifier.tags, for matching the existing
// profile file. Empty if the board declares no tags.
std::string tags_sig(const rapidjson::Value& board) {
    std::vector<std::string> kv;
    if (board.IsObject() && board.HasMember("header") && board["header"].IsObject()) {
        const auto& h = board["header"];
        if (h.HasMember("identifier") && h["identifier"].IsObject()) {
            const auto& id = h["identifier"];
            if (id.HasMember("tags") && id["tags"].IsObject()) {
                for (auto it = id["tags"].MemberBegin(); it != id["tags"].MemberEnd(); ++it) {
                    if (it->value.IsString())
                        kv.emplace_back(std::string(it->name.GetString()) + "=" + it->value.GetString());
                }
            }
        }
    }
    std::sort(kv.begin(), kv.end());
    std::string s;
    for (const auto& e : kv) { s += e; s += ';'; }
    return s;
}

std::string sanitize(const std::string& name) {
    std::string s;
    for (char c : name) s += std::isalnum(static_cast<unsigned char>(c)) ? c : '_';
    return s.empty() ? std::string("profile") : s;
}

void fail(crow::response& res, int code, const std::string& msg) {
    res.code = code;
    res.set_header("Content-Type", "application/json");
    res.body = std::string("{\"error\":\"") + msg + "\"}";
    res.end();
}

}  // namespace

void register_deploy(crow::SimpleApp& app, const std::string& base) {
    app.route_dynamic(base + "/deploy").methods(crow::HTTPMethod::POST)(
        [](const crow::request& req, crow::response& res) {
            if (req.get_header_value("X-Pswd") != HASH) { res.code = 401; res.end(); return; }

            rapidjson::Document doc;
            if (doc.Parse(req.body.c_str()).HasParseError() || !doc.IsObject() ||
                !doc.HasMember("DEVICE") || !doc["DEVICE"].IsObject() ||
                !doc["DEVICE"].HasMember("name") || !doc["DEVICE"]["name"].IsString()) {
                fail(res, 400, "expected a board JSON with DEVICE.name");
                return;
            }

            const std::string dir  = env_or("CONBOARD_BOARDS_DIR", "/conboard/boards");
            const std::string name = doc["DEVICE"]["name"].GetString();
            const std::string sig  = tags_sig(doc);

            // Prefer overwriting the existing profile with matching tags.
            std::string target;
            if (DIR* d = opendir(dir.c_str())) {
                for (dirent* e; (e = readdir(d)) != nullptr; ) {
                    const std::string fn = e->d_name;
                    if (fn.size() < 6 || fn.substr(fn.size() - 5) != ".json") continue;
                    const std::string path = dir + "/" + fn;
                    rapidjson::Document cand;
                    if (cand.Parse(read_file(path).c_str()).HasParseError() || !cand.IsObject()) continue;
                    if (!sig.empty() && tags_sig(cand) == sig) { target = path; break; }
                }
                closedir(d);
            } else {
                fail(res, 500, "boards dir not found (is this running on the device?)");
                return;
            }
            if (target.empty()) target = dir + "/" + sanitize(name) + ".json";

            // Atomic write: temp + rename.
            const std::string tmp = target + ".tmp";
            {
                std::ofstream o(tmp, std::ios::binary);
                if (!o) { fail(res, 500, "cannot write profile file"); return; }
                o << req.body;
            }
            if (std::rename(tmp.c_str(), target.c_str()) != 0) {
                std::remove(tmp.c_str());
                fail(res, 500, "profile rename failed");
                return;
            }

            // Reload: the exact coldplug replay install-on-device.sh uses (the launcher
            // re-matches by tags and restarts the handler). Configurable; empty = skip.
            const std::string reload = env_or(
                "CONBOARD_RELOAD_CMD", "udevadm trigger --action=add --subsystem-match=usb");
            bool reloaded = false;
            if (!reload.empty()) reloaded = (std::system(reload.c_str()) == 0);

            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> w(sb);
            w.StartObject();
            w.Key("written");  w.String(target.c_str());
            w.Key("reloaded"); w.Bool(reloaded);
            w.EndObject();
            res.set_header("Content-Type", "application/json");
            res.body = sb.GetString();
            res.code = 200;
            res.end();
        });
}

}  // namespace conboard
