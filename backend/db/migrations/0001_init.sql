-- conboard rules DB — bootstrap migration (PLACEHOLDER, 2026-06-30).
--
-- The real schema (tables/columns/FKs) is GENERATED from ../harpia rule message
-- definitions and will replace / be sourced into this file. Do not hand-author the
-- rule schema here long-term — keep it harpia-generated so the DB, the C++ structs,
-- and the JSON stay from one source. See backend/db/README.md.
--
-- This file exists so docker-compose's first-init has something to apply and so the
-- migrations directory is tracked.

-- Sanity marker so we can confirm init ran against a fresh volume.
CREATE TABLE IF NOT EXISTS schema_bootstrap (
    id          SMALLINT PRIMARY KEY DEFAULT 1,
    applied_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    note        TEXT NOT NULL DEFAULT 'placeholder; harpia-generated schema pending'
);
INSERT INTO schema_bootstrap (id) VALUES (1) ON CONFLICT (id) DO NOTHING;
