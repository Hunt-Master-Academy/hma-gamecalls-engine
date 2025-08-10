#!/usr/bin/env bash
# Fast developer loop: build debug (incremental) and run similarity readiness related tests.
set -euo pipefail
ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build/debug"
PRESET="docker-debug"

# Select only the critical tests (master calls + any future readiness tests)
FILTER=${FILTER:-"MasterCallsComparisonTest.*:RealtimeSimilarity*"}
TIMEOUT=${TIMEOUT:-40}

echo "[dev_fast_test] Configuring (if needed) preset=$PRESET -> $BUILD_DIR" >&2
if [[ ! -d "$BUILD_DIR" || ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  cmake --preset "$PRESET" >/dev/null
fi

echo "[dev_fast_test] Building changed targets (ccache accelerated)" >&2
ninja -C "$BUILD_DIR" RunEngineTests >/dev/null

echo "[dev_fast_test] Running focused tests (timeout ${TIMEOUT}s, filter: $FILTER)" >&2
set +e
OUTPUT=$(timeout "$TIMEOUT" "$BUILD_DIR/bin/RunEngineTests" --gtest_filter="$FILTER" --gtest_brief=yes 2>&1)
STATUS=$?
set -e

echo "$OUTPUT"

# Post-process to emit framesObserved diagnostics when reliability failures occur
if [[ $STATUS -ne 0 ]]; then
  echo "[dev_fast_test] Test failures detected; extracting realtime readiness diagnostics..." >&2
  # Grep our injected debug lines if present
  echo "$OUTPUT" | grep -E "\[RealtimeState\]" || echo "[dev_fast_test] No realtime state debug lines captured." >&2
fi

exit $STATUS
