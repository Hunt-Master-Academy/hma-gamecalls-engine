#!/usr/bin/env bash
# Huntmaster fast developer test loop
# Runs a focused subset of tests to validate similarity readiness before commit.

set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build/debug}"
CMAKE_PRESET="${CMAKE_PRESET:-docker-debug}"
TEST_TIMEOUT="${TEST_TIMEOUT:-60}"
GTEST_FILTER="${GTEST_FILTER:-*MasterCallsComparison*:*RealtimeSimilarity*}" # align with fast readiness focus
RUNNER="${BUILD_DIR}/RunEngineTests"

# Ensure build directory is configured.
if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  cmake --preset "${CMAKE_PRESET}" >/dev/null
fi

# Build the focused test binary if it does not exist yet.
if [[ ! -x "${RUNNER}" ]]; then
  ninja -C "${BUILD_DIR}" RunEngineTests >/dev/null
fi

# Execute the focused test suite with timeout protection.
if ! timeout "${TEST_TIMEOUT}" "${RUNNER}" --gtest_filter="${GTEST_FILTER}" --gtest_brief=yes; then
  exit_code=$?
  if [[ ${exit_code} -eq 124 ]]; then
    echo "Fast test suite timed out after ${TEST_TIMEOUT}s" >&2
  fi
  exit ${exit_code}
fi
