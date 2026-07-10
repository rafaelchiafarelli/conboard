# conboard rules-library domain (`.harpia`)

`conboard.harpia` is the **single source artifact** we hand to
[harpia](../../../harpia/USAGE.md) (a black box) to generate the backend's C++ layer:
message structs + JSON + a **SOCI-backed** CRUDL DAO + REST (Crow) + gRPC + the SQL
schema. It models the **authoring / portability library** (see project memory
`conboard-rules-db-architecture`) — **not** the realtime path (hardware still runs from
the on-device board JSON). It mirrors [`frontend/console/src/model/rules.ts`](../../frontend/console/src/model/rules.ts).

## Generate

harpia picks the DB dialect at generation time. For the conboard backend we target
**PostgreSQL**:

```sh
# from the harpia repo, Postgres dialect (see note below on env forwarding):
cd ../harpia
docker run --rm -u "$(id -u):$(id -g)" \
  -v "$PWD":/harpia -w /harpia \
  -v /abs/path/to/conboard/backend/harpia:/in:ro \
  -v /abs/path/to/conboard/backend/generated:/out \
  -e HOME=/tmp -e HARPIA_DB_BACKEND=postgresql \
  -e HARPIA_INPUT_FILE=/in/conboard.harpia \
  -e HARPIA_INCLUDE_FOLDER=/in/Include \
  -e HARPIA_OUTPUT_DIR=/out \
  harpia-build python3 main.py
```

`run_harpia.sh <in> <out>` is the simpler entry point but does **not** forward
`HARPIA_DB_BACKEND` (defaults to sqlite), so use it only for quick validation. The
C++ API is identical across dialects — only the emitted SQL changes.

> **Never hand-edit `../generated/`** — it is regenerated wholesale. Change
> `conboard.harpia` and regenerate.

## Domain shape

harpia has no union/oneof, so the TypeScript unions are **flattened** into one message
each with a `kind` discriminant enum plus optional per-variant fields:

| message (`_table`) | mirrors `rules.ts` | notes |
|---|---|---|
| `trigger` | `Trigger` (`Midi`/`Evdev`) | `kind` = `tk_midi`/`tk_evdev`; midi uses `b0..b2`, evdev uses `code`/`edge`/`value`/… |
| `output_action` | `OutputAction` (`Midi`/`Keyboard`/`Mouse`) | `kind` = `ak_*`; mouse fields kept as `string` for board-file fidelity |
| `rule` | `Rule` | 1-to-1 FK `input` → `trigger`; 1-to-many `outputs` → `output_action`; `change_mode_*` |
| `mode` | `Mode` | `mode_id`, `active`; 1-to-many `mode_header` + `rules` |
| `board` | `Board` | device profile; `generics`/`tags` maps; 1-to-many `header_actions` + `modes` |

Booleans are `int` (0/1) — harpia's scalar set is `int`/`string`. Primary keys
(`ID_<hash>`) are **caller-assigned** (set before `create()`), which makes
**copy A→B** a straightforward row-duplication re-keyed to board B.

Generated identifiers are **md5-hash-qualified** from this input
(`board_<hash>_crudl.h`, accessor `id_<hash>()`); the hash changes if the domain
changes, so the backend include paths track it.

## harpia authoring constraints (learned the hard way)

The harpia lexer/pipeline is fragile; a valid-looking `.harpia` can still fail. Keep to:

1. **All enums in the root file.** Enums defined in an `Include/` module and referenced
   as a field type fail with `REGEX_NOT_FOUND`.
2. **An `Include/` module must exist and be imported.** An empty include set crashes the
   pipeline (`main.py` `NameError: analizer`). `Include/types.harpia` is a minimal,
   non-persisted placeholder that exists only to satisfy this.
3. **Comments must be punctuation-plain, ASCII only.** These characters in comment text
   break the lexer/pre-lexer:
   - the block-comment open/close two-char sequences (e.g. writing a glob like a slash
     immediately followed by a star) — miscounted as a real block comment;
   - `:` (colon) and `'` (apostrophe) — tokenized and rejected;
   - non-ASCII (em-dash, smart quotes) — `NON_ASCII_CHAR`.
   Prefer plain words, `.` `,` and `-` separators. Full rationale lives here in the
   README, not in the `.harpia`.
