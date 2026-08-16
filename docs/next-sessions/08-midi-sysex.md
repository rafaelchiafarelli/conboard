# Session 8 — MIDI SysEx support

**Scope: large, and genuinely different in shape from the rest of the rule
engine — this is the one MIDI gap that touches every layer (firmware wire
format, ZMQ envelope, DB schema, console UI), not just the MIDI handler.
Deliberately kept as its own session, separate from
`09-midi-identical-device-separation.md` — different layers, different risk,
no reason to block one on the other. Needs the board + real MIDI hardware
that actually emits SysEx (patch dumps, firmware update tools, some
controllers' extended-feature messages) to verify against — you have MIDI
hardware now, confirm it can produce/consume real SysEx before starting.**

## Why

`README.md`'s "What is Missing?" list: *"SysEx commands are not working."*
Never scoped beyond that one line before this session.

## The core problem: every layer assumes a fixed 3-byte MIDI message

SysEx is fundamentally a different shape of message than everything else
this project's rule engine currently models — this isn't a small gap to
patch, it's a missing message class:

- **Firmware struct**: `midiSignal` (`LowLevel/Common/include/actions.h:46`)
  is `typedef union { char byte[4]; uint32_t asInt; } midiSignal` — a fixed
  4-byte union (3 data bytes + a padding byte, see the comment in
  `LowLevel/MIDI/src/midithread.cpp:185` about the 4th byte being an
  out-of-bounds-write fix, not real data). A SysEx message is
  `0xF0 <arbitrary-length data> 0xF7` — anywhere from a few bytes to tens of
  KB (patch/firmware dumps). **Cannot fit in this struct at all**, not even
  approximately.
- **Matcher**: `midimap::matches()`/`resolveOutputs()`
  (`LowLevel/Common/include/midiMap.hpp`, `.cpp`) compare `b0`/`b1`/`b2`
  exactly or by threshold (`midi_normal`/`trigger_higher`/`trigger_lower`/
  `spot`/`blink`) — none of these modes make sense for SysEx. A SysEx
  "trigger" is more naturally *match a byte-prefix* (e.g. "any message
  starting with this manufacturer ID") or *match exactly* (a specific known
  dump request), not a 3-byte equality/threshold check.
- **DB schema**: `backend/harpia/conboard.harpia` trigger/output_action
  entities have `optional int b0/b1/b2` (lines ~66-68, ~83-85) — no
  byte-array/blob field exists. Adding SysEx support means a real `.harpia`
  schema change (new field, regenerated `backend/generated/` — see
  `backend/harpia/README.md`'s authoring constraints before touching this).
- **Wire envelope (the risky one)**: `INTERFACE.md` §2 — the ZMQ io/heartbeat
  legs are **plain text**, fields split on `"; "` by the hand-rolled
  `explode()` parser. MIDI data bytes are 0–127 (0x00–0x7F) by spec, and
  `';'` is 0x3B (59), `' '` is 0x20 (32) — **both are valid MIDI data-byte
  values that can legitimately appear inside a real SysEx payload.** Sending
  raw SysEx bytes through the current text envelope unescaped **will**
  eventually corrupt framing on some real payload, not just in theory. This
  needs an encoding (hex or base64) for the SysEx payload specifically —
  don't reuse the raw-bytes approach the rest of the envelope uses.
- **Rule editor UI**: the console's MIDI rule editor
  (`frontend/console/src/` — search for where `b0`/`b1`/`b2` are edited)
  authors 3-byte triggers/outputs today; SysEx needs its own input mode (a
  hex-string field, most likely, with basic validation that it starts
  `F0`/ends `F7`).

## Suggested scope for a first landing (don't try to do all of the above at once)

Given the size, consider splitting further once you're in it rather than
committing to all five layers up front:

1. **Decide the matching model first** — this is a design decision, not
   implementation, and it changes what the schema/UI need to hold:
   - Exact-match only (simplest: store the full expected byte sequence,
     compare exactly) — covers "send this specific dump request" and
     "recognize this specific reply," probably the more common real use case
     for a control surface (vs. general SysEx routing).
   - Prefix-match (manufacturer ID + partial payload) — more flexible,
     more UI/schema complexity (need a "match length" or wildcard concept).
   - Recommend starting exact-match-only; prefix-match can be a later
     addition to the same feature if it turns out to matter.
2. **Wire format**: hex-encode the SysEx payload as a new field on the
   existing envelope (e.g. `<uuid>; <DevName>; <action>; sysex=<hexstring>; v0;`
   or similar — keep it additive per `INTERFACE.md`'s own stated rules of
   engagement: "additive change is cheap ... no ack needed" since there's
   effectively one session now, but still worth stating explicitly in that
   ledger for the record). Confirm real-world SysEx sizes from your hardware
   before picking a max length / deciding whether to worry about ZMQ message
   size limits.
3. **Firmware**: extend `midiActions`/`Actions` (`actions.h`) with an
   optional `std::vector<char>` (or similar) SysEx payload alongside the
   existing `midiSignal`, gated by a new `midi_mode` enum value (e.g.
   `midi_sysex`) so existing 3-byte rules are completely untouched. Extend
   `MIDI::in_func()` (`midithread.cpp`) to accumulate a SysEx message across
   multiple `snd_rawmidi_read()` calls if needed (a SysEx message spanning
   more than one 256-byte `buf` read is a real case to handle, not an edge
   case — patch dumps are often >256 bytes).
4. **Schema + backend**: add the field to `backend/harpia/conboard.harpia`,
   regenerate (`backend/generate.sh`), bump the auth hash per
   `backend/README.md`'s convention (see how `hmi_binding`'s addition did
   this, `docs/NEXT-SESSION.md`'s "Endpoints" section for the current hash).
5. **Console UI**: minimal hex-input field for authoring a SysEx
   trigger/output, gated behind MIDI device type same as `b0`/`b1`/`b2` today.
6. **Hardware-verify**: a real round trip — a physical controller/device that
   emits SysEx you can capture (even a simple "identity request/reply"
   SysEx exchange, which most MIDI gear supports, is a good minimal test that
   doesn't need a full patch-dump-capable device).

## Done criteria

- At least one real SysEx message (from actual connected MIDI hardware, not
  a synthetic test) round-trips: recognized as a trigger, and/or emitted as
  an output action.
- Existing 3-byte MIDI rules (`b0`/`b1`/`b2` normal/higher/lower/spot/blink)
  completely unaffected — regression-run `./run-tests.sh midi` and re-verify
  an existing board profile (e.g. `boards/Dj4Mix.json`) still works.
- `INTERFACE.md` updated with the new envelope field (additive, per its own
  rules of engagement).
- `README.md`'s MIDI "What is Missing?" line updated once this lands.
