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
- `src/` — the app host: `main.cpp` (servers) + `entity_*.cpp` (one TU per entity,
  see `include/conboard_entities.h` for why).
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
3. **Dispatcher seam** — the realtime event stream from `LowLevel/dispatcher` (ZMQ),
   relayed to the frontend over the `/ws` websocket. The ZMQ consumer is still TODO;
   the contract is `INTERFACE.md` (repo root).

> A central **PostgreSQL** authoring host is still possible later — harpia emits it with
> `HARPIA_DB_BACKEND=postgresql backend/generate.sh` and the C++ API is identical. Not
> used on-device.

## Endpoints
REST base `/api/v1` (configurable):
- `GET/POST/PUT/DELETE /api/v1/{board,mode,rule,trigger,output_action}[/<id>]`
- `GET /healthz`
- `GET /ws` — websocket, dispatcher event relay (seam; consumer TODO)

Every generated REST route is **credential-gated**: requests must carry
`X-User: <entity>` and `X-Pswd: <hash>` (the hash is the compile-time md5 of the domain,
not a secret — conboard's own auth, the power-password design below, layers in front).
gRPC mirrors this via `x-user`/`x-pswd` metadata.

## Runtime config (env)
| var | default | meaning |
|---|---|---|
| `CONBOARD_DB` | `conboard.db` | SQLite file path |
| `CONBOARD_HTTP_HOST` | `127.0.0.1` | REST/ws bind host (nginx proxies) |
| `CONBOARD_HTTP_PORT` | `8080` | REST/ws port |
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
