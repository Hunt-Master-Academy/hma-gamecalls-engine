#!/usr/bin/env bash
# Huntmaster pre-commit quality gate
set -euo pipefail
ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build/debug"
PRESET="docker-debug"
MAX_FIX=200

red() { printf "\e[31m%s\e[0m\n" "$*"; }
green() { printf "\e[32m%s\e[0m\n" "$*"; }
yellow() { printf "\e[33m%s\e[0m\n" "$*"; }

# 1. Format staged C/C++ sources
CHANGED=$(git diff --cached --name-only --diff-filter=AM | grep -E '\.(c|cc|cpp|h|hpp)$' || true)
if [[ -n "$CHANGED" ]]; then
  yellow "[pre-commit] Formatting changed sources" >&2
  echo "$CHANGED" | while read -r f; do
    if [[ -f "$f" ]]; then
      clang-format -i "$f"
      git add "$f"
    fi
  done
fi

# 2. Ensure build directory exists (configure once)
if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  cmake --preset "$PRESET" >/dev/null
fi

# 3. Compile (fast) target if any C++ files changed
if [[ -n "$CHANGED" ]]; then
  yellow "[pre-commit] Incremental build (RunEngineTests)" >&2
  ninja -C "$BUILD_DIR" RunEngineTests >/dev/null || { red "Build failed"; exit 1; }
fi

# 4. Run focused fast test suite unless disabled
if [[ "${SKIP_FAST_TEST:-0}" != "1" ]]; then
  TEST_TRIGGER=$(git diff --cached --name-only --diff-filter=AM | grep -E '(/src/|/test_data/|/tests/|UnifiedAudioEngine\.cpp)' || true)
  if [[ -n "$TEST_TRIGGER" ]]; then
    yellow "[pre-commit] Running fast similarity readiness tests" >&2
    if ! bash "$ROOT_DIR/scripts/dev_fast_test.sh" >/dev/null; then
      red "[pre-commit] Fast tests failed" >&2
      bash "$ROOT_DIR/scripts/dev_fast_test.sh" || true
      exit 1
    fi
  fi
else
  yellow "[pre-commit] SKIP_FAST_TEST=1 (developer override)" >&2
fi

# 5. Basic static analysis (clang-tidy on changed files) limited for speed
if command -v clang-tidy >/dev/null 2>&1 && [[ -n "$CHANGED" ]]; then
  yellow "[pre-commit] clang-tidy (changed files with cache)" >&2
  CACHE_DIR="$ROOT_DIR/.cache/clang_tidy"
  mkdir -p "$CACHE_DIR"
  FAIL=0
  echo "$CHANGED" | while read -r f; do
    [[ -f "$f" ]] || continue
    HASH=$(sha1sum "$f" | cut -d' ' -f1)
    CACHE_FILE="$CACHE_DIR/${HASH}.ok"
    if [[ -f "$CACHE_FILE" ]]; then
      continue
    fi
    if clang-tidy "$f" -- -I"$ROOT_DIR/include" -std=c++20 >/dev/null 2>&1; then
      touch "$CACHE_FILE"
    else
      red "[pre-commit] clang-tidy issues in $f" >&2
      FAIL=1
    fi
  done
  if [[ $FAIL -ne 0 ]]; then
    red "[pre-commit] clang-tidy issues detected" >&2
  fi
fi

green "[pre-commit] All checks passed"
exit 0
