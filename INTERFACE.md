# INTERFACE.md — dispatcher ⇄ backend living ledger

> **What this is.** The shared, negotiated contract between the two work streams:
> - **HW/dispatcher session** — owns `LowLevel/` (realtime device path) and
>   `LowLevel/dispatcher/` (the ZMQ dispatcher + its Crow HTTP/WS surface).
> - **backend/UI session** — owns `backend/` (management API + Postgres rules library)
>   and `frontend/`.
>
> This file is a **living ledger, not a frozen spec.** We agree the slow-changing
> **framing** (envelope / sender-registration id / versioning) and let **payloads churn.**
>
> **Rules of engagement**
> - **Additive change is cheap** — add the field/route/command, note it in the Changelog,
>   proceed. No ack needed.
> - **Breaking change must be flagged** — add it under *Open items* marked `NEEDS ACK`,
>   and wait for the other session to acknowledge in git before relying on it.
> - **git history is the negotiation record.** Reference commits when you change this file.
> - The eventual `.harpia` message definitions are the concrete form of the agreed
>   **payloads**; harpia will generate the (de)serialization that replaces today's
>   hand-rolled string parsing. **This is not designed yet** — blocked on harpia's
>   consumer docs (`../harpia/docs/consumer/`) which do not exist as of v0. Until then
>   the current string framing below is **frozen as v0** and only payloads move.

Status: **v0 — as-built snapshot + proposals.** Nothing here has been acked by the
other session yet; items needing agreement are marked `NEEDS ACK`.

---

## 1. Topology

```
  physical device
       │  (evdev / ALSA — realtime, out of scope for this ledger)
       ▼
  device handler process        LowLevel/{MIDI,Joystick,KeyBoard,Mouse}
  (conMIDI / conJoyS / …)        via LowLevel/Common  →  zmq_coms
       │
       │  ZMQ  (3 REQ/REP legs — §2)          ← the seam this session owns
       ▼
  dispatcher                     LowLevel/dispatcher
       │
       │  Crow HTTP + WebSocket  (§3)         ← the negotiated dispatcher⇄backend seam
       ▼
  backend / frontend             backend/  (mgmt API + Postgres library — sibling session)
```

Two distinct wire surfaces:
- **§2 device ⇄ dispatcher (ZMQ)** — internal to `LowLevel/`, but documented here
  because it is the source of the events that flow out §3, and it is harpia's first
  codegen target.
- **§3 dispatcher ⇄ backend (Crow HTTP/WS)** — **the shared negotiation surface.**

---

## 2. Device ⇄ dispatcher — ZMQ (framing = FROZEN v0)

Three independent REQ/REP socket pairs. The **dispatcher binds (REP)**; each **device
handler connects (REQ)**. Addresses/ports come from
`LowLevel/dispatcher/assets/config.json`:

| Leg | Role | Dispatcher bind (default) | Config key |
|-----|------|---------------------------|------------|
| register | allocate sender id | `tcp://127.0.0.1:5551` | `zmq_un` |
| io | device → dispatcher user-action reports | `tcp://127.0.0.1:5552` | `zmq_io` |
| heartbeat | liveness + downstream commands | `tcp://127.0.0.1:5550` | `zmq_hb` |

**Envelope (all three legs).** Plain text, fields separated by `"; "` (semicolon; the
parser `explode()` strips all spaces), usually a trailing `;`. **Field 0 is the
sender-registration id** on every leg *except* the initial register request and except
for the version token added by O2 below. There is still no length prefix or
message-type tag.

**Envelope version (O2, resolved 2026-08-12).** io and heartbeat, both directions,
now carry a literal `v0` field
(`CONBOARD_ENVELOPE_VERSION`, `LowLevel/Common/include/envelope_version.hpp`,
shared by `zmq_coms.cpp` and `dispatcher.cpp`) — but **trailing, not leading**.
Field 0 stays the uuid on every leg exactly as before; `v0` is appended as the
*last* field instead. This was a deliberate correction during verification, not
the original design: an earlier version of this change *did* lead with `v0`, which
silently broke live uuid matching. Root cause, found and fixed the same session: the
shared `explode()` helper calls `std::remove(token.begin(), token.end(), ' ')` to
strip spaces but never follows it with the matching `erase()`, so `std::remove`'s
"new logical end" is never actually applied — every token *after the first* (i.e.
every one that had a leading space from `"; "`) keeps its original length with a
duplicated trailing character (`"OK"` parsed back out as `"OKK"`, a uuid gains one
extra trailing char, etc.), which breaks exact-match comparisons on it. Leading the
frame with `v0` pushed the uuid from field 0 (immune -- nothing before it to leave a
stray space) to field 1 (corrupted). Fixed properly at the source in both
`explode()` copies (`token.erase(remove(...), token.end())`), but the version field
was left trailing anyway since there's no upside to moving the uuid back off field 0
now that other things already key off that position. **This also means
heartbeat-delivered `reload`/`file`/`outstop` commands (§2.3) were silently
non-functional before today** — the exact-match compares in
`DeviceEngine::coms_handler()` against a corrupted token never matched anything, so
no queued command ever actually fired (harmless for `"OK"`, silently broken for
real commands). Not a regression from this session's work; O2 testing is what
surfaced it. **Deliberately excluded from `v0`: the register leg (§2.1)** — its raw,
undelimited-bytes wire shape is what the devname-corruption bug (fixed 2026-08-12,
see `docs/NEXT-SESSION.md`) depended on diffing correctly; adding a version field
there means changing that shape, which is a separate decision.

### 2.1 register (`zmq_un`) — the sender-registration id
- **device → dispatcher:** the raw `DevName` string (no envelope, no delimiter).
- **dispatcher → device:** `<uuid>; <devname>; `
- The device keeps **field 0** as its **uuid** — the stable addressing token used on
  every other leg and in the backend command API (§3). The dispatcher records
  `devices[uuid] = devname`.
- **The id is a UUID** (libuuid `uuid_generate`/`uuid_unparse`, canonical 36-char form),
  e.g. `550e8400-e29b-41d4-a716-446655440000`. (Memory/harpia call this the "unique
  number assigned to a sender when it registers"; concretely it is this UUID string.)

### 2.2 io (`zmq_io`) — user-action reports  *(payload churns)*
- **device → dispatcher:** `<uuid>; <DevName>; <action>; v0;`
- **dispatcher → device:** `OK` (trivial ack, not itself versioned)
- Dispatcher stores the latest as `(uuid → action)` and keeps a history ring
  (`zmq_io.history`, default 100). The `<action>` string is an **opaque device-produced
  payload** (e.g. conMIDI's `ar_str()` report) — **this is the churning payload**, not
  framing.
- **O4 relieved (2026-08-12).** Device side buffers reports in an in-process queue,
  `STACKED_IO_MSG` (`LowLevel/Common/include/zmq_coms.hpp`), raised `10` → `64`. On
  overflow `zmq_coms::dispatch()` now evicts the **oldest** queued report to make room
  for the new one (previously rejected the newest) — a live monitor cares about current
  state more than a complete backlog, so this keeps the stream from lagging further
  behind during a sustained burst. Still reporting-path only; the action/command path is
  unaffected. The overflow log (`DeviceEngine::report()`) is now rate-limited to once/sec
  instead of once per dropped report.

### 2.3 heartbeat (`zmq_hb`) — liveness + commands  *(command vocab churns)*
- **device → dispatcher:** `<uuid>; <DevName>; v0;`
- **dispatcher → device:** `<uuid>; OK; v0;`  (nothing pending)
  or  `<uuid>; <cmd>; <params>; v0;`  (a queued command)
- The device **filters the reply by matching field 0 to its own uuid**.
- Commands are queued by the backend via `POST /iocommand` (§3) and delivered on the
  device's next heartbeat. Current command vocabulary (**payload**):
  - `reload`
  - `file <absolute path>`   — e.g. `file /conboard/boards/Arduino Micro.json`
  - `outstop`
  - `change_mode <mode>`

---

## 3. Dispatcher ⇄ backend — Crow HTTP + WebSocket  *(the negotiated seam)*

The dispatcher runs an embedded **Crow** app. Routes (`LowLevel/dispatcher/src/main.cpp`):

| Method | Route | Body | Response | Purpose |
|--------|-------|------|----------|---------|
| GET  | `/config`        | — | JSON string of dispatcher config | read config |
| POST | `/iocommand`     | `{"UUID":"<uuid>","params":["cmd","arg",…]}` | `{"result":<0\|1>}` (400 on bad body) | queue a device command (delivered via §2.3) |
| POST | `/screencommand` | `{"UUID":"<uuid>","params":[…]}` | `{"result":<0\|1>}` | queue a screen command |
| WS   | `/ws`            | — | see below | live user-action stream |

**`/ws` payload.** On connect the dispatcher sends the action history as CSV:
```
UUID,UserAction\r\n
v0,<uuid>,<action>\r\n
…
```
then streams each new action as a single `v0,<uuid>,<action>` text frame. (It currently
also rebroadcasts any inbound WS frame to all clients — assume that is incidental and
subject to change.) The `v0,` envelope-version token (O2, resolved 2026-08-12) leads
every data row; the CSV header row stays unversioned since the console keys its
"skip this line" check off the literal `UUID,` prefix. The console strips the token
defensively, so an older dispatcher build that doesn't send it still parses fine.

**Proposed heartbeat/roster frame (`NEEDS ACK`, O5).** Action frames stay exactly as
above — their `<action>` payload already contains commas (e.g. conMIDI's `[b0,b1,b2]`),
so we do **not** add fields to them. Instead the dispatcher additionally emits, about
once a second, one **heartbeat frame per live sender**, distinguished by a literal `HB`
first token (now led by the `v0,` envelope-version token, O2):
```
v0,HB,<uuid>,<devname>\r\n
```
`<devname>` is the sender's registered `DevName` (§2.1), which contains no comma. This
one additive frame gives the console two things it can't derive today: the
**uuid → devname map** (so it can filter the single action stream per configured
device — items 2/3 of the console worklist) and **liveness** (a device is "connected"
iff an `HB` for it arrived within a few seconds → drives the per-device LED). The
console already parses defensively: a line whose first token is `HB` is a heartbeat, any
other line is a legacy/action frame, so shipping this is backward-compatible on the
console side.

**Deployment note.** `backend/assets/interface.conf` (nginx) proxies
`/websocket → localhost:40080` and `/ → localhost:8080`. The `:8080` upstream is the
**backend management API** (owned by the sibling session — CRUD over the Postgres rules
library + copy-A→B); it is a **separate surface** from this dispatcher seam and is out of
scope for this ledger except as context.

---

## 4. Framing agreement (the slow-changing part — keep stable)

1. **Sender-registration id.** The **UUID** allocated at register (§2.1) is *the* stable
   addressing token across all ZMQ legs and the backend command API (`POST` uses it).
   **Agreed to keep.**
2. **Surface shape.** 3 ZMQ REQ/REP legs (register / io / heartbeat-commands) + the Crow
   HTTP/WS surface. **Agreed to keep** the count and roles.
3. **Envelope, today.** Delimited text on ZMQ; JSON on HTTP; CSV/text on WS. **Frozen as
   v0** — this is exactly the layer harpia will replace. Payloads may churn under it.
4. **Envelope, target (post-harpia).** When harpia's consumer docs land and its C++
   target is confirmed usable, payloads become harpia-generated structs (JSON on HTTP/WS,
   harpia's transport on ZMQ) carrying an explicit **message-type id** and a **version
   tag**. Design deferred — see Open item O2.

---

## 5. Open items

- **O1 — HTTP port is inconsistent. RESOLVED (2026-08-11).**
  The dispatcher Crow app now reads its port from config
  (`dispatcher::GetHTTPPort()` → `app.port(dsp.GetHTTPPort())` in
  `LowLevel/dispatcher/src/main.cpp`) instead of hardcoding `40080`.
  `config.json`'s `http.port` was changed from `9080` to `40080` — settling on the
  value nginx (`interface.conf`) and every deployed board already use for
  `/websocket`, rather than the unused `9080`. The `configParser` default of `9999`
  is an unreached fallback (config.json always ships the key) and was left as-is.

- **O2 — Envelope version field. RESOLVED + HARDWARE-VERIFIED (2026-08-12).**
  Added a literal `v0` token (`CONBOARD_ENVELOPE_VERSION`,
  `LowLevel/Common/include/envelope_version.hpp`, shared by device handlers and the
  dispatcher) to every leg that already carries structured, delimited payload: io and
  heartbeat on ZMQ (both directions — **trailing** field, see §2.2/§2.3 for why not
  leading), and the dispatcher's `/ws` output (action rows and the `HB` roster frame —
  leading `v0,`, §3, safe there since the frontend does plain JS string splitting, not
  the buggy C++ `explode()`). WS-side receivers strip the token defensively (tolerating
  an unversioned sender); the ZMQ legs don't need to since the uuid position didn't
  move. **Deliberately left unversioned: the register leg** (§2.1, raw undelimited
  bytes — versioning it means changing that wire shape, a separate decision) **and the
  io leg's trivial `OK` ack** (no structured content to key a version off).
  **Found and fixed along the way**: a real, previously-unknown bug in the shared
  `explode()` parser (see §2 preamble) that silently broke heartbeat-delivered
  `reload`/`file`/`outstop` commands — unrelated to O2 itself, but surfaced by it.
  Verified live on `192.168.7.4`: connected directly to `/ws` (port `40080`) with the
  real wireless keyboard+mouse combo running — `v0,HB,<uuid>,<devname>` frames stream
  once/sec for both devices, and typing on the physical keyboard produced
  `v0,<uuid>,[1,<code>,<press/release>]` action frames, confirming both the ZMQ-side
  fields (uuid stayed matchable) and the WS-side version token work correctly
  end-to-end.

- **O3 — WS action payload is opaque CSV.** `<uuid>,<action>` where `<action>` is a raw
  device string. As typed rules/reports arrive this **will churn** — treated as payload,
  not framing. No ack needed; noted so the frontend doesn't hardcode a shape.

- **O4 — Reporting-queue overflow. RELIEVED + DEPLOYED (2026-08-12).** `STACKED_IO_MSG`
  (`LowLevel/Common/include/zmq_coms.hpp`) raised `10` → `64`, and `zmq_coms::dispatch()`
  now evicts the oldest queued report on overflow instead of rejecting the newest — see
  §2.2 for detail. Reporting path only, doesn't touch the action/command path. This is
  relief, not a structural fix: a *sustained* burst (not just a short spike) can still
  overflow a bounded queue fed one round-trip at a time; revisit with real pipelining
  (batching, or a non-REQ/REP transport) when building MIDI→keystroke/text rules if that
  turns out to matter in practice. Deployed to `192.168.7.4` alongside O2; not
  separately load-tested against a real burst (no easy way to generate one from the
  wireless keyboard/mouse combo used for this session's hardware verification).

- **O5 — Heartbeat/roster frame on `/ws`. RESOLVED + HARDWARE-VERIFIED (2026-08-12).**
  `dispatcher::GetHeartbeats()` (`LowLevel/dispatcher/src/dispatcher.cpp`) builds one
  `HB,<uuid>,<devname>\r\n` line per device whose last ping (io or heartbeat leg) is
  within `HeartbeatLiveWindowSec` (5s), reusing the existing `devices`/`last_ping` maps
  — no new state. `user_handler` (`main.cpp`) broadcasts that string to all `/ws`
  clients about once a second, alongside the existing action-frame broadcast. Verified
  live on `192.168.7.4` connecting straight to `/ws` (not yet through the console UI
  itself, but the console parses this exact stream) — one `HB` line per second for both
  the `WirelessKB` and `WirelessMouse` uuids, continuously, over multiple checks.

---

## 6. Changelog

- **v0** — HW/dispatcher session. Initial as-built snapshot of both wire surfaces
  (§2 ZMQ, §3 Crow HTTP/WS), the framing agreement (§4), and open items O1–O4. Nothing
  acked by the backend/UI session yet.
- **v0.1** — backend/UI session. Added **O5**: proposed the additive `HB,<uuid>,<devname>`
  heartbeat/roster frame on `/ws` (§3) so the console can do device-centric live
  filtering + heartbeat LEDs. Console consumes it with fallback; awaiting dispatcher ack.
- **v0.1 (milestone `2026-08-10`)** — the console/backend side of everything referenced
  above (deploy/undeploy, the O5-consuming live monitor, device inventory) is merged to
  `main`. No wire-format change since v0.1; O1 and O5 are still open on the dispatcher
  side.
- **v0.2 (2026-08-11)** — **O1 resolved**: dispatcher HTTP port is now config-driven,
  settled on `40080`. O5 (heartbeat/roster frame) still open on the dispatcher side.
- **v0.3 (2026-08-12)** — HW/dispatcher session. **O5 resolved**: dispatcher now emits
  the `HB,<uuid>,<devname>` roster/heartbeat frame on `/ws` ~1/s per live sender.
  Console side needs no change (already consumed this format). Not yet verified
  end-to-end against real hardware.
- **v0.4 (2026-08-12)** — HW/dispatcher session. **O2 resolved + hardware-verified**:
  added the `v0` envelope-version token to io/heartbeat (both directions, ZMQ,
  trailing field) and `/ws` output (action rows + `HB` frame, leading field);
  registration leg and the io ack intentionally left unversioned (see O2). **O4
  relieved + deployed**: `STACKED_IO_MSG` `10`→`64` plus drop-oldest overflow policy.
  Console (`frontend/console/src/model/events.ts`) updated to strip the version token,
  tolerating dispatcher builds that predate it. **Two bugs found and fixed along the
  way** (see O2): (1) a heartbeat-reply construction bug in
  `dispatcher::th_heart_beat()` that sent queued commands to devices as empty strings
  instead of the real command; (2) a missing `erase()` after `remove()` in the shared
  `explode()` parser (both copies) that silently corrupted every non-first token,
  which meant heartbeat-delivered `reload`/`file`/`outstop` commands never actually
  fired, for any device, ever — root-caused by isolated reproduction
  (`std::remove` compacts but doesn't shrink) before being fixed at the source.
  Deployed to `192.168.7.4` and verified live: `HB` frames streaming correctly for
  both `WirelessKB`/`WirelessMouse`, and real physical keystrokes producing correctly
  versioned action frames on `/ws`.
- **Dispatcher SIGABRT-on-stop. FOUND + FIXED + HARDWARE-VERIFIED (2026-08-12,
  same session).** `dispatcher.service` was throwing `std::system_error("Invalid
  argument")` and getting SIGABRT'd on every stop during
  `install-on-device.sh`'s "stopping any running conboard services" step (seen
  repeatedly in `journalctl -u dispatcher.service` across multiple reinstalls this
  session). Exactly the same class of double-join bug already fixed in
  `zmq_coms::die()` for `conKeyB`/`conMouse`: `main.cpp` calls `dsp.die()`
  explicitly before returning, then `~dispatcher()` calls `die()` again as the
  stack-allocated `dsp` goes out of scope, double-joining `hb`/`th_unuique_numb`/
  `io` — undefined behaviour that libstdc++ turns into exactly this crash. Fixed
  the same way: `joinable()` guards on each `.join()` in `dispatcher::die()`
  (`LowLevel/dispatcher/src/dispatcher.cpp`). Verified on `192.168.7.4`: two
  consecutive `install-on-device.sh` reinstalls against the fixed binary both show
  `Deactivated successfully` in the journal, no `terminate`/`ABRT`, no
  `system_error`.
