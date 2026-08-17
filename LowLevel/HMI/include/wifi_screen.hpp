// Phase 4a: the one real screen -- a read-only WiFi network list, sourced
// entirely from GET /hmi/wifi/networks (backend/src/hmi.cpp). No local WiFi
// state reads, no nmcli calls here -- see rest_client.hpp for why.
#pragma once

#include "app_shell.hpp"
#include "rest_client.hpp"

namespace wifi_screen {

// Pushes a new screen onto `shell` and fills it with one menu row per
// network (SSID + signal strength), fetched via `rest`. Rows are display
// only for now -- there's no WiFi-connect action on the backend yet, so
// selecting a row does nothing. Falls back to an info label, same pattern
// as main.cpp's console-url demo, if the fetch fails or returns no
// networks.
void push(appshell::Shell &shell, const RestClient &rest);

} // namespace wifi_screen
