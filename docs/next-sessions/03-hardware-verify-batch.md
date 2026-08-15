# Sessions 3, 4, 5 — hardware verification, back-to-back

**Scope: small each, but all three need exclusive time on the physical
board (`192.168.7.4`) and on you, physically, plugging in different
hardware. They cannot run in parallel with each other, or with anything
else that's mid-`install-on-device.sh` on that board — only one install can
be live on it at a time, and only one physical device can be plugged into a
given USB port at a time.**

## The "back-to-back" concept

Don't split these into three separate chat sessions spread across days. Each
one is a short, focused verification with almost no code expected (these are
*confirm it actually works*, not *build something new*) — the overhead of a
fresh Claude session re-deriving board state, SSH access, and current install
state each time would cost more than the checks themselves take. Instead:

1. Do all three in **one sitting**, one continuous conversation.
2. Run them **in this order** (3 → 4 → 5) since each needs a different
   physical device swapped into the board, and you're already there.
3. If any of them finds a real bug, that becomes its own small fix+verify
   loop (same pattern as the keyboard/mouse trigger-parsing fix from
   2026-08-15: read code, reproduce live, fix, rebuild, redeploy, reverify)
   — don't try to also finish the other two checks in the same breath as a
   deep-dive fix. Finish the fix, then resume the batch.
4. Board access: SSH to `192.168.7.4` with your usual credentials (not
   written here deliberately — this file gets committed and shared). Web
   console login: `sudo conboard-password` on the board if you need it again.

---

## 3a. Joystick hardware test (needs a gamepad)

**Why**: joystick is the *only* device type whose trigger-parsing was never
actually the bug (it was the one case in `parseIO()` that correctly called
`evmatch::resolveSymbol()` all along) — but it's also the one device type
that's **never been hardware-tested at all**. `docs/HW-TEST-evdev.md` has the
general runbook; no gamepad was available in any session so far.

**Facts**:
- Starter profile already exists: `boards/Xbox360.json`. Its device-match
  tags (`header.identifier.tags`): `ID_BUS=usb`, `ID_VENDOR_ID=045e`,
  `ID_MODEL_ID=028e` (a real Xbox 360 pad/receiver). If your gamepad isn't
  this exact one, either match those IDs or the console's Add-Device flow
  (backed by `GET /api/v1/devices`) should still classify it as a joystick by
  kernel ABI (`condetect::classifyEvdev` — VID/PID is identity-only, not
  matching).
- Executable: `LowLevel/Joystick/build/conJoyS`, shares the same
  `EvdevDevice`/`DeviceEngine` stack as `conKeyB`/`conMouse` — the exact code
  path that was just proven working for keyboard/mouse on 2026-08-15.

**Steps**:
1. Plug the gamepad in, `ssh` to the board, confirm a `conJoyS-*.service`
   auto-starts (`systemctl list-units | grep -i joy`).
2. Same live-watch technique used 2026-08-15 works here too: watch the
   gamepad's `/dev/input/eventN` and `/dev/hidg0` for real activity while
   pressing buttons (the inotify python watcher used that session is at
   `/tmp/watch_multi.py` on the board if still present, or rewrite it — it's
   ~30 lines, see the technique described in `NOTES.md` under "RESOLVED —
   mouse/keyboard rule output not firing").
3. Confirm output actually reaches the connected host, not just the write().

**Done**: real button press → real output on host, confirmed by eye. Update
`NOTES.md`/`README.md` "What is Missing?" section (currently says joystick
"built + unit-tested but not yet exercised on real hardware").

---

## 3b. Queue-overflow load test (needs a way to generate a real sustained burst)

**Why**: `STACKED_IO_MSG` (`LowLevel/Common/include/zmq_coms.hpp:20`) was
raised `10`→`64` with drop-oldest eviction on 2026-08-12 (see `INTERFACE.md`
§O4) and deployed, but never actually load-tested against a real sustained
burst — only shipped.

**Facts**:
- The overflow check is in `LowLevel/Common/src/zmq_coms.cpp:136`
  (`if (io_queue.size() >= STACKED_IO_MSG)`).
- A real sustained burst is easiest to generate with continuous mouse
  motion/wheel — `REL_X`/`REL_Y`/`REL_WHEEL` events fire very fast under
  normal use (this was visible in the 2026-08-15 session: ~1000 events in
  45 seconds from ordinary mouse use). Deliberately shake/spin the mouse
  fast and sustained for several seconds to try to exceed 64 queued
  `report()` calls before the dispatcher drains them.
- Watch for the rate-limited overflow log (`dispatch overflow (reporting
  queue full, dropping oldest)`, `DeviceEngine::report()` in
  `LowLevel/Common/src/deviceEngine.cpp`) in
  `journalctl -u WirelessMouse-port-4-1.service -f` while doing this.

**Done**: either (a) you can't trigger overflow under realistic use, which is
itself a useful confirmation, or (b) you can, and the drop-oldest behavior
degrades gracefully (no crash, no wedge, monitor recovers) rather than losing
the pipeline. Note the result in `NOTES.md`.

---

## 3c. DJ-Tech-4-Mix MIDI reconfirm (needs that MIDI controller)

**Why**: `conMIDI`'s open-retry fix (recovering from a transient ALSA
port-busy failure after a redeploy restart, `LowLevel/Common/include/
runDevice.hpp`) was written and unit-tested but flagged "pending" since
2026-08-11 — no MIDI hardware was available in later sessions to confirm it
on the device it was actually written for.

**Facts**:
- Starter profile: `boards/Dj4Mix.json`, match tag `ID_MODEL=DJ-Tech_4-Mix`.
- This board already has real MIDI hardware-verified in general (input/output,
  operation modes) — this check is specifically about the **redeploy-recovery**
  path, not first-time functionality.

**Steps**:
1. Plug in the DJ-Tech-4-Mix controller, confirm it's picked up normally.
2. Deploy/redeploy its profile via the console (this is what used to leave the
   handler inert — the restart is the point of the test, not just plugging
   in).
3. Confirm `conMIDI` recovers and keeps reporting/producing output correctly
   after the redeploy restart, not just that it starts.

**Done**: redeploy-restart survives cleanly with real hardware attached, MIDI
events keep flowing before and after. Update `NOTES.md` to close this out
(currently "Still pending... hardware wasn't available this session").
