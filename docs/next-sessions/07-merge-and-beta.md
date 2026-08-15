# Session 7 — big merge, full regression pass, aggregate real-tester feedback

**Precondition: Sessions 1–6 above (whichever have landed) are each merged to
`main` on their own branch already — this session doesn't write feature code,
it consolidates, re-verifies as a whole, and turns outside feedback into a
punch list.** Needs the board for the regression pass.

## Part A — merge + full regression

1. Confirm every session branch that's ready is actually merged into `main`
   (`git log --oneline --all`, `git branch -a` — same commands used at the
   start of the 2026-08-15 assessment session to get oriented). Resolve any
   conflicts now, not by cherry-picking around them later.
2. Full clean-slate hardware regression, not just "the new stuff":
   - `./run-tests.sh` — full unit suite green.
   - `./build-cross.sh zero3` — clean cross-build.
   - On `192.168.7.4`: `sudo ./uninstall-on-device.sh --purge` → fresh
     `sudo ./install-on-device.sh` (clean-slate recommended after any
     schema/DB-shape change — check if Session 4/"6"'s synthetic-rules work
     touched the DB shape).
   - **Re-run the exact core-loop check from 2026-08-15**: real key press /
     mouse click actually producing output on the connected host. This is
     the one thing that must never silently regress again — it's the entire
     point of the product. The inotify-watch technique
     (`/dev/input/eventN` + `/dev/hidg0`, described in `NOTES.md` under
     "RESOLVED — mouse/keyboard rule output not firing") is the fastest way
     to confirm it live instead of trusting a visual "looked fine."
   - Re-run whichever of Sessions 3/4/5's hardware checks apply (joystick,
     queue overflow, DJ-MIDI redeploy) if any hardware-adjacent code changed
     since they were last confirmed.
   - Full install/uninstall/`--purge`/reinstall cycle once more end-to-end
     (this exact cycle has caught real bugs twice before — devname
     corruption, incomplete gadget teardown, wrong `.htpasswd` ownership —
     don't skip it because "nothing touched that area this time").
3. Tag the result. `v1.0.0-pre-beta` already exists (tagged/pushed
   2026-08-15). If this merge is substantive (new features from Sessions 4/
   "6" and/or 5-6/"7-8"), the next tag is a judgment call:
   - Bug-fixes-and-polish only → `v1.0.0-pre-beta.2` or similar.
   - New features (synthetic 1:1 rules, HMI screens) on top of a still-rough
     product → `v1.1.0-pre-beta` or promote to `v1.0.0-beta` if you're
     confident enough in stability to drop "pre-". Your call at the time —
     this file isn't prescribing which, just flagging that it needs an
     explicit decision, not a default.

## Part B — aggregate feedback from real testers

You're sharing `v1.0.0-pre-beta` with people who aren't you, on hardware you
don't control, in setups you haven't tested. Treat their reports with the
same rigor this project's own hardware-verification discipline demands for
your own claims — "a tester said X is broken" is a lead, not a confirmed bug,
until reproduced or clearly understood.

1. **Collect feedback somewhere durable and structured**, not scattered
   across chat/DMs you'll lose track of. Simplest workable approach: one file
   per tester under `docs/tester-feedback/<name-or-handle>.md`, each covering:
   - Their board (Zero3 vs. original Zero/H3) and what USB/HID devices they
     tried.
   - What worked, what didn't, in their own words first.
   - Anything you additionally probed once they reported an issue.
   If the repo has GitHub Issues enabled and testers are comfortable there,
   that works too — but keep a local rollup either way so this session can
   actually triage from one place instead of re-collecting each time.
2. **Triage each report into one of three buckets**:
   - **Reproducible bug** — you can make it happen, or the report is precise
     enough to trust without reproducing (rare — prefer reproducing). These
     become new numbered session files, same pattern as this roadmap:
     small, scoped, with the exact facts (paths/commands) a fresh session
     would need, so fixing them doesn't require re-deriving context.
   - **Environment/setup issue** — their board, their device, their network,
     not a code bug. Goes into `docs/GETTING-STARTED.md` as a troubleshooting
     note if it's likely to recur for other testers, not into a code-fix
     session.
   - **Feature request / scope creep** — valid, but not this cycle. Goes into
     `NOTES.md`'s direction notes for later prioritization, explicitly not
     bundled into whatever's "in progress" right now (same discipline as
     keeping HMI out of the core-loop fix).
3. **Don't let unverified tester reports become the next roadmap wholesale.**
   Read them, triage them, decide deliberately what becomes a real session —
   the same "don't scope-creep sideways" lesson from before v1 applies just
   as much to inbound feedback as to your own ideas.

## Done criteria

- `main` green on full regression, board reinstalled clean and re-verified.
- A tag reflecting the actual state, decided deliberately (see Part A.3).
- Every tester report triaged into one of the three buckets above, with
  reproducible bugs turned into new, scoped session files — not left as a
  loose pile of chat messages.
