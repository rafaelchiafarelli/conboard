# Session 6 (roadmap "Session 8") — HMI phase 4b: activation/radio + encoder/button wiring

**Depends on Session 5 ("Session 7" in the roadmap) landing first** — don't
start this until the WiFi screen is merged and hardware-confirmed. Scope:
small-medium, needs the board for verification, queue behind hardware
sessions in progress.

## Why this comes after, and stays split from Session 5

Same reasoning as splitting phase 4 into two sessions at all: one merged,
hardware-confirmed screen at a time, not a multi-week HMI block shipped
while other things are unverified. This session has three genuinely
different pieces (activation screen, radio screen, physical encoder/button
wiring) — if any one of them turns out bigger than expected, land what's
done and push the rest to a follow-up rather than stretching this session.

## Start here — exact facts, no need to re-read the project

### Activation screen — currently a documented stub, not a real feature

- `GET /api/v1/hmi/activation` (`backend/src/hmi.cpp:139`) returns a fixed
  stub today: `{"activated": false, "power_password": null,
  "failed_attempts": 0}`. It's explicitly *not* wired to anything real.
- The actual design it's meant to eventually front is the **power-password**
  system (`backend/README.md`, "Security API" section, ~line 126): a
  password shown on the device screen, regenerated on every login attempt;
  3 failed attempts → lockout + red screen edge + harder password; 10 failed
  attempts → refuses remote connections until a password is entered
  *physically on the keyboard*; reboot resets the failed-attempt count.
- **This session does NOT implement power-password** — that's a real backend
  security feature, bigger than an HMI screen, out of scope here. What this
  session *can* do: build the activation screen against the stub's current
  shape (display `activated`/`failed_attempts`, no login flow yet), so the
  screen exists and is themed/navigable, ready for the real backend logic
  later without blocking on it. Say so explicitly in the screen or in
  `NOTES.md` — don't let it look more finished than it is.

### Radio screen — data source still genuinely undecided

- `GET /api/v1/hmi/radio/stations` (`backend/src/hmi.cpp:157`) returns an
  empty stub array today. The station data source is explicitly TBD — no
  decision has been made about where station data comes from (hardcoded
  list? internet radio API? local file?).
- **Decide the data source before building the screen**, or scope the screen
  itself down to "renders whatever the endpoint returns, including empty" so
  it isn't blocked on that decision. If you make the data-source call this
  session, update `backend/src/hmi.cpp`'s stub accordingly and note the
  decision in `NOTES.md` — this is exactly the kind of thing a future session
  (or a future you) will otherwise have to re-derive from scratch.

### Encoder/button wiring — physical GPIO, needs the board and a screwdriver

- Currently placeholder-only (`LowLevel/HMI/src/main.cpp` env var defaults):
  `CONHMI_ENC1_A/B`=5/6, `CONHMI_ENC1_BTN`=12, `CONHMI_ENC2_A/B`=16/20,
  `CONHMI_ENC2_BTN`=21, `CONHMI_BTN1`=26, `CONHMI_BTN2`=19. `conHMI` starts
  fine without them wired (logs "did not start (no hardware?)" per control
  and continues) — this is why it's been safe to defer.
- Once physically wired: update the `systemctl edit hmi.service` drop-in
  (see `docs/NEXT-SESSION.md`'s "Persisting hardware config" section for the
  exact pattern — **never edit `/etc/systemd/system/hmi.service` directly**,
  `install-on-device.sh` overwrites it fresh on every run) with the real GPIO
  line numbers, then tell whoever's doing the next session (or update this
  file / `main.cpp`'s compiled-in defaults) so future installs don't need the
  drop-in.
- Also still open: the **working box crop** (`CONHMI_WORK_X_OFFSET`/
  `Y_OFFSET`/`WIDTH`/`HEIGHT`, all default to full 320×240 landscape, no
  crop) — the physical enclosure crops the visible area to something smaller/
  offset, needs real measurements against the actual enclosure. See
  `LowLevel/HMI/include/clipped_panel.hpp` for how the crop composes
  (everything above it in the stack — LVGL, AppShell — only ever sees the
  cropped size, so get this right before fine-tuning screen layouts against
  it).
- Which physical control does what (nav scheme — e.g. "left encoder scrolls
  the menu, right encoder's button confirms") is also still an open design
  decision, not just a wiring task. `LowLevel/HMI/src/lvgl_glue.cpp` exposes
  each `RotaryEncoder`/`PushButton` as a clean LVGL indev; deciding which
  indev goes into which screen's focus group is explicitly left to this
  phase (see the comment at the top of `app_shell.hpp`).

## Done criteria (per piece — land independently, don't block one on another)

- Activation screen: renders the stub's current fields, clearly not a real
  login flow yet, hardware-confirmed on the panel.
- Radio screen: data-source decision recorded, screen renders whatever the
  (possibly still-stub) endpoint returns without crashing on empty.
- Encoder/button wiring: real GPIO lines confirmed via a `systemctl edit`
  drop-in, physical turn/press actually navigates something on the panel,
  nav-scheme decision recorded in `NOTES.md` or this file for the next
  screen that needs it.
- Update `README.md`'s HMI status line and `docs/NEXT-SESSION.md` once any of
  these land — don't leave the "phase 4-5 not started" language stale.
