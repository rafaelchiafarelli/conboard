# conboard — next-session handoff (2026-07-13)

All console-bug work is on branch **`integration/console-fixes`** (pushed). It stacks
the console fixes + item-8 backend regen + Cluster B/D + the live rework. Build it with
`./build-cross.sh zero3` (now `-j16` via `BACKEND_JOBS`).

## ⚠️ Do a CLEAN SLATE on the board before trusting anything
Stale on-device state was the real cause of "delete doesn't work / DJ-Tech stopped":
- The launcher writes one **auto-generated `<devname>.service`** per device. Nothing
  used to remove them, so a stale unit got `systemctl restart`ed with an old
  `ExecStart`/JSON → dead handler after re-install. **Fixed** — install + uninstall now
  wipe units whose `Description="auto generated service file."`.
- The **SQLite DB** (`/conboard/backend/data`) is preserved across installs by design;
  stale/duplicate rows linger. `uninstall --purge` wipes it.
- `boards/*.json` ARE reset by install (rsync `--delete` + the artifact ships the 3 real).

**Recovery procedure (run on the board):**
```sh
sudo ./uninstall-on-device.sh --purge      # program + DB + (now) stale per-device units
# rebuild the fresh artifact on the build host, copy it over, then:
sudo ./install-on-device.sh                # seeds a clean DB; launcher regenerates units
# verify no stale services survived:
grep -l "auto generated service file" /etc/systemd/system/*.service   # should print nothing
systemctl status "$(systemctl list-units --type=service | grep -i dj || true)"  # DJ-Tech unit healthy?
```

## Still needs on-board verification / a dispatcher change
- **Delete**: backend DELETE is correct (204) + frontend tombstone persists deletions;
  confirm on a CLEAN DB that delete removes the row and (via `/undeploy`) stops the
  handler. If a device still shows after delete+reload on a clean DB, capture
  `curl -s -X DELETE -H "X-User:board" -H "X-Pswd:<hash>" localhost:8080/api/v1/board/<id>`.
- **DJ-Tech events**: with clean services + the conMIDI open-retry fix, confirm it sends.
- **Live monitor sender name/type + per-device filtering + heartbeat LEDs**: these light
  up only once the dispatcher emits the **`HB,<uuid>,<devname>`** frame — INTERFACE.md
  **O5 (NEEDS ACK)**. The console consumes it with fallback today.
- **Live events** are now a permanent RIGHT column (all devices), not an overlay.

---

# conboard — earlier handoff (2026-07-10)

Quick-start for the session that will **fix the remaining console bugs**. Pairs with the
project memory `conboard-backend-wireup` (auto-loaded). Read this first, then the memory.

## TL;DR state
The full **frontend ↔ backend ↔ device** loop is built and on `main`:
- **Backend** (`backend/`): harpia-generated REST + gRPC over **SOCI/SQLite**, plus a
  hand-written `POST /api/v1/deploy` (Axis C). Runtime `.so` closure bundled into the
  tarball with `RUNPATH=/conboard/lib` → **no apt on the device**.
- **Console** (`frontend/console/`): loads boards from the backend, edits + **Save**s
  them, **board CRUD** (New/Copy/Delete), a **live monitor** off the dispatcher `/ws`,
  and **Deploy to device**. Served on-device by nginx (`interface.conf`).
- **Axis C**: deploy writes `/conboard/boards/<name>.json` (overwriting the profile
  whose `header.identifier.tags` match — the launcher matches by tags, not filename)
  and reloads via the installer's udev coldplug replay.

### ⚠️ Repo state — READ BEFORE PUSHING
- Local `main` = **`ec657d6`** (console fixes). `origin/main` = **`128c76e`**. The last
  commit is **unpushed** — GitHub was unreachable and the user said hold. It's a clean
  fast-forward: `git push origin main` when ready.
- **`dist/` is STALE** (2026-07-05, pre-console/pre-Axis-C — no console SPA, no `/deploy`).
  Rebuild before deploying: `./build-cross.sh zero3` (builds from the **local tree**, so
  no push needed; `-j` is capped to 4 for QEMU).

## Bugs from board testing (this is the next-session worklist)
| # | Issue | Status |
|---|---|---|
| 1 | New device was midi-only | ✅ FIXED — prompts type (midi/joystick/keyboard/mouse) |
| 2 | Live events show a UUID, not a device | ⚠️ PARTIAL — see below (dispatcher-side) |
| 3 | DJ-Tech events don't show in monitor | ⚠️ DIAGNOSE on board — see below |
| 4 | Rules/live panels not resizable | ✅ FIXED — flex + `resize:horizontal` |
| 5 | Couldn't add joystick rules | ✅ FIXED — unblocked by #1 |
| 6 | Simulated signals present | ✅ FIXED — removed; live-only monitor |

### #2 and #3 bottom out at the dispatcher `/ws` (NOT frontend-only)
The dispatcher streams `<uuid>,<action>` text — the sender-registration **UUID** plus an
**opaque** action string (INTERFACE.md **O3**). The console therefore:
- **#2**: shows/filter-by the UUID (no UUID→device-name map exists). *Real fix*: the
  **dispatcher** must add the device name (and ideally structured fields) to the `/ws`
  payload. Coordinate with the dispatcher/hw session; spec it against `INTERFACE.md`.
- **#3**: On the board, open the monitor and read the **status pill**:
  - `disconnected` → the `/websocket` → `:40080` nginx proxy or the dispatcher HTTP port
    is wrong. **INTERFACE.md O1**: dispatcher hardcodes `app.port(40080)` while
    `config.json` says `9080`. Reconcile the port so nginx `/websocket` reaches it.
  - `listening` but no rows when you press a control → the **device handler isn't
    reporting** to the dispatcher io channel (handler/dispatcher-side, not the console).
    (See also O4 `STACKED_IO_MSG=10` overflow, memory `conboard-dispatch-overflow`.)

## How to build / install / deploy / test
```sh
# build the board artifact from the local tree (no push needed)
./build-cross.sh zero3            # -> dist/zero3/conboard-zero3.tar.gz

# on the board (no apt, no compile; bundled libs; installs nginx site if nginx present)
scp dist/zero3/conboard-zero3.tar.gz <board>:~
tar xzf conboard-zero3.tar.gz && cd conboard && sudo ./install-on-device.sh
sudo ./uninstall-on-device.sh [--purge]     # remove (keeps rules DB unless --purge)

# verify
curl -s localhost:8080/healthz   # backend direct
curl -s localhost/healthz        # via nginx (same origin as UI)
open http://<board-ip>/          # the console
sudo journalctl -u backend -f    # NOTE: needs sudo (backend runs as root)

# deploy a profile from the console: edit a rule -> Save -> "Deploy to device"
#   -> POST /api/v1/deploy  -> writes boards/*.json + udev coldplug reload
```

## Endpoints
`/api/v1/{board,mode,rule,trigger,output_action}[/<id>]` · `POST /api/v1/deploy` ·
`GET /healthz` · `GET /ws` (backend relay seam, unused) · nginx serves the console at `/`
and proxies `/websocket` → dispatcher `:40080`. REST is credential-gated
(`X-User: <entity>`, `X-Pswd: <hash>`); hash = `1bf812ac18b80d4a5ea4d51e6bfb7f58`
(bumped when `midi_mode` was added to the trigger message; regen via `backend/generate.sh`).

## Don't-relearn facts
- **harpia is a black box.** Regenerate: `backend/generate.sh` (SQLite default;
  `HARPIA_DB_BACKEND=postgresql` for a future central host). Never hand-edit
  `backend/generated/`. `.harpia` authoring constraints (enums in root file, must import
  an `Include/` module, punctuation-plain ASCII comments) — see `backend/harpia/README.md`.
- **Realtime path** runs from `/conboard/boards/*.json`, decoupled from the DB by design;
  the launcher matches a device to a profile by `header.identifier.tags`.
- **Build**: emulated arm64 via QEMU; backend build `-j` capped to 4 (many-core hosts OOM
  the emulated protobuf/gRPC compiles → exit 126).
- **CRLF**: `.gitattributes` forces LF; a fresh Windows/WSL clone must renormalize
  (`git config core.autocrlf false && git rm --cached -r . && git reset --hard`).
- Frontend↔harpia JSON: camelCase fields, `ID<hash>` caller-assigned PK unique per table,
  enums as names, zero values omitted. Mapping in `frontend/console/src/api/{harpia,map,client}.ts`.

## Suggested next-session order
1. `git push origin main` (once network's up) so everyone's synced.
2. Rebuild `dist/` and reinstall on the board (get console + deploy on-device).
3. Drive #3 on the board (status pill) → decide if it's the O1 port or handler reporting.
4. Take #2/#3's dispatcher-side fix to the dispatcher/hw session (add device name +
   structured fields to `/ws`; reconcile the O1 port).
