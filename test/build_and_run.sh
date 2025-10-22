#!/usr/bin/env bash
set -euo pipefail

if [[ -t 1 ]]; then
  COLOR_BLUE=$'\033[1;34m'
  COLOR_GREEN=$'\033[1;32m'
  COLOR_RED=$'\033[1;31m'
  COLOR_RESET=$'\033[0m'
else
  COLOR_BLUE=''
  COLOR_GREEN=''
  COLOR_RED=''
  COLOR_RESET=''
fi

section() {
  printf '\n%s==>%s %s\n' "$COLOR_BLUE" "$COLOR_RESET" "$1"
}

success() {
  printf '\n%s✔%s %s\n' "$COLOR_GREEN" "$COLOR_RESET" "$1"
}

failure() {
  printf '\n%s✖%s %s\n' "$COLOR_RED" "$COLOR_RESET" "$1" >&2
}

trap 'rc=$?; if [[ $rc -ne 0 ]]; then failure "Test run failed (exit ${rc})"; fi' EXIT

# Resolve repo root from this script’s directory (so we don’t rely on UNC ROOT)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# If build_and_run.sh is in test/, ROOT is one dir up.
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

OUT_DIR="$ROOT/test/onpc/build"
OUT_BIN="$OUT_DIR/onpc_tests.exe"

# Unity location (you said it's directly under test/onpc)
UNITY_C="$ROOT/test/onpc/src/unity.c"
UNITY_INC_DIR="$ROOT/test/onpc/inc"

# Test sources
TEST_MAIN="$ROOT/test/onpc/src/main.c"
TEST_FUNCS="$ROOT/test/onpc/src/functions.c"
TEST_RFG="$ROOT/test/onpc/src/rfg_test.c"
TEST_INC="$ROOT/test/onpc/inc"
CORE_INC="$ROOT/Core/Inc"   # include if your test headers reference project headers
CMD_INC="$ROOT/Core/Src/cmdlist.c"
PRIO_LIST_SRC="$ROOT/Core/Src/prioritylist.c"
PRIO_PUSHPOP_SRC="$ROOT/Core/Src/priority_pushpop.c"

# Sanity checks to fail early with a clear message
for f in "$UNITY_C" "$TEST_MAIN" "$TEST_FUNCS" "$TEST_RFG" \
         "$CMD_INC" "$PRIO_LIST_SRC" "$PRIO_PUSHPOP_SRC"; do
  [[ -f "$f" ]] || { echo "Missing file: $f" >&2; exit 1; }
done
for d in "$UNITY_INC_DIR" "$TEST_INC"; do
  [[ -d "$d" ]] || { echo "Missing include dir: $d" >&2; exit 1; }
done

section "Preparing build directory"
mkdir -p "$OUT_DIR"

# Build (host build, no HAL)
section "Building host Unity test binary"
gcc -Wall -Wextra -Werror -Wshadow -O0 -g -std=c11 \
  "$UNITY_C" "$TEST_MAIN" "$TEST_FUNCS" "$TEST_RFG" \
  "$CMD_INC" "$PRIO_LIST_SRC" "$PRIO_PUSHPOP_SRC" \
  -I"$UNITY_INC_DIR" -I"$TEST_INC" -I"$CORE_INC"\
  -o "$OUT_BIN"

# Run tests
section "Running host Unity tests"
"$OUT_BIN"

success "All host Unity tests passed"