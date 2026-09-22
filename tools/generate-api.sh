#!/usr/bin/env bash
set -euo pipefail

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
WORK="$ROOT/.tmp/docs-setup"
OUTPUT="$ROOT/docs/api/generated"
CLANG_DOC=${CLANG_DOC:-clang-doc}
CXX=${CXX:-clang++}

command -v cmake >/dev/null || { echo 'Install CMake first.' >&2; exit 1; }
command -v ninja >/dev/null || { echo 'Install Ninja first.' >&2; exit 1; }
command -v "$CLANG_DOC" >/dev/null || { echo 'Install clang-doc first.' >&2; exit 1; }
command -v "$CXX" >/dev/null || { echo 'Install clang++ first.' >&2; exit 1; }

BUILD="$WORK/cmake"
RAW="$WORK/raw"
mkdir -p "$WORK" "$RAW" "$OUTPUT"

cmake -S "$ROOT" -B "$BUILD" -G Ninja \
  -DCMAKE_CXX_COMPILER="$CXX" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DSHADCN_BUILD_TESTS=OFF \
  -DSHADCN_BUILD_EXAMPLES=OFF

find "$OUTPUT" -mindepth 1 -delete
find "$RAW" -mindepth 1 -delete
"$CLANG_DOC" --format=md --public --output="$RAW" -p "$BUILD" \
  include/shadcn/core.hpp include/shadcn/widgets.hpp

test -f "$RAW/shadcn/index.md" || { echo 'clang-doc did not emit the shadcn namespace reference.' >&2; exit 1; }
python3 "$ROOT/tools/api_markdown.py" "$RAW/shadcn" "$OUTPUT"
test -f "$OUTPUT/index.md" || { echo 'Generated API reference is empty.' >&2; exit 1; }
