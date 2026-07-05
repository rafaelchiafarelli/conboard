# conboard rules database (PostgreSQL)

The persisted **rules/actions library** — the source of truth for *authoring* and
*portability*, **not** the realtime execution path (hardware runs from `boards/*.json`).
See the project memory `conboard-rules-db-architecture`.

## Layout
- `schema/` — **harpia-generated DDL** (tables/columns/FKs derived from the rule
  message definitions). Generated, not hand-edited. Empty until harpia C++/DB codegen
  is wired in.
- `migrations/` — ordered bootstrap/migration SQL applied on first init by the
  docker-compose Postgres (`/docker-entrypoint-initdb.d`). `0001_init.sql` is a
  placeholder; harpia DDL will be folded in here (or sourced from `schema/`).
- `connection.env.example` — connection template (copy → `.env`, gitignored).

## Model (intended, from harpia messages)
Entities are whatever the harpia rule definitions compile to — expect roughly:
`device` / `profile` / `mode` / `rule` (trigger → output[]) / `action`. A rule row
references its device+mode; **copy A→B** duplicates the relevant rows re-keyed to
device B's identity. Final shape comes from the `.harpia` definitions, so this is
descriptive, not authoritative.
