#!/usr/bin/env bash
# Coverage wrapper retained for compatibility. Performs an instrumented build (debug) then delegates.
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "$0")/.." && pwd)"
echo "[coverage wrapper] Configuring coverage build" >&2
cmake -B "$ROOT/build/coverage" -S "$ROOT" --preset docker-coverage 2>/dev/null || cmake -B "$ROOT/build/coverage" -S "$ROOT" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage -O0" -DCMAKE_EXE_LINKER_FLAGS="--coverage"
cmake --build "$ROOT/build/coverage" --parallel
BUILD_DIR=build/coverage COVERAGE=true ENFORCE_COVERAGE=true "$ROOT/scripts/master_test.sh" --phases=unit,diagnostics-off,coverage "$@"
