#!/usr/bin/env bash
# selfhost.sh — three-stage bootstrap to prove guscc can compile itself.
#
# Stage 1 (built by host gcc, today's `build/guscc`)
#   -> compiles each src/*.c to .s, assembles, links -> guscc-stage2
# Stage 2 (`guscc-stage2`)
#   -> compiles each src/*.c to .s, assembles, links -> guscc-stage3
# Then `cmp -s guscc-stage2 guscc-stage3` must return 0 (bit-identical fixed point).
#
# Usage: test/selfhost.sh [build-dir]   (default build-dir: build)

set -e

BUILD_DIR="${1:-build}"
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SRC_DIR="$PROJECT_DIR/src"

if [[ ! -x "$BUILD_DIR/guscc" ]]; then
    echo "selfhost: stage-1 binary $BUILD_DIR/guscc not found — run cmake build first" >&2
    exit 1
fi

# build_stage <compiler> <out-binary>
#   compiles every src/*.c with <compiler>, assembles, links into <out-binary>.
build_stage() {
    local cc="$1"
    local outbin="$2"
    local stagedir="$BUILD_DIR/$(basename "$outbin").d"
    rm -rf "$stagedir"
    mkdir -p "$stagedir"

    local ofiles=()
    for src in "$SRC_DIR"/*.c; do
        local base
        base="$(basename "$src" .c)"
        local sf="$stagedir/$base.s"
        local of="$stagedir/$base.o"
        "$cc" -S -o "$sf" "$src"
        as "$sf" -o "$of"
        ofiles+=("$of")
    done

    # Link through cc, same as guscc.c does, so crt paths and the dynamic
    # linker come from the host toolchain instead of a hardcoded distro layout.
    cc -no-pie -o "$outbin" "${ofiles[@]}"
}

echo "[selfhost] stage-2: building with $BUILD_DIR/guscc"
build_stage "$BUILD_DIR/guscc" "$BUILD_DIR/guscc-stage2"
echo "[selfhost] stage-2 built: $BUILD_DIR/guscc-stage2"

echo "[selfhost] stage-3: building with $BUILD_DIR/guscc-stage2"
build_stage "$BUILD_DIR/guscc-stage2" "$BUILD_DIR/guscc-stage3"
echo "[selfhost] stage-3 built: $BUILD_DIR/guscc-stage3"

echo "[selfhost] cmp stage-2 vs stage-3..."
if cmp -s "$BUILD_DIR/guscc-stage2" "$BUILD_DIR/guscc-stage3"; then
    echo "[selfhost] ✅ guscc-stage2 == guscc-stage3 — fixed point reached"
else
    echo "[selfhost] ❌ guscc-stage2 != guscc-stage3"
    ls -l "$BUILD_DIR/guscc-stage2" "$BUILD_DIR/guscc-stage3"
    exit 1
fi
