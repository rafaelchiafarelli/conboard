# conboard frontend — proposal & current-state inventory

This document is the starting point for specifying the conboard **user interface**.
It records **what exists today** (so we don't re-discover it) and frames **what the UI
is for** in the larger system. It is deliberately an inventory + scope sketch, not a
final spec — the detailed feature spec gets layered on top once we agree on direction.

> Scope split (agreed): this repo's next work divides into
> **(A) backend + frontend / user interface** — handled here, and
> **(B) hw-interface + dispatcher** — handled in a separate session.
> The **backend and the dispatcher are to be (re)implemented via the `../harpia`
> meta-language**, not hand-written. This document covers side (A), UI-facing.

---

## 1. What exists today

### 1.1 Frontend (`frontend/dash/`)
A Create-React-App scaffold using **react-admin 3.16** + `ra-data-json-server`.

- **It is a demo skeleton, not wired to conboard.** The data provider points at
  `https://jsonplaceholder.typicode.com` — the lists/forms below operate on that
  public fake API, not on any conboard backend.
- Components present:
  - `App.js` — react-admin `<Admin>` shell with two resources (`users`, `posts`).
  - `components/dashboard/Dashboard.jsx` — placeholder (`<h1>THis is the dashboard</h1>`).
  - `components/table/` — `User.jsx` (user list), `Post.jsx` (post list),
    `EditPost.jsx`, `CreatePost.jsx`, `FilterPost.jsx` — all standard react-admin
    CRUD against the placeholder API.
- React 17, react-scripts 4, yarn. Default CRA test still asserts "learn react".

**Honest status: this is boilerplate.** None of it reflects conboard's real domain
(devices, modes, rules, live events). It demonstrates the react-admin pattern and
nothing conboard-specific.

### 1.2 Backend (`backend/`)
A **Flask + flask-socketio skeleton**, also largely stubbed.

- `main.py` — Flask app with `flask_socketio`. One route `/` that tries to
  `json.load("/conboard/boards/Dj4Mix.json")` (note: passes a path string, not a
  file handle — not functional as written). Static/template wiring points at a
  `static/dash/` build dir.
- `landpage/landpage.py` — page-handler stubs per device family (`midiPage`,
  `keyboardPage`, `joystickPage`, `mousePage`, `dmxPage`, `nonamePage`, `landPage`)
  returning placeholder dicts; `landPage` renders an `index.html` template. Depends
  on a `sidebar` module not present in the tree.
- `assets/` — `requirements.txt`, `interface.conf`, `frontend.service` (systemd unit).
- `backend/README.md` — **a written-but-unimplemented API design** for:
  - **User API** — profile (name/pic/email/phone/occupation/…), connection stats,
    recent user activity.
  - **Security API** — local-access login via a **power password shown on the
    device screen**, rotating on each attempt, lockout escalation (3 fails → red
    edge + harder password; 10 fails → physical-keyboard-only unlock), email notify.
  - **Shared API** — share/unshare files, per-file share scope, share counts.
  - **Vault API** — add/retrieve/clear files in a vault.

**Honest status: design notes + scaffold, not a working backend.** The README is the
most concrete artifact here and is the best seed for the real API surface.

### 1.3 The system the UI must front (already built, in `LowLevel/`)
This is the real, partly hardware-verified product the UI exists to control. The UI's
job is to expose these concepts:

- **Devices as USB gadgets**: the board presents itself to a host PC as keyboard,
  mouse, joystick, and bootable mass-storage; attached input devices are detected via
  kernel ABIs (evdev / USB class), not VID/PID.
- **Per-device handlers**, each its own process on a shared engine:
  - `conMIDI` — MIDI in/out. **Hardware-verified.**
  - `conJoyS` / `conKeyB` / `conMouse` — evdev joystick/keyboard/mouse, built on the
    shared `EvdevDevice`/`DeviceEngine`. **Built, not yet hardware-tested.**
- **Operation modes** per device — unlimited named modes, switchable at runtime; the
  current mode is remembered.
- **A rule language** (in `boards/*.json`, e.g. `boards/Xbox360.json`): a trigger
  (MIDI `b0/b1/b2`+mode, or symbolic evdev `BTN_*`/`KEY_*`/`ABS_*`/`REL_*`+mode with
  press/release/hold/hold_once/higher/lower/spot) maps to an output list of
  keyboard/mouse/MIDI actions.
- **Device identity & separation** — two identical controllers separate by trustworthy
  serial else by physical USB port; device moves between ports are handled.
- **A live event stream**: handlers report input events over ZMQ to a **dispatcher**,
  which currently pushes to a **websocket** for any client. (There is a known tiny
  10-slot reporting-queue overflow under heavy traffic — `STACKED_IO_MSG`.)

So the surface area the UI ultimately needs to cover is: see attached devices and
their type/identity, see/switch operation modes, **view and edit the rule mappings**
per device/mode, watch the live event stream, and the User/Security/Shared/Vault
concerns above.

---

## 2. What the UI is *for* (scope sketch)

Framed from the system above, the UI's natural feature areas are:

1. **Live monitor** — show devices currently attached (type, identity, port, mode)
   and a live feed of incoming events from the websocket. *(Backend already emits
   these; today nothing consumes them — the python frontend isn't installed.)*
2. **Mode control** — view a device's operation modes and switch the active one.
3. **Rule editor** — the core differentiator: view and edit the trigger→output
   mappings (`boards/*.json` rules) per device and per mode, without hand-editing
   JSON. This is what turns conboard from "runs a config" into "a control surface you
   configure from a screen."
4. **Device/profile management** — list/add/edit per-device profiles (the
   `boards/<name>.json` files), incl. identity binding.
5. **Access/security** — the local power-password login flow described in the backend
   README.
6. **User / Shared / Vault** — the secondary APIs from the backend README, if/when
   they stay in scope.

The current frontend covers **none** of these conboard-specific areas yet; it is a
react-admin demo. The next step is for the user to specify which of the above are
in-scope and in what priority, after which this document becomes the real spec.

---

## 3. Known constraints & decisions carried in

- **Backend + dispatcher will be generated from `../harpia`** (the user's meta-language).
  conboard's dispatcher↔device messages — today defined ad-hoc and parsed by a
  hand-written RapidJSON parser — are the natural first consumer of harpia-generated
  structs + JSON + ZMQ code. The UI's contract with the backend should therefore be
  expressible as harpia message definitions, not just informal JSON.
- **Portability-first**: conboard targets many enthusiasts on heterogeneous Linux SBCs
  (Orange Pi Zero 3 today, Raspberry Pi / others later). The UI should assume nothing
  board-specific.
- **"User shouldn't have dangerous power"**: launching/detecting services is currently
  too user-configurable in a way that's risky; the UI should constrain configuration to
  safe, validated rule/profile editing rather than arbitrary service control.
- **Security is unstarted**: no firewall / access control yet beyond the (unimplemented)
  power-password design. The UI is local-access oriented.
- **Backend reorged python→C++ (2026-06-30, structural).** The deprecated Flask
  backend was retired; `backend/` now holds a C++ skeleton (Crow HTTP + libpqxx +
  harpia-generated structs) with three concerns kept separate: persistence/DB,
  management API, dispatcher seam. No real implementation yet.
- **Rules/actions persist in PostgreSQL as a portable LIBRARY** (authoring + copy
  device→device), decoupled from the realtime execution path; the DB schema is
  **harpia-generated** from the rule message definitions. So the UI's CRUD is a view
  layer over harpia's data model. See memory `conboard-rules-db-architecture`.
- **Frontend library choice is deferred** until the harpia C++ backend's API shape is
  concrete. The real CRUD layer (rules/actions library) raises react-admin's value,
  but the pick is tied to the backend's data-provider contract; current frontend left
  untouched for now. (CRA itself is deprecated — Vite is the likely tooling either way.)

---

## 4. Open questions for the spec (to resolve next)

- Which feature areas in §2 are in the first cut, and in what order?
- Does the harpia-generated backend **replace** Flask, or does Flask stay as a thin
  shell calling harpia-generated message/transport code?
- Keep react-admin, or move to a lighter custom React app? (react-admin fits CRUD
  over profiles/users; the live monitor + rule editor are not natural react-admin
  resources.)
- Is the rule editor a structured form, a node/graph editor, or a guided table?
- What exactly is the websocket message contract (this should become harpia messages)?
