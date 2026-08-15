# Session 4 (roadmap "Session 6") — synthetic 1:1 keyboard rules on hotplug

**Scope: medium-small, already sized. Code-writing needs no hardware; final
verify needs the board, so it can queue behind Sessions 3/4/5 if they're
using it. First genuinely new feature after the v1.0.0-pre-beta tag — not a
bug fix.**

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

## Open design question — decide before writing code

**Where does "a new keyboard was plugged in" get noticed and trigger this?**
Two options, still open as of 2026-08-11, need a decision this session:

1. **The launcher** (udev-triggered C++, `LowLevel/launcher/`) calls the
   backend REST API when it sees an unmatched keyboard-class device — closer
   to "fully automatic," but new territory: the launcher today only touches
   `boards/*.json` + systemd, never the DB (deliberately decoupled — see
   "Don't-relearn facts" in `docs/NEXT-SESSION.md`).
2. **The console** does it explicitly — e.g. a "seed 1:1 rules" button in the
   existing Add-Device dialog, reusing the `GET /devices` inventory already
   there. Less new code, fits the current architecture cleanly, costs a click
   instead of being silent.

Recommendation if you want a default: start with option 2 (console button) —
smaller blast radius, doesn't touch the launcher's DB-decoupling invariant,
and ships something testable faster. Option 1 can follow later as a "make it
automatic" upgrade once the rule-generation logic itself is proven.

Either route needs simple dedup (don't regenerate ~100 rows every time the
same dongle reconnects) — a lookup-before-insert keyed on VID/PID or serial.

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

- Design decision (launcher vs. console button) recorded in this file or
  `NOTES.md` before code.
- Rule generation produces correct DB rows via the existing harpia CRUD.
- Dedup confirmed (replug the same device, row count doesn't grow).
- At least one generated rule verified live on the board to actually produce
  output — not just confirmed present in the DB.
