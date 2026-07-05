# conboard backend (C++)

> **Status: structural skeleton (2026-06-30).** The python/Flask backend was retired
> here; this is the target C++ layout. No real implementation yet — most code is to be
> **generated from `../harpia`** (message structs + JSON + DB schema + ZMQ transport).
> See `frontend/proposal.md` and the project memory `conboard-rules-db-architecture`.

This module connects the **user** to the device. It does **not** handle the realtime
device IO (that's `LowLevel/`), and it does **not** own the websocket transport
itself — it consumes the dispatcher's event stream and exposes management APIs.

## Architecture (three separate concerns)

1. **Persistence / DB** (`db/`) — a **PostgreSQL** database holding the rules/actions
   **library**. This is the source of truth for *authoring* and *portability*
   (create/edit rules, **copy a rule set from one device to another**). It is **NOT**
   in the realtime execution path — hardware runs from `boards/*.json`, not from live
   DB queries. The schema is **harpia-generated** (a rule = a harpia message → C++
   struct + JSON + DB table/FK).
2. **Management API** (`src/`, `include/`) — HTTP/JSON over **Crow** (already a submodule
   at `LowLevel/Common/Crow`), DB access via **libpqxx**. CRUD over devices/profiles/
   rules/actions/modes, plus the cross-entity **copy A→B** operation. Serves the
   frontend.
3. **Dispatcher seam** — consumes the realtime event stream from `LowLevel/dispatcher`
   (ZMQ) and relays to the frontend (websocket). The message contract here is
   negotiated with the hw/dispatcher session (see "Interface" below).

## Build (planned)
CMake project linking `LowLevel/Common`, Crow, and libpqxx. Not yet wired into
`build-cross.sh`. `src/main.cpp` is a placeholder Crow app.

## Postgres (dev)
`docker/docker-compose.yml` brings up a local Postgres for development.
`db/connection.env.example` is the connection template (copy → `.env`, never commit
secrets). `db/schema/` holds harpia-generated DDL; `db/migrations/` holds ordered
bootstrap/migration SQL.

```bash
cd backend/docker && docker compose up -d   # local postgres on :5432
```

## Interface (cross-session, negotiated — not frozen)
The dispatcher↔backend message contract is negotiated with the hw/dispatcher session
via a shared living ledger (intended: `INTERFACE.md` at repo root, not yet created).
Agree the slow-changing **framing** (envelope / sender-registration id / versioning);
let **payloads** churn. Additive = cheap; breaking = flag + wait for ack.

---

## User-facing API design (carried over from the python prototype)

> Preserved from the original design so it isn't lost; to be (re)implemented in C++.

### User API
Information about the user and their activity (user *actions* are handled elsewhere).
- Get/Set user profile — name, pic, profile_pic, email, phone, occupation, education,
  description, share_status, visible_status, etc.
- User statistics — last connected, how long connected.
- User activities — last N user actions.

### Security API
Local-access control. To connect, the user enters a **power password shown on the
device screen**; a new password is shown on every attempt. If someone is operating the
screen when a login occurs, login takes precedence; **3 failed attempts** → login
shuts down, screen edge turns red, password gets harder. Reboot resets the count —
but after **10 failed attempts** the system refuses connections until a password is
entered **physically on the keyboard**. The password is sent to the connected user.
- Login with power password
- Send password to screen
- (optional) Email notification to the user

### Shared API
- Get shared files (share scope is per-file; track how many times shared)
- Share / UnShare a file

### Vault API
- Clear vault
- Add file to the vault
- Retrieve file from the vault
