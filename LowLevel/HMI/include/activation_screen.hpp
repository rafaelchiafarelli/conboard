// Phase 4b: activation screen, built against the documented STUB shape of
// GET /hmi/activation (backend/src/hmi.cpp) -- {activated, power_password,
// failed_attempts}. This is NOT the power-password login flow described in
// backend/README.md's Security API section; that's a real backend security
// feature, out of scope here. This screen only ever displays what the stub
// returns, and says so on-screen, so it can't be mistaken for a finished
// login flow.
#pragma once

#include "app_shell.hpp"
#include "rest_client.hpp"

namespace activation_screen {

// Pushes a new screen onto `shell` and renders the stub's current fields.
// Falls back to an info label, same pattern as wifi_screen::push, if the
// fetch fails.
void push(appshell::Shell &shell, const RestClient &rest);

} // namespace activation_screen
