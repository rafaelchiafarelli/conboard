# Session 5 (roadmap "Session 7") — HMI phase 4a: one real screen (WiFi list)

**Scope: small, deliberately one screen only. Code-writing needs no
hardware; final verify needs the board + the ST7789 panel. Queue behind
Sessions 3/4/5 for board time if they're active.**

## Why this is scoped to exactly one screen

HMI phases 1–3 (deps, adaptive component layer, dark/amber theme matching the
console) are done and hardware-confirmed — see `docs/NEXT-SESSION.md` /
`NOTES.md` for the 2026-08-11/12 writeup, including hard-won SPI/GPIO gotchas
(don't touch `panel_st7789.cpp`'s init sequence without re-testing on real
hardware — see "Do not clean up this sequence" note there).

Phases 4–5 (actual WiFi/activation/radio screens, encoder/button wiring) are
**not started**. Building all of them in one session repeats the exact
scope-creep pattern from before v1 (shipping a whole touchscreen subsystem
while the core remap loop was still silently broken). Do **one screen**, get
it hardware-confirmed, stop. Session 6 (roadmap "Session 8") picks up the
rest only after this one lands.

## Start here — exact facts, no need to re-read the project

- Backend endpoint, already real (not a stub):
  `GET /api/v1/hmi/wifi/networks` (`backend/src/hmi.cpp:118`), gated by the
  same `X-Pswd` header as every other route. Response shape (confirmed by
  reading the handler):
  ```json
  [ { "ssid": "...", "signal": "...", "security": "..." }, ... ]
  ```
  Built from `nmcli -t -f SSID,SIGNAL,SECURITY dev wifi list`. Read-only —
  there's no WiFi-*connect* action yet, that's explicitly a later, mutating
  addition. This screen is display-only: show the list.
- REST client on the device side: `RestClient` (`LowLevel/HMI/include/
  rest_client.hpp` / `src/rest_client.cpp`), libcurl-based. The existing demo
  screen in `LowLevel/HMI/src/main.cpp` (around line 184) shows the exact
  pattern to copy:
  ```cpp
  lv_obj_t *scr = shell.pushScreen();
  RestClient rest(restBase, "hmi", envOr("CONHMI_REST_PSWD_HASH", "1bf812ac18b80d4a5ea4d51e6bfb7f58"));
  auto response = rest.get("/hmi/console-url");
  ...
  appshell::createInfoLabel(scr, text);
  ```
  For a list screen, use `appshell::createMenuList()`/`addMenuItem()` instead
  of `createInfoLabel` — see `LowLevel/HMI/include/app_shell.hpp` for the
  full `Shell`/`createMenuList`/`addMenuItem`/`ValueRow` API. One row per
  network (SSID + maybe signal strength in the label text), no `onActivate`
  behavior needed yet since this is read-only (a no-op callback is fine, or
  wire it later when WiFi-connect exists).
- JSON parsing on the device side: vendored `nlohmann/json`
  (`LowLevel/HMI/include/json.hpp`) — same library already used elsewhere in
  this module.
- Theme: already wired globally in `main.cpp` right after
  `lvgl_glue::createDisplay()` — a new screen built from `app_shell`
  primitives (`lv_list`, its buttons) inherits it automatically, per the
  `hmi_theme.cpp` design (deliberately narrow: covers screens/containers/
  `lv_list` only — if this screen needs a new widget kind LVGL-wise, check
  `themeApply()` in `LowLevel/HMI/src/hmi_theme.cpp` first).
- Build/deploy: `./build-cross.sh zero3`, then the same
  scp+tar+install-on-device.sh cycle as everything else (see any
  `docs/NEXT-SESSION.md` section for the exact commands) —
  **remember the documented redeploy pitfall**: always re-run
  `tar xzf conboard-zero3.tar.gz` before `install-on-device.sh` on every
  redeploy, not just the first, or you'll silently reinstall a stale binary.
  `hmi.service` is not auto-started by install — `systemctl enable --now
  hmi.service` manually, or it may already be enabled from a prior session
  (`systemctl is-active hmi.service` to check).

## Task

1. Add a WiFi-list screen: fetch `/hmi/wifi/networks`, render one menu row per
   network via `appshell::createMenuList`/`addMenuItem`.
2. Wire navigation to reach it from wherever the existing demo screen is
   pushed (or replace the demo screen with a small top-level menu that has
   "WiFi" as one entry, if that's a cleaner base for phase 4b's activation/
   radio screens to slot into later — your call, but keep it minimal).
3. Handle the empty/error case (no networks found, REST call fails) without
   crashing — `RestClient`'s existing error handling pattern (see how the
   demo screen handles a failed `rest.get()`) should be enough to mirror.

## Done criteria

- Compiles, cross-builds clean.
- **Hardware-confirmed on the real ST7789 panel**, not just `--panel null
  --dump` under qemu (that's fine for a first visual check, but this needs a
  real confirm same as phase 3's theme did) — deploy to `192.168.7.4`,
  restart `hmi.service`, confirm the WiFi list actually renders real networks
  by eye on the physical panel.
- Update `docs/NEXT-SESSION.md`'s HMI section to reflect phase 4a done, and
  narrow "phases 4-5 not started" down to just what's left (activation/radio
  screens, encoder/button wiring — Session 6/"Session 8" territory).
