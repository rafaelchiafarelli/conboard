#include "activation_screen.hpp"

namespace activation_screen {

void push(appshell::Shell &shell, const RestClient &rest)
{
    lv_obj_t *scr = shell.pushScreen();

    auto response = rest.get("/hmi/activation");
    if (!response) {
        appshell::createInfoLabel(scr, "activation status unavailable\n(backend unreachable?)");
        return;
    }

    bool activated = response->value("activated", false);
    int failedAttempts = response->value("failed_attempts", 0);
    std::string password = "none";
    if (response->contains("power_password") && !response->at("power_password").is_null())
        password = response->at("power_password").get<std::string>();

    std::string text = "Activation\n\n";
    text += std::string("activated: ") + (activated ? "yes" : "no") + "\n";
    text += "power password: " + password + "\n";
    text += "failed attempts: " + std::to_string(failedAttempts) + "\n\n";
    text += "(stub data -- no login flow yet)";

    appshell::createInfoLabel(scr, text);
}

} // namespace activation_screen
