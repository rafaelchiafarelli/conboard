# conboard — next-session handoff (milestone `2026-08-10`)

`main` = the full console-fixes stack (console-quickwins → deploy-regression →
device-inventory → live-monitor → console-fixes) merged in, plus a Blender-control
investigation's salvageable bits (DJTech-4-Mix.csv, boards/blender.cmds, DevInspector).
Tagged `milestone-2026-08-10`. Pushed to `origin/main`. Full history is normalized to
one git identity (`rafael.chiafarelli@gmail.com`) — a mixed-identity repo was the
original trigger for that cleanup; a `backup/blender-investigation-2026-08-10` branch
holds the raw pre-cleanup local work in case anything in it is ever worth a second look.

See the top-level [README.md § What's built](../README.md#whats-built) for the feature
list. This file is the **live punch list** — what's known broken or unverified.

## Open bugs

- **`uninstall-on-device.sh --purge` is not fully reliable** (reported after installing
  this milestone on real hardware). Not yet root-caused — start by comparing what
  `--purge` actually removes (`docker/uninstall-on-device.sh`) against what
  `install-on-device.sh` and the launcher actually create (stale per-device
  `.service` units, `/conboard/backend/data` SQLite file, `boards/*.json`). Get a
  precise repro (what's left behind, on a board that had *what* installed) before
  changing anything.

## Still needs a dispatcher-side change

Both are `NEEDS ACK` in `INTERFACE.md` §5, confirmed still unimplemented (`grep '"HB'
LowLevel/dispatcher/src/` finds nothing; `app.port(40080)` is still hardcoded):

- **O1 — HTTP port inconsistency.** Dispatcher hardcodes `app.port(40080)`;
  `config.json` says `9080`. Nginx's `/websocket` proxy matches only the hardcode by
  luck. Make the dispatcher read its port from config and settle on one value.
- **O5 — heartbeat/roster frame.** The console's live view wants a
  `HB,<uuid>,<devname>` frame ~1/s per live sender (device-name map + liveness for the
  per-device LEDs and live filtering). Console consumes it with graceful fallback
  today (shows raw uuids). Needs the dispatcher to start emitting it.

## Still needs on-board verification

- **evdev hardware test** (conJoyS/conKeyB/conMouse) — built + unit-tested, never
  exercised on real hardware. Runbook: `docs/HW-TEST-evdev.md`.
- **Delete/undeploy round-trip** on a clean install: delete a device in the console →
  confirm the backend row is gone (204) **and** the on-device profile + handler unit
  are gone (`POST /undeploy`, `backend/src/deploy.cpp`).
- **DJ-Tech-4-Mix events reach the monitor** — the conMIDI open-retry fix
  (`LowLevel/Common/include/runDevice.hpp`) should make the handler recover from the
  transient ALSA-port-busy failure that used to leave it inert after a redeploy
  restart; confirm on a board.

## How to build / install / deploy / test

```sh
./build-cross.sh zero3            # -> dist/zero3/conboard-zero3.tar.gz

# on the board (no apt, no compile; bundled libs; installs nginx site if nginx present)
scp dist/zero3/conboard-zero3.tar.gz <board>:~
tar xzf conboard-zero3.tar.gz && cd conboard
sudo ./uninstall-on-device.sh --purge   # clean slate recommended after a schema/DB change
sudo ./install-on-device.sh

# verify
curl -s localhost:8080/healthz   # backend direct
curl -s localhost/healthz        # via nginx (same origin as UI)
open http://<board-ip>/          # the console
sudo journalctl -u backend -f    # NOTE: needs sudo (backend runs as root)
```

## Endpoints

`/api/v1/{board,mode,rule,trigger,output_action}[/<id>]` · `POST /api/v1/deploy` ·
`POST /api/v1/undeploy` · `GET /api/v1/devices` · `GET /healthz` · `GET /ws` (backend
relay seam, unused). nginx serves the console at `/` and proxies `/websocket` →
dispatcher `:40080`. REST is credential-gated (`X-User: <entity>`, `X-Pswd: <hash>`);
hash = `1bf812ac18b80d4a5ea4d51e6bfb7f58` (bumped when `midi_mode` was added to the
trigger message; regen via `backend/generate.sh`).

## Don't-relearn facts

- **harpia is a black box.** Regenerate: `backend/generate.sh` (SQLite default;
  `HARPIA_DB_BACKEND=postgresql` for a future central host). Never hand-edit
  `backend/generated/`. `.harpia` authoring constraints (enums in root file, must import
  an `Include/` module, punctuation-plain ASCII comments) — see `backend/harpia/README.md`.
- **`backend/generated/` IS committed source, not build output — do not gitignore
  it.** It's regenerated wholesale (harpia cleans the output dir each run), so a
  half-regenerated tree (old-hash files still committed alongside new ones) causes
  duplicate-symbol link errors. If you regenerate, `git status` should show the old
  hash's files as deleted and the new hash's as added — commit both sides together.
- **Realtime path** runs from `/conboard/boards/*.json`, decoupled from the DB by design;
  the launcher matches a device to a profile by `header.identifier.tags`.
- **Build**: emulated arm64 via QEMU; backend build `-j` capped (`BACKEND_JOBS`,
  currently 8 — many-core hosts OOM the emulated protobuf/gRPC compiles at higher
  values → exit 126).
- **CRLF**: `.gitattributes` forces LF; a fresh Windows/WSL clone must renormalize
  (`git config core.autocrlf false && git rm --cached -r . && git reset --hard`).
- Frontend↔harpia JSON: camelCase fields, `ID<hash>` caller-assigned PK unique per table,
  enums as names, zero values omitted. Mapping in `frontend/console/src/api/{harpia,map,client}.ts`.
