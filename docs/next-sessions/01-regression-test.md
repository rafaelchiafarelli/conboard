# Session 1 — regression test for the trigger-parsing gap

**Scope: small. No hardware, no board access needed. Safe to run fully in
parallel with Session 2 or anything else — own branch, no file overlap.**

## Why this is session 1

On 2026-08-15 we found and fixed a bug (commit `82b8bd9`) where
`jsonParser::parseIO()` (`LowLevel/Common/src/jsonParser.cpp`) only populated a
rule's evdev trigger (`evtrig`, via `evmatch::resolveSymbol()`) for
`"type":"joystick"` input objects. The `mouse`/`keyboard` case branches only
parsed *output*-shaped fields (`dx`/`dy`/..., `data`/`keyType`/...) and never
touched `evtrig` — so every keyboard/mouse rule silently never matched, ever,
with **83 unit tests passing the whole time.** The bug shipped for weeks
because nothing tested that specific path. Live proof it was broken: an
inotify watch on `/dev/input/event0`+`event1`+`/dev/hidg0` caught 1306 real
input events and 0 HID writes before the fix (full writeup: `NOTES.md`, search
"RESOLVED — mouse/keyboard rule output not firing").

This session closes that test gap so the same class of bug can't silently
reappear.

## Start here — exact facts, no need to re-read the project

- Test file: `tests/test_jsonparser.cpp`. Already has the pattern to copy —
  read the test case **`"joystick input: symbolic code + mode resolve into
  evtrig"`** (lines 110–146 as of this writing). It builds a synthetic board
  JSON via `jp.ReloadFromString(...)`, then asserts on
  `modes[0].body_actions[N].in.evtrig.{type,code,mode,threshold,holdMs}`.
- There is **no equivalent test for `"type":"keyboard"` or `"type":"mouse"`
  input objects** — that gap is exactly what let the bug through.
- The fix itself lives in a shared helper `parseEvTrigger()` (anonymous
  namespace, top of `LowLevel/Common/src/jsonParser.cpp`), called from all
  three `case devType::{joystick,keyboard,mouse}` branches in `parseIO()`,
  guarded on `act.HasMember("code")` so it's a no-op on output objects.
- Test build/run: `./run-tests.sh json` runs just this suite (fast). Full
  suite: `./run-tests.sh` (currently 83 cases / 233 assertions, all passing).
  No `--qemu` needed for this — pure host build is enough since this is
  logic-only, no hardware.

## Task

Add test cases to the `TEST_SUITE("json")` block in `tests/test_jsonparser.cpp`,
modeled directly on the existing joystick one:

1. A keyboard input trigger, e.g. `{"type":"keyboard","code":"KEY_A","mode":"press"}`
   as a rule's `"input"` — assert `evtrig.type == evmatch::EV_KEY_`,
   `evtrig.code` matches `KEY_A`'s resolved code, `evtrig.mode == evmatch::ev_press`.
2. A keyboard `hold` trigger with `"interval"` — assert `holdMs` comes from
   `interval` (mirror the joystick `BTN_TL hold` case).
3. A mouse input trigger, e.g. `{"type":"mouse","code":"BTN_LEFT","mode":"press"}`
   — same shape of assertions.
4. A mouse axis trigger, e.g. `{"type":"mouse","code":"REL_WHEEL","mode":"higher","value":0}`
   — assert `evtrig.mode == ev_higher` and `threshold`.
5. (Optional but cheap) One case that also parses the existing keyboard
   **output** shape (`"data"`/`"keyType"`) on the *same* rule as one of the
   above, to lock in that adding trigger-parsing to the keyboard/mouse
   branches didn't break output parsing — i.e. both `a.in.evtrig` and
   `a.out[0].kData` are correct on one rule.

Keep the assertions in the same style as the existing suite (`CHECK`/`REQUIRE`,
`evmatch::` namespace for the enums).

## Done criteria

- New cases pass: `./run-tests.sh json`.
- Full suite still green: `./run-tests.sh` (expect ~88 cases now).
- Commit on its own branch (e.g. `test/keyboard-mouse-trigger-parsing`), merge
  to `main` when green. This doesn't need board verification — it's pure
  logic, same as the rest of the `json` suite.
