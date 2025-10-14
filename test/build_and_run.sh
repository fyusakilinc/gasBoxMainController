#!/usr/bin/env bash
set -e
ROOT='//FILESERVERBHF/Public/Furkan/workspace/gasBoxMainController'
gcc "$ROOT/test/host/src/"test_*.c "$ROOT/test/host/src/unity.c" "$ROOT/test/host/src/host_mocks.c" "$ROOT/Core/Src/apc.c" -I"$ROOT/Core/Inc" -I"$ROOT/test/host/inc" -std=c11 -O2 -o "$ROOT/test/host/host_tests.exe"
"$ROOT/test/host/host_tests.exe"
