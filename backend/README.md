# conboard backend (C++)

Connects the **user** (frontend console) to the device. It does **not** handle realtime
device IO (that is `LowLevel/`); it exposes management APIs over the harpia-generated
rules-library and relays the dispatcher's event stream to the frontend.

Most of the code is **generated from `../harpia`** (a black box — see
[`harpia/README.md`](harpia/README.md) and `../harpia/USAGE.md`): message structs,
JSON, a **SOCI**-backed CRUDL DAO, REST (Crow) and gRPC, plus the SQL schema. This
module is the thin host that mounts those surfaces. **The DB client is SOCI, not
libpqxx.**

## Layout
- `harpia/` — the `.harpia` domain spec (the one artifact handed to harpia).
- `generate.sh` — regenerate `generated/` from `harpia/` (SQLite dialect by default).
- `generated/` — the harpia output, **committed** so conboard builds without the harpia
  repo present. **Never hand-edit**; regenerate.
- `src/` — the app host: `main.cpp` (servers) + `entity_*.cpp` (one TU per harpia
  entity, see `include/conboard_entities.h` for why) + two hand-written (non-harpia)
  files: `deploy.cpp` (Axis C deploy/undeploy) and `devices.cpp` (device inventory,
  links the shared `deviceDetect`/condetect classifier from `LowLevel/Common`).
- `db/` — the SQLite rules-library notes + runtime env template.
- `assets/backend.service` — systemd unit.

## Architecture (three separate concerns)
1. **Persistence / DB** — an embedded **SQLite** file (the rules/actions **library**):
   the source of truth for *authoring* and *portability* (create/edit rules, **copy a
   rule set from one device to another**). **NOT** in the realtime path — hardware runs
   from `boards/*.json`, not from live DB queries. Device-local, single-writer,
   authoring-only, so SQLite (one bundled `libsqlite3.so`, no server/daemon, no
   apt/docker on the device) is the portable fit. The schema is **harpia-generated**;
   tables are created at boot via the DAO `create_table()`.
2. **Management API** — REST (Crow) + gRPC over the generated DAOs. Serves the frontend.
3. **Realtime event stream** — `LowLevel/dispatcher` (ZMQ) speaks its own websocket
   directly (`INTERFACE.md`, repo root); nginx proxies `/websocket` straight to it
   (`127.0.0.1:40080/ws`), so the backend is not in this path at all. An earlier design
   had the backend relay that stream itself over its own `/ws` — never implemented (no
   ZMQ consumer) and removed (2026-08-13) once the direct-proxy path was hardware-
   verified working end to end.

> A central **PostgreSQL** authoring host is still possible later — harpia emits it with
> `HARPIA_DB_BACKEND=postgresql backend/generate.sh` and the C++ API is identical. Not
> used on-device.

## Endpoints
REST base `/api/v1` (configurable):
- `GET/POST/PUT/DELETE /api/v1/{board,mode,rule,trigger,output_action}[/<id>]`
- `POST /api/v1/deploy` — **Axis C**: deploy an authored profile to the realtime path.
  Body is a board in the `boards/*.json` shape (what the frontend `Board` model
  serializes to). Writes it into `CONBOARD_BOARDS_DIR` (default `/conboard/boards`),
  **overwriting the profile whose `header.identifier.tags` + `DEVICE.type` match**
  (the launcher itself binds a device to a profile by tags alone, not filename — but
  deploy also folds in `DEVICE.type` so that a composite USB device exposing multiple
  functional interfaces under one shared VID/PID, e.g. a keyboard+mouse combo
  receiver, gets a separate on-device file per profile instead of the second deploy
  silently overwriting the first), then reloads the handler via
  `CONBOARD_RELOAD_CMD` (default: the udev coldplug replay the installer uses). Returns
  `{"written":"<path>","reloaded":<bool>}`. Hand-written (not harpia-generated).
- `POST /api/v1/undeploy` — inverse of deploy: removes the tag-matched on-device
  profile and stops (+ removes) its handler systemd unit(s) — the exact handler unit
  and any evdev `<base>-<id>` variants — so the hardware stops. Returns
  `{"removed":"<path>","stopped":<bool>}`. Hand-written, `backend/src/deploy.cpp`.
- `GET /api/v1/devices` — device inventory for the console's add-device flow: lists
  attached USB/input devices, classifies each with the same condetect classifier the
  launcher uses (via libudev), and marks whether a `boards/*.json` profile already
  matches it (`designated`). Hand-written, `backend/src/devices.cpp`.
- `GET /healthz`

MIDI triggers also carry an optional **operation mode** (`midiMode` on the wire,
`mm_normal`/`mm_trigger_higher`/`mm_trigger_lower`/`mm_spot`/`mm_blink` — mirrors the
firmware's `midi_action_mode`; `mm_normal` is the omitted zero-value). Added to
`backend/harpia/conboard.harpia`; regenerating bumped the domain hash to
`1bf812ac18b80d4a5ea4d51e6bfb7f58`.

Every generated REST route is **credential-gated**: requests must carry
`X-User: <entity>` and `X-Pswd: <hash>` (the hash is the compile-time md5 of the domain,
not a secret — a real credential was meant to layer in front, see the power-password
design below). gRPC mirrors this via `x-user`/`x-pswd` metadata.

**That front layer, until power-password exists**: `backend/assets/interface.conf`
puts nginx Basic Auth in front of the whole site (console + `/api/v1` + `/websocket`,
`/healthz` excepted) — `install-on-device.sh` generates a random per-install password
into `/etc/conboard-web-password.txt` (root-only) on first install -- retrieve or
rotate it any time with `sudo conboard-password` / `sudo conboard-password --reset`
(`docker/assets/conboard-password.sh`, installed to `/usr/local/bin`), so losing the
password is never a real lockout. It's a stand-in,
not the designed experience below (no rotating password, no on-screen prompt, no
lockout escalation) — that design stays unimplemented, tracked in
[../NOTES.md](../NOTES.md).

## Runtime config (env)
| var | default | meaning |
|---|---|---|
| `CONBOARD_DB` | `conboard.db` | SQLite file path |
| `CONBOARD_HTTP_HOST` | `127.0.0.1` | REST bind host (nginx proxies) |
| `CONBOARD_HTTP_PORT` | `8080` | REST port |
| `CONBOARD_GRPC_ADDR` | `127.0.0.1:50051` | gRPC listen address |
| `CONBOARD_API_BASE` | `/api/v1` | REST base path |

See `db/backend.env.example`.

## Build
CMake project; consumes `generated/` (include `generated/cpp`, compile the protobuf
`.pb.cc`, vendor crow/asio/tinyxml2, link `soci_core`+`soci_sqlite3`+`sqlite3`,
protobuf, gRPC). Build/verify in the `harpia-build` image (has all deps); cross-compile
for the board via `build-cross.sh` (the cross image installs the same deps, and the
runtime `.so`s are bundled into the tarball — no apt on the device).

```sh
cmake -S backend -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
```

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
