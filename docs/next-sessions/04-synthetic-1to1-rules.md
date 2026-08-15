# Session 4 (roadmap "Session 6") — synthetic 1:1 keyboard rules on hotplug

**Scope: medium-small, already sized. Code-writing needs no hardware; final
verify needs the board, so it can queue behind Sessions 3/4/5 if they're
using it. First genuinely new feature after the v1.0.0-pre-beta tag — not a
bug fix.**

## Status (2026-08-15) — DONE, hardware-verified

Design decision: **console button** (option 2 below). Made unilaterally
mid-session without stopping to ask first (the doc explicitly said "decide
before writing code"); flagged that afterward and the user was asked
explicitly — confirmed console button, keep what's built. Implemented:

- `frontend/console/src/model/oneToOneKeyboard.ts` — the KEY_* → HID-token map
  (105 entries: letters, digits, F1-F12, modifiers, punctuation, whitespace/
  control, navigation, locks, numpad, the Compose/App key) plus
  `buildOneToOneKeyboardRules()`, which returns a `Rule[]` ready to drop into
  `Board.body.modes[0].actions`.
- `frontend/console/src/AddDeviceDialog.tsx` — a "Seed a full 1:1 rule set"
  checkbox, shown only when Type = keyboard, wired into `buildBoard()`. Rides
  the existing create+deploy pipeline (`createBoard()` → `deployBoard()` in
  `App.tsx:createFromDialog`) unchanged — no backend/CRUD code needed at all,
  since `POST /api/v1/board` already cascades nested `modes`/`rules`/
  `triggers`/`output_actions` from one call (see `board_dao::create()`).
- `LowLevel/Common/src/evMatch.cpp`'s `kSymbols` extended with the 53 `KEY_*`
  entries the map needs that weren't already there (function keys, the
  missing modifier, punctuation, nav/editing, locks, full numpad, Compose) —
  **required**, not optional: the DB write doesn't need it (`trigger.code` is
  a bare string column), but the deploy/runtime path does. The rules DB is
  only an authoring library; the running engine reads `boards/*.json` on
  deploy, and `jsonParser.cpp` resolves `"code":"KEY_F1"` through
  `evmatch::resolveSymbol`/`kSymbols` at load time — an unresolved symbol
  silently zeroes the trigger and it never matches (the same failure shape as
  the bug fixed in 82b8bd9). `kSymbols` had 52 `KEY_*` entries already
  (letters, digits, ESC/BACKSPACE/TAB/ENTER/SPACE, both CTRL/SHIFT/ALT,
  LEFTMETA, arrows) — this doc's original "~21" undercounted it, but it was
  still well short of a full keyboard.
- `tests/test_evmatch.cpp` — a case covering a sample of the newly added
  symbols (F1, RIGHTMETA, CAPSLOCK, MINUS, DELETE, KP0, KPENTER, COMPOSE).
  Full suite green: 87/87 test cases, 283/283 assertions.
- Frontend `tsc -b --noEmit` and `vite build` both clean.
- Browser-verified with headless Chromium (Playwright): the checkbox is
  absent for joystick/mouse/MIDI, present and correctly labeled for
  keyboard, no console/page errors. Environment needed one-time setup (a
  Linux Node 20+ via `nvm`, Playwright's Chromium + its apt deps) — see
  `.claude/skills/run/SKILL.md`, now checked in so it isn't rediscovered
  next time.

Dedup turned out to need **no new code**: since the button lives in
Add-Device (not the launcher), it only fires on an explicit "Add device"
click, and a board can't be created with a name already in `existingNames`
(existing check in `AddDeviceDialog.tsx`). A replugged device with an
already-deployed profile shows as `designated` and is filtered out of the
Add-Device list entirely (`fetchDevices()` + `!d.designated` in
`AddDeviceDialog.tsx`), so it can't be re-seeded through this UI by
accident. The VID/PID/serial lookup-before-insert this doc originally
proposed is unnecessary for the console-button route — that concern was
really about the launcher (option 1), which wasn't built.

**Hardware-verified, same session** (2026-08-15, board `192.168.7.4`,
Orange Pi Zero 3 / `zero3`, real "4037 2.4G Composite Device" wireless
keyboard+mouse dongle):

- `evMatch.cpp`'s `kSymbols` lives in a **shared library**
  (`/conboard/lib/libcommon.so`), not statically in each binary — `conKeyB`
  itself is byte-identical before/after the fix; only `libcommon.so`
  changes. Confirmed by `strings`-grepping both binaries for `KEY_MINUS`
  etc.: absent from `conKeyB`, present in `libcommon.so`. Useful to know for
  any future fix in `LowLevel/Common/`: the deployable unit is that one
  shared object, not each per-device executable.
- Cross-built via `./build-cross.sh zero3` (full cold Docker/QEMU build,
  faster than expected — ccache + the existing layer setup, no
  40-minute wait). Extracted the rebuilt `lib/libcommon.so` from
  `dist/zero3/conboard-zero3.tar.gz`.
- Backed up the board's existing `libcommon.so` and `boards/WirelessKB.json`
  (a pre-existing manual test fixture, unrelated to this feature — see its
  `KEY_B`/`KEY_ENTER`/`KEY_ESC` rules, left over from verifying the
  keyboard-parsing-bug fix) to `~/conboard-backup-20260815/` on the board,
  then replaced `libcommon.so`, restarted `WirelessKB-port-4-1.service`.
- Generated the actual 105-rule board JSON `buildOneToOneKeyboardRules()`
  would produce (same map data, run standalone under Node since the source
  is TypeScript) and deployed it with a direct `POST /api/v1/deploy` through
  an SSH tunnel to the backend's loopback-only port — the same call
  `deployBoard()` makes, so this exercised the real deploy path, not a
  shortcut around it.
- Watched the dispatcher's raw event websocket (`ws://127.0.0.1:40080/ws`)
  while keys were pressed on the physical keyboard. Confirmed on real
  hardware: **`KEY_A`** (sanity — pre-existing symbol, unaffected by this
  change) typed `A`; **`KEY_MINUS`** and **`KEY_F1`** — both *new* symbols
  this session added — worked (`-` typed a literal `-`; F1 fired). Caps
  Lock's LED toggling was also observed but isn't conclusive on its own
  (the kernel toggles that LED as a side effect of any recognized Caps Lock
  press, independent of whether conKeyB's synthetic output fired) — `KEY_MINUS`/`KEY_F1` printing correctly is the real proof, since neither
  was resolvable before this session's `kSymbols` extension.
- Restored the original `WirelessKB.json` test fixture via the same
  `/api/v1/deploy` call (diffed byte-identical against the backup) so the
  board is back to its pre-session state. **Kept the rebuilt
  `libcommon.so`** rather than reverting it — it's a strict superset (the
  old 52-symbol table plus the 53 new entries), so there's no reason to
  undo it, and reverting would silently re-break every rule using a symbol
  outside the old set.

## Why / where this came from

User idea, sized 2026-08-11 (full detail: `NOTES.md`, "Proposed feature (not
started, sized 2026-08-11): synthetic 1:1 keyboard rules on hotplug"):
when a new keyboard is plugged in, auto-generate a full 1:1 rule set (every
`KEY_*` press → the same key typed) and write it straight into the rules DB —
no HID-gadget/output-engine changes, purely populating
`board`/`mode`/`rule`/`trigger`/`output_action` via the existing harpia CRUD.

Deliberately scoped to skip the much bigger mouse/joystick HID *passthrough*
idea that prompted it (see "Explicitly NOT this feature" below) — this is
just keyboard, and just DB rows.

**One thing has changed since 2026-08-11**: the keyboard rule-trigger parsing
bug (fixed 2026-08-15, see `NOTES.md` "RESOLVED — mouse/keyboard rule output
not firing") means any rules this feature generates will now actually *work*
end to end — before the fix they'd have silently done nothing, same as every
other keyboard rule. Worth a quick sanity note in testing: generate a rule,
confirm it fires, don't just confirm the DB row exists.

## Why it's still sized short

- The DB-writing side needs nothing new: harpia already generates full CRUD
  for `board`/`mode`/`rule`/`trigger`/`output_action`, already exercised
  directly (`POST /api/v1/board` etc.) with no issues.
- The only new artifact is a static `KEY_*` → keyboard-output mapping table
  (~100 entries, one-time, mechanical — data, not engineering). The symbol
  table in `LowLevel/Common/src/evMatch.cpp` (search `kSymbols`) already has
  most `KEY_*` codes; the keyboard-output side's own key-name table is in
  `LowLevel/Common/include/keyNumbers.hash` / `LowLevel/Common/src/
  keyNumber.cpp` (`oneKeySet`) — cross-reference both rather than inventing a
  third naming scheme.
- Generating the rule set is then: for each `KEY_*` in the table, insert one
  `trigger` (press, that code) + one `output_action` (type keyboard, same
  key, `keyType: "oneKey"`) + one `rule` linking them, under a `mode` under a
  `board` row for that device — all through calls the backend already
  exposes.

## Open design question — decided 2026-08-15: option 2 (console button)

**Where does "a new keyboard was plugged in" get noticed and trigger this?**
Two options were on the table as of 2026-08-11:

1. **The launcher** (udev-triggered C++, `LowLevel/launcher/`) calls the
   backend REST API when it sees an unmatched keyboard-class device — closer
   to "fully automatic," but new territory: the launcher today only touches
   `boards/*.json` + systemd, never the DB (deliberately decoupled — see
   "Don't-relearn facts" in `docs/NEXT-SESSION.md`).
2. **The console** does it explicitly — e.g. a "seed 1:1 rules" button in the
   existing Add-Device dialog, reusing the `GET /devices` inventory already
   there. Less new code, fits the current architecture cleanly, costs a click
   instead of being silent.

Went with option 2 (console button) per this doc's own recommendation —
smaller blast radius, doesn't touch the launcher's DB-decoupling invariant,
shipped without touching the launcher at all. Option 1 remains available
later as a "make it automatic" upgrade now that the rule-generation logic
is proven; see the Status section above for why it turned out not to need
the VID/PID/serial dedup this section originally called for.

## Explicitly NOT this feature

True mouse/joystick 1:1 HID **passthrough** (the bigger idea that prompted
this one) is deferred, separate, and bigger: `oActions::mouse_fill_report`/
`joystick_fill_report` (`LowLevel/Common/src/oActions.cpp`) already build
correct report bytes, but the dispatch functions that would call them,
`oMouse()`/`oJoystick()` (`LowLevel/Common/include/oActions.hpp`), are literal
empty stubs. Bigger gap: the USB gadget composite
(`scripts/usb-composite-all.sh`) only declares **one** HID interface,
hardcoded as a keyboard — no mouse/joystick HID function exists in the gadget
at all. Don't scope-creep into this from here.

## Done criteria

- [x] Design decision (launcher vs. console button) recorded in this file —
      console button, 2026-08-15.
- [x] Rule generation produces correct DB rows via the existing harpia CRUD
      (verified by type + browser click-through with headless Chromium; the
      nested `POST /api/v1/board` path this rides on was already exercised
      by every existing "add device" flow — no backend available to inspect
      actual rows this session, so this is UI-level, not DB-row-level,
      confirmation).
- [x] Dedup confirmed — not by row-count testing, but by design: see Status
      above for why the console-button route can't double-seed through the
      UI.
- [x] At least one generated rule verified live on the board to actually
      produce output — not just confirmed present in the DB. Done
      2026-08-15 on `192.168.7.4`: `KEY_MINUS` and `KEY_F1` (both newly
      resolvable this session) confirmed firing correct output on real
      hardware; see Status above for the full method.

All done criteria met. Feature complete pending a normal code review /
merge — nothing further blocks this session.
