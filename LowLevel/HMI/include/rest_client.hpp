// Thin HTTP GET + JSON-parse client. This is the ONLY way conHMI learns
// anything about WiFi/activation/console-URL/radio: no nmcli shell-outs, no
// reading system network state, no local business logic -- the backend owns
// all of that and exposes it as JSON; this just calls the documented
// endpoints (backend/src/hmi.cpp) and hands back a parsed value.
#pragma once

#include "json.hpp"

#include <optional>
#include <string>

class RestClient {
public:
    // baseUrl e.g. "http://127.0.0.1:8080/api/v1" (the backend's own default,
    // see backend/README.md), no trailing slash. user/pswdHash mirror the
    // X-User/X-Pswd headers every backend route requires (backend/README.md:
    // "the hash is the compile-time md5 of the domain, not a secret").
    RestClient(std::string baseUrl, std::string user, std::string pswdHash);

    // GET <baseUrl><path>. Returns nullopt on any transport/HTTP-status/parse
    // error (logged to stderr); a parsed JSON value otherwise.
    std::optional<nlohmann::json> get(const std::string &path) const;

private:
    std::string baseUrl_;
    std::string user_;
    std::string pswdHash_;
};
