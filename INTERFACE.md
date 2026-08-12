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
sender-registration id** on every leg *except* the initial register request. There is
**no length prefix, no message-type tag, and no version field** today.

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
- **device → dispatcher:** `<uuid>; <DevName>; <action>; `
- **dispatcher → device:** `OK`
- Dispatcher stores the latest as `(uuid → action)` and keeps a history ring
  (`zmq_io.history`, default 100). The `<action>` string is an **opaque device-produced
  payload** (e.g. conMIDI's `ar_str()` report) — **this is the churning payload**, not
  framing.
- ⚠️ Device side buffers reports in an in-process queue bounded by `STACKED_IO_MSG = 10`
  (`LowLevel/Common/include/zmq_coms.hpp`); bursts overflow and **drop reports**
  (reporting path only — does not affect the action/command path). Deferred; see project
  memory `conboard-dispatch-overflow`.

### 2.3 heartbeat (`zmq_hb`) — liveness + commands  *(command vocab churns)*
- **device → dispatcher:** `<uuid>; <DevName>; `
- **dispatcher → device:** `<uuid>; OK;`  (nothing pending)
  or  `<uuid>; <cmd>; <params>; `  (a queued command)
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
<uuid>,<action>\r\n
…
```
then streams each new action as a single `<uuid>,<action>` text frame. (It currently
also rebroadcasts any inbound WS frame to all clients — assume that is incidental and
subject to change.)

**Proposed heartbeat/roster frame (`NEEDS ACK`, O5).** Action frames stay exactly as
above — their `<action>` payload already contains commas (e.g. conMIDI's `[b0,b1,b2]`),
so we do **not** add fields to them. Instead the dispatcher additionally emits, about
once a second, one **heartbeat frame per live sender**, distinguished by a literal `HB`
first token:
```
HB,<uuid>,<devname>\r\n
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

- **O2 — No envelope version field. `NEEDS ACK`.**
  Nothing in §2/§3 carries a version. **Proposal (additive):** introduce a `v` field in
  the envelope so v0↔v1 can be distinguished when harpia lands. Cheap to add now; flagged
  because both sides must agree where it lives.

- **O3 — WS action payload is opaque CSV.** `<uuid>,<action>` where `<action>` is a raw
  device string. As typed rules/reports arrive this **will churn** — treated as payload,
  not framing. No ack needed; noted so the frontend doesn't hardcode a shape.

- **O4 — Reporting-queue overflow (`STACKED_IO_MSG = 10`).** Known, deferred; reporting
  path only. Revisit when building MIDI→keystroke/text rules. See memory
  `conboard-dispatch-overflow`.

- **O5 — Heartbeat/roster frame on `/ws`. RESOLVED (2026-08-12).**
  `dispatcher::GetHeartbeats()` (`LowLevel/dispatcher/src/dispatcher.cpp`) builds one
  `HB,<uuid>,<devname>\r\n` line per device whose last ping (io or heartbeat leg) is
  within `HeartbeatLiveWindowSec` (5s), reusing the existing `devices`/`last_ping` maps
  — no new state. `user_handler` (`main.cpp`) broadcasts that string to all `/ws`
  clients about once a second, alongside the existing action-frame broadcast. Verified
  via a full `./build-cross.sh zero3` (compiles clean under the real arm64 toolchain);
  not yet exercised against a live console + real device on hardware — do that before
  fully trusting the liveness LEDs.

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
