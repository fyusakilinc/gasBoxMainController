#!/usr/bin/env bash
set -euo pipefail

# Resolve repo root from this script’s directory (so we don’t rely on UNC ROOT)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# If build_and_run.sh is in test/onpc/, ROOT is two dirs up; adjust if yours is elsewhere.
ROOT="//Fileserverbhf/Public/Furkan/workspace/gasBoxMainController"

OUT_DIR="$ROOT/test/onpc/build"
OUT_BIN="$OUT_DIR/onpc_tests.exe"

# Unity location (you said it's directly under test/onpc)
UNITY_C="$ROOT/test/onpc/src/unity.c"
UNITY_INC_DIR="$ROOT/test/onpc/inc"

# Test sources
TEST_MAIN="$ROOT/test/onpc/src/main.c"
TEST_FUNCS="$ROOT/test/onpc/src/functions.c"
TEST_INC="$ROOT/test/onpc/inc"
CORE_INC="$ROOT/Core/Inc"   # include if your test headers reference project headers
CMD_INC="$ROOT/Core/Src/cmdlist.c"

# Sanity checks to fail early with a clear message
for f in "$UNITY_C" "$TEST_MAIN" "$TEST_FUNCS"; do
  [[ -f "$f" ]] || { echo "Missing file: $f" >&2; exit 1; }
done
for d in "$UNITY_INC_DIR" "$TEST_INC"; do
  [[ -d "$d" ]] || { echo "Missing include dir: $d" >&2; exit 1; }
done

mkdir -p "$OUT_DIR"

# Build (host build, no HAL)
gcc -Wall -Wextra -Werror -Wshadow -O0 -g -std=c11 \
  "$UNITY_C" "$TEST_MAIN" "$TEST_FUNCS" "$CMD_INC" \
  -I"$UNITY_INC_DIR" -I"$TEST_INC" -I"$CORE_INC"\
  -o "$OUT_BIN"

# Run tests
"$OUT_BIN"
