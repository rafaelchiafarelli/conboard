# conboard frontend

The user-facing console for conboard — see attached devices, switch modes, and
(the core) **edit the trigger → output rules** without hand-editing `boards/*.json`.
See `../backend/README.md` for the API it talks to. `proposal.md` is the original
scope sketch/inventory written before any of this existed — kept as a historical
design record, superseded by what's actually built (this file + the top-level
[README.md](../README.md#whats-built)).

## Layout

- **`console/`** — the real app: **Vite + React + TypeScript**. Active development.
- **`dash/`** — the original Create-React-App + react-admin demo (wired to
  jsonplaceholder). Kept as reference only; **not** the app being built.

> The react-admin-vs-custom decision is deferred until the (harpia-generated)
> backend API shape is concrete. `console/` is plain Vite + React for now; that
> choice is compatible with either future.

## Running it

**Containerized (nothing installed on the host).** The Node toolchain lives
entirely in Docker, so the same environment is reproducible on any machine (this
matches the C++ build in `docker/`). You need only Docker; no host `node`/`npm`.

```bash
cd frontend
docker compose up console      # http://localhost:5173  — hot reload on save
```

On WSL2 + VS Code the port is forwarded to Windows automatically; open the URL in
your browser. Stop with `docker compose down`. Dependencies install into a named
volume on first start (`console_node_modules`) and are not written to the host tree.

Common tasks (all run in-container):

```bash
docker compose run --rm console npm install <pkg>   # add a dependency
docker compose run --rm console npm run typecheck   # type-check
docker compose run --rm console npm run build       # production bundle -> console/dist
```

**With a host Node instead**, if you have `node`/`npm` available (e.g. Node 22+):

```bash
cd frontend/console
npm install
npm run dev -- --host      # http://localhost:5173
npm run typecheck
npm run build               # -> console/dist
```

Talks to a real backend at `/api/v1` and `/ws` (proxy it or run one locally per
[`../backend/README.md`](../backend/README.md)) — there is no mock/offline data
provider anymore; the console seeds an empty backend from `src/fixtures/boards.ts`
on first load instead.

To extract just the static bundle (e.g. for the backend to serve later):

```bash
cd console
docker buildx build --target artifact -o type=local,dest=dist .
```

## Corporate proxy

Behind a TLS-intercepting proxy, Docker Hub / the npm registry may fail with
*"self-signed certificate in certificate chain"*. Trust the proxy's root CA in
the build environment (e.g. add it to the host Docker daemon's trusted certs, or
`COPY` it in and run `update-ca-certificates` + set `npm config set cafile`).
Do this on a machine with normal network access; do not disable TLS verification.

## What's here today

A feature console, not a skeleton — device rail (Add/Copy/Remove, backed by the
backend's `/devices` inventory), a full trigger→output rule editor (MIDI incl.
operation modes, plus evdev), a live monitor with per-device sender roster/LEDs and
a per-device live dock (seed a rule straight from an observed event), and
deploy/undeploy to the realtime path. See the top-level
[README.md § What's built](../README.md#whats-built) for the full list, and
[`../docs/NEXT-SESSION.md`](../docs/NEXT-SESSION.md) for what's still open.

Layout inside `console/src/`: `App.tsx` (shell + device rail), `RuleEditor.tsx`,
`Monitor.tsx` + `LiveDock.tsx` (live views), `*Dialog.tsx` (add/copy/remove),
`api/` (backend client + harpia wire mapping), `model/` (the typed rule model).
`DevInspector.tsx` is a dev-only (stripped from prod) Alt+click-to-source tool.
