#!/usr/bin/env bash
# Regenerate backend/generated/ from backend/harpia/conboard.harpia via harpia
# (a black box, see ../harpia/USAGE.md). harpia CLEANS the output dir each run, so
# this script lives OUTSIDE backend/generated/ (which would otherwise be wiped).
#
# Why not ../harpia/run_harpia.sh? It does not forward HARPIA_DB_BACKEND, so it can
# only emit the sqlite dialect. conboard targets PostgreSQL, so we invoke the
# harpia-build image directly with the env the pipeline reads.
#
# Usage:
#   backend/generate.sh                 # postgresql dialect (default, prod target)
#   HARPIA_DB_BACKEND=sqlite backend/generate.sh   # sqlite (constrained boards / tests)
#   HARPIA_REPO=/path/to/harpia backend/generate.sh
#
# The generated tree is committed to the repo so conboard builds standalone without
# the harpia repo present (USAGE.md section 4). Do NOT hand-edit backend/generated/.
set -euo pipefail

BACKEND_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HARPIA_REPO="${HARPIA_REPO:-$(cd "$BACKEND_DIR/../../harpia" && pwd)}"
HARPIA_DB_BACKEND="${HARPIA_DB_BACKEND:-postgresql}"
IMAGE="${HARPIA_IMAGE:-harpia-build}"

IN="$BACKEND_DIR/harpia"
OUT="$BACKEND_DIR/generated"

[ -f "$IN/conboard.harpia" ] || { echo "error: $IN/conboard.harpia not found" >&2; exit 1; }
[ -d "$HARPIA_REPO" ] || { echo "error: harpia repo not found at $HARPIA_REPO (set HARPIA_REPO)" >&2; exit 1; }
if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
    echo "building $IMAGE (first run only)..." >&2
    docker build -t "$IMAGE" "$HARPIA_REPO"
fi

mkdir -p "$OUT"
echo "harpia repo : $HARPIA_REPO"
echo "dialect     : $HARPIA_DB_BACKEND"
echo "input       : $IN/conboard.harpia"
echo "output      : $OUT (cleaned by harpia)"

docker run --rm -i -u "$(id -u):$(id -g)" \
    -v "$HARPIA_REPO":/harpia -w /harpia \
    -v "$IN":/in:ro \
    -v "$OUT":/out \
    -e HOME=/tmp \
    -e HARPIA_DB_BACKEND="$HARPIA_DB_BACKEND" \
    -e HARPIA_INPUT_FILE=/in/conboard.harpia \
    -e HARPIA_INCLUDE_FOLDER=/in/Include \
    -e HARPIA_OUTPUT_DIR=/out \
    "$IMAGE" python3 main.py

echo "done. generated project -> $OUT"
echo "generated identifier hash (changes with the domain):"
ls "$OUT/generated/cpp/db"/*_crudl.h 2>/dev/null | sed -E 's:.*/[a-z_]+_([0-9a-f]{32})_crudl\.h:  \1:' | sort -u
