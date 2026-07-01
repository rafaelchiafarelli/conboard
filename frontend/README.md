# conboard frontend

The user-facing console for conboard — see attached devices, switch modes, and
(the core) **edit the trigger → output rules** without hand-editing `boards/*.json`.
See `proposal.md` for the scope sketch and `../backend/README.md` for the API it
will eventually talk to.

## Layout

- **`console/`** — the real app: **Vite + React + TypeScript**. Active development.
- **`dash/`** — the original Create-React-App + react-admin demo (wired to
  jsonplaceholder). Kept as reference only; **not** the app being built.

> The react-admin-vs-custom decision is deferred until the (harpia-generated)
> backend API shape is concrete. `console/` is plain Vite + React for now; that
> choice is compatible with either future.

## Running it — containerized, nothing installed on the host

The Node toolchain lives entirely in Docker, so the same environment is
reproducible on any machine (this matches the C++ build in `docker/`). You need
only Docker; no host `node`/`npm`.

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

## What's here today (step 1)

A containerized skeleton that boots and renders **real board data** (device list,
modes, rule counts) through the typed rule model in `console/src/model/`. It
proves the toolchain + data flow. The full trigger/output rule editor is built in
the following steps; the design target is the mockup shared separately.
