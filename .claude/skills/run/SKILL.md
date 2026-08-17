---
name: run
description: Launch and drive the conboard console (frontend/console, Vite+React) in this WSL2 environment, including headless-Chromium UI verification via Playwright. Use whenever asked to run, start, screenshot, or browser-verify the console.
---

# Running conboard's console (WSL2 environment)

This machine is WSL2 (Ubuntu 24.04) with Windows Node.js also reachable on
`PATH` (`/mnt/c/Program Files/nodejs/`). That combination causes two gotchas
below that don't show up on a normal Linux box — read them before debugging
a failure that looks unrelated.

## One-time machine setup (already done on this machine, 2026-08-15)

Skip this section if `node -v` under `nvm use 22` already reports `v22.x`
and `~/.cache/ms-playwright/chromium-*` exists. Otherwise:

1. **Native Linux Node 20+.** Ubuntu 24.04's apt `nodejs` is v18, too old for
   current Playwright (`Playwright requires Node.js 20 or higher`). Install
   via nvm instead of apt (no `sudo` needed, and avoids the apt package
   entirely):
   ```bash
   curl -fsSL https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.1/install.sh | bash
   export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh"; nvm install 22
   ```
2. **Playwright's Chromium.** Two parts, only one needs root:
   ```bash
   # browser binary itself -> ~/.cache/ms-playwright, no sudo
   npx --yes playwright@latest install chromium

   # OS shared libs (libnss3, libatk, ...) -> needs sudo, and needs a REAL
   # terminal: it prompts for a password itself and there's no way to feed
   # that through a non-interactive tool. Run it yourself, interactively:
   npx --yes playwright@latest install-deps chromium
   ```

## Every session: activate the right Node

Shell state doesn't persist between tool calls, so prefix every command
that needs Node 22 with this (or put it at the top of a script):

```bash
export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh"; nvm use 22 >/dev/null
```

## Gotcha: `node_modules` installed under the wrong Node breaks Vite

If `frontend/console/node_modules` was ever installed while Windows'
`node.exe` was the one resolving on `PATH` (e.g. someone ran a plain `npm
install`/`npm run build` before nvm was set up), `vite` fails to start with:

```
Error: Cannot find module '@rollup/rollup-linux-x64-gnu'
```

This is npm's optional-dependency platform bug (installs the `win32`
native binary instead of `linux-x64-gnu` because that's what the installing
Node process reported as its platform). Fix: reinstall under the nvm Node,
not just re-run `npm install`:

```bash
export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh"; nvm use 22 >/dev/null
cd frontend/console && rm -rf node_modules && npm install
```

## Launch the dev server

```bash
export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh"; nvm use 22 >/dev/null
cd frontend/console
lsof -ti:5173 -sTCP:LISTEN | xargs -r kill 2>/dev/null   # free the port first
nohup npm run dev -- --port 5173 > /tmp/vite-dev.log 2>&1 &
disown
timeout 30 bash -c 'until curl -sf http://localhost:5173 >/dev/null; do sleep 1; done'
```

No backend is required for UI-only checks — the console falls back to
manual/offline mode (`AttachedDevice` fetch fails gracefully, "Not on the
list" still works) when `/api/v1` isn't reachable.

## Drive it with Playwright (no `chromium-cli` on this machine)

Write a small `.mjs` script rather than reaching for `chromium-cli` — it
isn't installed here. `playwright` itself doesn't need a project
dependency; install it ad hoc in the scratchpad and run from there (it
picks up the already-downloaded browser from `~/.cache/ms-playwright`
automatically, no re-download):

```bash
export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh"; nvm use 22 >/dev/null
mkdir -p /path/to/scratchpad/pw-check && cd /path/to/scratchpad/pw-check
npm init -y >/dev/null 2>&1
npm install playwright@latest
```

```js
// check.mjs
import { chromium } from 'playwright';
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
const errors = [];
page.on('console', (m) => { if (m.type() === 'error') errors.push(m.text()); });
page.on('pageerror', (e) => errors.push(String(e)));

await page.goto('http://localhost:5173', { waitUntil: 'networkidle' });
// ...click/select/fill through Playwright's API, not raw DOM writes — React
// controlled inputs need real input events, not `el.value = ...`.
await page.screenshot({ path: 'out.png' });
console.log('errors:', errors);
await browser.close();
```

```bash
node check.mjs
```

Then `Read` the resulting `.png` to actually look at it — a passing script
with no errors is not the same as a correct-looking page.

## Stop the dev server when done

```bash
lsof -ti:5173 -sTCP:LISTEN | xargs -r kill 2>/dev/null
```

## Backend (C++) — unaffected by any of the above

`build/tests/` already has a working CMake tree; `cd build/tests && make &&
./conboard_tests` runs the full doctest suite (no Node/WSL gotchas — this
is a normal native Linux C++ build).
