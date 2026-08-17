#include "wifi_screen.hpp"

namespace wifi_screen {

void push(appshell::Shell &shell, const RestClient &rest)
{
    lv_obj_t *scr = shell.pushScreen();

    auto response = rest.get("/hmi/wifi/networks");
    if (!response) {
        appshell::createInfoLabel(scr, "WiFi list unavailable\n(backend unreachable?)");
        return;
    }
    if (!response->is_array() || response->empty()) {
        appshell::createInfoLabel(scr, "no networks found");
        return;
    }

    lv_obj_t *list = appshell::createMenuList(shell, scr);
    for (const auto &net : *response) {
        std::string ssid = net.value("ssid", "");
        if (ssid.empty())
            continue;
        std::string signal = net.value("signal", "");
        std::string label = signal.empty() ? ssid : ssid + "  (" + signal + "%)";
        appshell::addMenuItem(shell, list, label, []() {});
    }
}

} // namespace wifi_screen
