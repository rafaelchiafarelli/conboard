// conHMI -- the local screen + buttons + encoders UI process. Independent of
// the other LowLevel/* device handlers by design (see the plan doc): no
// libcommon, no DeviceEngine, no udev/launcher path. It owns a small SPI
// panel + 2 rotary encoders (each with its own pushbutton) + 2 plain
// pushbuttons, renders an LVGL UI, and gets ALL domain data (WiFi,
// activation, console URL, radio) from the backend's REST/JSON API -- never
// from local business logic.
//
// Phase 1-2 scope wired the dependencies together and shipped one concrete
// end-to-end demo screen (the console URL, fetched over HTTP) to prove the
// REST-sourced-data path for real. Phase 4a added the first real domain
// screen (WiFi list, wifi_screen.cpp). Phase 4b adds the activation screen
// (activation_screen.cpp) -- display-only against the still-stubbed backend
// endpoint, explicitly not the real power-password login flow. The radio
// screen is still later phase-4b/5 work.
//
// Nav scheme (which physical control does what): the standalone buttons
// now read their LVGL key from GET /hmi_binding (backend/harpia/
// conboard.harpia's hmi_binding table, separate from the rules-library
// domain) instead of a hardcoded key, falling back to the old ESC/NEXT
// choice if a binding is missing. Both encoders still use LVGL's native
// ENCODER indev semantics on real hardware -- fully honoring per-direction
// hmi_binding rows for them needs a custom keypad-style adapter, not done
// here. A dev-only /simulate HTTP endpoint (sim_server.cpp, opt-in via
// CONHMI_SIM_PORT) lets any of the 8 hmi_control events be exercised over
// curl without real GPIO hardware wired up.
#include "app_shell.hpp"
#include "clipped_panel.hpp"
#include "hmi_theme.hpp"
#include "lvgl_glue.hpp"
#include "panel_null.hpp"
#include "panel_st7789.hpp"
#include "push_button.hpp"
#include "rest_client.hpp"
#include "rotary_encoder.hpp"
#include "activation_screen.hpp"
#include "sim_server.hpp"
#include "wifi_screen.hpp"

#include <lvgl.h>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace {

std::atomic_bool g_running{true};
void onSignal(int) { g_running = false; }

std::string envOr(const char *key, const std::string &def)
{
    const char *v = std::getenv(key);
    return (v && *v) ? std::string(v) : def;
}
unsigned envOrUint(const char *key, unsigned def)
{
    const char *v = std::getenv(key);
    return (v && *v) ? static_cast<unsigned>(std::atoi(v)) : def;
}
int envOrInt(const char *key, int def)
{
    const char *v = std::getenv(key);
    return (v && *v) ? std::atoi(v) : def;
}

// Starts a control's poll thread; on failure (no such gpiochip/line -- e.g.
// running this build off the actual board) logs a warning and leaves it
// un-started rather than aborting, so the rest of the UI still runs. Real
// hardware failures are a phase-4/bring-up concern, not this demo's job.
template <typename Control>
bool tryStart(Control &c, const char *name)
{
    if (c.start())
        return true;
    std::cerr << "conHMI: " << name << " did not start (no hardware?) -- continuing without it"
              << std::endl;
    return false;
}

std::string lookupOr(const std::map<std::string, std::string> &m, const std::string &key,
                      const std::string &def)
{
    auto it = m.find(key);
    return it != m.end() ? it->second : def;
}

// Mirrors hmi_nav_key (backend/harpia/conboard.harpia) to the LV_KEY_
// constants app_shell/lvgl_glue already build screens against. Returns
// `fallback` for an empty/unrecognized name (0 is never a real LV_KEY_
// value here, so callers that pass 0 as fallback can use that to detect
// "no usable binding").
uint32_t navKeyToLvKey(const std::string &navKey, uint32_t fallback)
{
    if (navKey == "nk_next")   return LV_KEY_NEXT;
    if (navKey == "nk_prev")   return LV_KEY_PREV;
    if (navKey == "nk_select") return LV_KEY_ENTER;
    if (navKey == "nk_back")   return LV_KEY_ESC;
    if (navKey == "nk_up")     return LV_KEY_UP;
    if (navKey == "nk_down")   return LV_KEY_DOWN;
    return fallback;
}

// Fetches every {control, navKey} row from GET /hmi_binding -- the
// hmi_binding table (backend/harpia/conboard.harpia), separate from the
// rules-library domain, added so the panel's nav scheme is data, not
// compiled-in wiring. Absent "control"/"navKey" fields mean the enum's
// zero value (hc_encoder1_ccw / nk_next -- protobuf3 JSON omits
// zero-valued fields on the wire), matching every other harpia table.
std::map<std::string, std::string> fetchHmiBindings(const RestClient &bindingsRest)
{
    std::map<std::string, std::string> bindings;
    auto response = bindingsRest.get("/hmi_binding");
    if (!response || !response->is_array())
        return bindings;
    for (const auto &row : *response) {
        const std::string control = row.value("control", "hc_encoder1_ccw");
        const std::string navKey  = row.value("navKey", "nk_next");
        bindings[control] = navKey;
    }
    return bindings;
}

}  // namespace

int main(int argc, char *argv[])
{
    static const char short_options[] = "p:d:b:";
    static const struct option long_options[] = {
        {"panel", 1, nullptr, 'p'},        // "null" (default) or "st7789"
        {"dump",  1, nullptr, 'd'},        // panel=null: PPM path to write each frame to
        {"rest-base", 1, nullptr, 'b'},    // backend API base URL
        {}
    };

    std::string panelKind = "null";
    std::string dumpPath;
    std::string restBase = envOr("CONHMI_REST_BASE", "http://127.0.0.1:8080/api/v1");

    int c;
    while ((c = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
        switch (c) {
            case 'p': panelKind = optarg; break;
            case 'd': dumpPath  = optarg; break;
            case 'b': restBase  = optarg; break;
            default:
                std::cout << "usage: ./conHMI [--panel null|st7789] [--dump path.ppm] "
                             "[--rest-base http://host:port/api/v1]" << std::endl;
                return 1;
        }
    }

    std::unique_ptr<PanelDriver> panel;
    if (panelKind == "st7789") {
        St7789Panel::Config cfg;
        // spidev1.1, 4MHz, mode 3 are confirmed wiring/settings (the dev
        // board's known-working Python reference driver), not generic ST7789
        // defaults -- see the comment in panel_st7789.cpp.
        cfg.spiDevice     = envOr("CONHMI_SPI_DEVICE", "/dev/spidev1.1");
        cfg.spiSpeedHz    = envOrUint("CONHMI_PANEL_SPI_SPEED_HZ", 4'000'000);
        cfg.gpioChip      = envOr("CONHMI_GPIO_CHIP", "gpiochip0");
        cfg.resetLine     = envOrUint("CONHMI_PANEL_RESET_LINE", 71);  // PC7, confirmed wiring
        cfg.dcLine        = envOrUint("CONHMI_PANEL_DC_LINE", 74);    // PC10, confirmed wiring
        // -1 (default): no backlight line -- set this if your module needs one
        // driven high to light up (some tie BL permanently to 3.3V and don't).
        cfg.backlightLine = envOrInt("CONHMI_PANEL_BL_LINE", -1);
        // 240x320 is the panel's real physical resolution (a 2" ST7789
        // module) -- NOT the usable area once mounted in its enclosure; see
        // the CONHMI_WORK_* working-box wrapping below for that.
        cfg.width         = static_cast<int>(envOrUint("CONHMI_PANEL_WIDTH", 240));
        cfg.height        = static_cast<int>(envOrUint("CONHMI_PANEL_HEIGHT", 320));
        // 270 (landscape): hardware-confirmed correct orientation on this
        // exact panel/enclosure (2026-08-11).
        cfg.rotation      = envOrInt("CONHMI_PANEL_ROTATION", 270);
        panel = std::make_unique<St7789Panel>(cfg);
    } else {
        panel = std::make_unique<NullPanel>(
            static_cast<int>(envOrUint("CONHMI_PANEL_WIDTH", 240)),
            static_cast<int>(envOrUint("CONHMI_PANEL_HEIGHT", 320)),
            dumpPath);
    }

    // The "working box": the sub-rectangle of the panel's full physical
    // resolution that's actually usable once it's mounted in its enclosure.
    // Not known/fixed at compile time (the enclosure is still being worked
    // out), so it's entirely env-configured -- defaults to the full panel
    // (no clipping) until CONHMI_WORK_* is set. Everything above the panel
    // layer (LVGL, AppShell, this file's own demo screen) only ever sees
    // this size; ClippedPanel is the only place that knows about the offset.
    const int workX = envOrInt("CONHMI_WORK_X_OFFSET", 0);
    const int workY = envOrInt("CONHMI_WORK_Y_OFFSET", 0);
    const int workW = envOrInt("CONHMI_WORK_WIDTH", panel->width() - workX);
    const int workH = envOrInt("CONHMI_WORK_HEIGHT", panel->height() - workY);
    panel = std::make_unique<ClippedPanel>(std::move(panel), workX, workY, workW, workH);

    if (!panel->init()) {
        std::cerr << "conHMI: panel init failed (--panel " << panelKind << ")" << std::endl;
        return 1;
    }

    // Panel RESET/DC lines above are confirmed wiring (PC7/PC10). The encoder
    // and button line numbers below are still placeholders -- that wiring
    // isn't finalized yet; override via env for bring-up/testing until it is.
    const std::string gpioChip = envOr("CONHMI_GPIO_CHIP", "gpiochip0");
    RotaryEncoder encoder1({gpioChip, envOrUint("CONHMI_ENC1_A", 5), envOrUint("CONHMI_ENC1_B", 6)});
    PushButton encoder1Button({gpioChip, envOrUint("CONHMI_ENC1_BTN", 12)});
    RotaryEncoder encoder2({gpioChip, envOrUint("CONHMI_ENC2_A", 16), envOrUint("CONHMI_ENC2_B", 20)});
    PushButton encoder2Button({gpioChip, envOrUint("CONHMI_ENC2_BTN", 21)});
    PushButton button1({gpioChip, envOrUint("CONHMI_BTN1", 26)});
    PushButton button2({gpioChip, envOrUint("CONHMI_BTN2", 19)});

    // Each start() is attempted regardless of its sibling's result, so one
    // missing line doesn't hide a report about another.
    bool encoder1RotOk = tryStart(encoder1, "encoder1");
    bool encoder1BtnOk = tryStart(encoder1Button, "encoder1 button");
    bool haveEncoder1 = encoder1RotOk && encoder1BtnOk;
    bool encoder2RotOk = tryStart(encoder2, "encoder2");
    bool encoder2BtnOk = tryStart(encoder2Button, "encoder2 button");
    bool haveEncoder2 = encoder2RotOk && encoder2BtnOk;
    bool haveButton1 = tryStart(button1, "button1");
    bool haveButton2 = tryStart(button2, "button2");

    // Two RestClients, same credential, different X-User: "hmi" for the
    // hand-written backend/src/hmi.cpp routes (which only check X-Pswd),
    // "hmi_binding" for the harpia-generated hmi_binding CRUD (which checks
    // X-User == the entity name, like every generated route).
    const std::string pswdHash = envOr("CONHMI_REST_PSWD_HASH", "5a67e5f27cce34a1ec5ac267a70f5d87");
    RestClient rest(restBase, "hmi", pswdHash);
    RestClient bindingsRest(restBase, "hmi_binding", pswdHash);
    const std::map<std::string, std::string> bindings = fetchHmiBindings(bindingsRest);

    // Dev-only: simulate physical HMI events without real GPIO hardware,
    // e.g. `curl -d '{"control":"hc_button1_press"}' http://host:PORT/simulate`.
    // Off unless CONHMI_SIM_PORT is set -- see sim_server.hpp.
    const int simPort = envOrInt("CONHMI_SIM_PORT", 0);
    if (simPort > 0) {
        if (sim_server::start(simPort))
            std::cerr << "conHMI: simulate endpoint on :" << simPort << std::endl;
        else
            std::cerr << "conHMI: simulate endpoint failed to start on :" << simPort << std::endl;
    }

    lv_init();
    lv_display_t *disp = lvgl_glue::createDisplay(*panel);
    hmi_theme::init(disp);

    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    // Both encoders keep LVGL's native ENCODER indev semantics (rotation =
    // next/prev focus, press = select) -- hmi_binding rows for
    // hc_encoder*_ccw/cw/press exist and are honored by the /simulate path
    // below, but real hardware doesn't route through them yet: that needs a
    // custom keypad-style encoder adapter to replace LVGL's built-in one, a
    // separate decision from this pass. The two standalone buttons ARE
    // fully data-driven already -- createButtonIndev takes an arbitrary key.
    if (haveEncoder1) lvgl_glue::createEncoderIndev(encoder1, encoder1Button, group);
    if (haveEncoder2) lvgl_glue::createEncoderIndev(encoder2, encoder2Button, group);
    if (haveButton1)
        lvgl_glue::createButtonIndev(
            button1, navKeyToLvKey(lookupOr(bindings, "hc_button1_press", ""), LV_KEY_ESC), group);
    if (haveButton2)
        lvgl_glue::createButtonIndev(
            button2, navKeyToLvKey(lookupOr(bindings, "hc_button2_press", ""), LV_KEY_NEXT), group);

    // Top-level menu: a minimal base for phase 4b's screens to slot into,
    // one entry each. "Console URL" is the original phase 1-2 demo, now
    // reached via the menu instead of shown directly; "WiFi" is phase 4a's
    // first real domain screen; "Activation" is phase 4b's first addition.
    appshell::Shell shell(group);
    lv_obj_t *menuScr = shell.pushScreen();
    lv_obj_t *menuList = appshell::createMenuList(shell, menuScr);
    appshell::addMenuItem(shell, menuList, "Console URL", [&shell, &rest]() {
        lv_obj_t *scr = shell.pushScreen();
        auto response = rest.get("/hmi/console-url");
        std::string text = "console URL unavailable\n(backend unreachable?)";
        if (response && response->contains("url"))
            text = "conboard console:\n" + response->at("url").get<std::string>();
        appshell::createInfoLabel(scr, text);
    });
    appshell::addMenuItem(shell, menuList, "WiFi", [&shell, &rest]() {
        wifi_screen::push(shell, rest);
    });
    appshell::addMenuItem(shell, menuList, "Activation", [&shell, &rest]() {
        activation_screen::push(shell, rest);
    });

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    std::cerr << "conHMI: running (panel=" << panelKind << ", rest-base=" << restBase << ")"
              << std::endl;
    // onTick runs on this same thread, right after lv_timer_handler() --
    // the only place it's safe to call lv_group_send_data(), since LVGL
    // itself is not thread safe and sim_server's accept thread never
    // touches LVGL directly (see sim_server.hpp).
    lvgl_glue::runLoop(g_running, 5, [&]() {
        for (const auto &control : sim_server::drain()) {
            auto it = bindings.find(control);
            if (it == bindings.end()) {
                std::cerr << "conHMI: simulate -- no binding for control " << control << std::endl;
                continue;
            }
            const uint32_t key = navKeyToLvKey(it->second, 0);
            if (key == 0) {
                std::cerr << "conHMI: simulate -- unknown nav_key " << it->second
                          << " for control " << control << std::endl;
                continue;
            }
            lv_group_send_data(group, key);
            std::cerr << "conHMI: simulate -- " << control << " -> " << it->second << std::endl;
        }
    });

    encoder1.stop(); encoder1Button.stop();
    encoder2.stop(); encoder2Button.stop();
    button1.stop(); button2.stop();
    return 0;
}
