#!/usr/bin/env bash
#
# Cross-build conboard for a specific board, several boards, or all of them.
# Boards are declared in docker/boards.conf -- add a line there to add a target.
#
#   ./build-cross.sh list           # show available boards (and their USB-OTG status)
#   ./build-cross.sh                # build ALL boards
#   ./build-cross.sh zero3          # build one board
#   ./build-cross.sh zero3 rpi64    # build several
#
# Output, one folder per board:
#   dist/<board-id>/conboard-<board-id>.tar.gz   the installable artifact
#   dist/<board-id>/BOARD.txt                    which board this is for ("surname")
#   dist/<board-id>/HOW-TO-INSTALL.txt           the main install commands
#   dist/<board-id>/conboard/                    the unpacked tree + MANIFEST.txt
#
# Requires Docker with buildx (Docker 23+). QEMU emulators and the builder are
# set up automatically.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

CONF="docker/boards.conf"
DEST="${DEST:-dist}"
BUILDER="conboard-xbuild"

trim() { local s="$1"; s="${s#"${s%%[![:space:]]*}"}"; s="${s%"${s##*[![:space:]]}"}"; printf '%s' "$s"; }

# --- load the board registry into parallel arrays ---------------------------
ids=(); platforms=(); arches=(); descs=(); otgs=()
while IFS='|' read -r c_id c_plat c_arch c_desc c_otg; do
    c_id="$(trim "${c_id:-}")"
    [ -z "$c_id" ] && continue
    [ "${c_id:0:1}" = "#" ] && continue
    ids+=("$c_id")
    platforms+=("$(trim "${c_plat:-}")")
    arches+=("$(trim "${c_arch:-}")")
    descs+=("$(trim "${c_desc:-}")")
    otgs+=("$(trim "${c_otg:-}")")
done < "$CONF"

idx_of() {  # echo the array index of board-id $1, or nothing if absent
    local want="$1" i
    for i in "${!ids[@]}"; do [ "${ids[$i]}" = "$want" ] && { echo "$i"; return; }; done
}

list_boards() {
    printf '%-10s %-16s %-7s %-44s %s\n' "BOARD-ID" "PLATFORM" "ARCH" "GOES IN" "USB-OTG"
    local i
    for i in "${!ids[@]}"; do
        printf '%-10s %-16s %-7s %-44s %s\n' \
            "${ids[$i]}" "${platforms[$i]}" "${arches[$i]}" "${descs[$i]}" "${otgs[$i]}"
    done
}

# --- argument handling ------------------------------------------------------
if [ "${1:-}" = "list" ]; then
    list_boards
    exit 0
fi

selected=()
if [ "$#" -eq 0 ] || [ "${1:-}" = "all" ]; then
    selected=("${ids[@]}")
else
    for arg in "$@"; do
        if [ -z "$(idx_of "$arg")" ]; then
            echo "ERROR: unknown board '$arg'. Known boards:" >&2
            list_boards >&2
            exit 1
        fi
        selected+=("$arg")
    done
fi

# --- one-time setup ---------------------------------------------------------
echo ">> registering QEMU emulators (one-time, needs --privileged)"
docker run --privileged --rm tonistiigi/binfmt --install arm64,arm >/dev/null

echo ">> ensuring buildx builder '$BUILDER' exists"
if ! docker buildx inspect "$BUILDER" >/dev/null 2>&1; then
    docker buildx create --name "$BUILDER" --driver docker-container --bootstrap >/dev/null
fi

echo ">> initialising git submodules (Crow)"
git submodule update --init --recursive

# --- build each selected board ----------------------------------------------
for id in "${selected[@]}"; do
    i="$(idx_of "$id")"
    plat="${platforms[$i]}"; desc="${descs[$i]}"; otg="${otgs[$i]}"
    echo
    echo ">> building '$id' ($desc) [$plat] -> $DEST/$id/"
    docker buildx --builder "$BUILDER" build \
        --platform "$plat" \
        --target artifact \
        --build-arg BOARD_ID="$id" \
        --build-arg BOARD_DESC="$desc" \
        --build-arg BOARD_OTG="$otg" \
        --output "type=local,dest=$DEST/$id" \
        -f docker/Dockerfile \
        .
done

echo
echo ">> done. artifacts:"
for id in "${selected[@]}"; do
    echo "   $DEST/$id/conboard-$id.tar.gz"
done
echo
echo "Each folder has BOARD.txt (which board) and HOW-TO-INSTALL.txt (commands)."
