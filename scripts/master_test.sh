#!/usr/bin/env bash
set -euo pipefail

# Consolidated Master Test Orchestrator (v2)
# Single entry point for: unit/integration tests, diagnostics-off variant, tool smoke tests,
# dynamic executable discovery, optional coverage, XML export, and configurable phases.

VERSION="2.0"
PROJECT_ROOT="$(cd -- "$(dirname -- "$0")/.." && pwd)"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
TEST_OUTPUT_DIR="$PROJECT_ROOT/test_logs"
mkdir -p "$TEST_OUTPUT_DIR"

# Defaults / env overrides
BUILD_DIR="${BUILD_DIR:-build}"
PHASES_DEFAULT="unit,diagnostics-off,tools,discovery,coverage"
PHASES="${PHASES:-$PHASES_DEFAULT}"
COVERAGE="${COVERAGE:-false}"        # or --coverage flag
ENFORCE_COVERAGE="${ENFORCE_COVERAGE:-false}"
COVERAGE_TARGET="${COVERAGE_TARGET:-90}"
FAST_FAIL="${FAST_FAIL:-false}"
GTEST_FILTER="${GTEST_FILTER:-}"      # optional pattern
XML="${XML:-false}"
SUITE_TIMEOUT="${SUITE_TIMEOUT:-120}"
NO_COLOR="${NO_COLOR:-false}"
TOOLS_LIST_DEFAULT="generate_features,debug_dtw_similarity,performance_profiling_demo"
TOOLS_LIST="${TOOLS_LIST:-$TOOLS_LIST_DEFAULT}"

# Colors
if [[ "$NO_COLOR" == "true" || ! -t 1 ]]; then
    RED=""; GREEN=""; YELLOW=""; BLUE=""; PURPLE=""; CYAN=""; NC=""
else
    RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; PURPLE='\033[0;35m'; CYAN='\033[0;36m'; NC='\033[0m'
fi

TOTAL=0; PASSED=0; FAILED=0; SKIPPED=0
LOG_MAIN="$TEST_OUTPUT_DIR/master_test_${TIMESTAMP}.log"
COVERAGE_LOG="$TEST_OUTPUT_DIR/coverage_${TIMESTAMP}.log"

log() { echo -e "$@" | tee -a "$LOG_MAIN"; }
ok() { echo -e "${GREEN}✅ $1${NC}" | tee -a "$LOG_MAIN"; ((PASSED+=1)); }
fail() { echo -e "${RED}❌ $1${NC}" | tee -a "$LOG_MAIN"; ((FAILED+=1)); }
skip() { echo -e "${YELLOW}⏭️  $1${NC}" | tee -a "$LOG_MAIN"; ((SKIPPED+=1)); }
hdr() { echo -e "${BLUE}================================================${NC}\n${BLUE}  $1${NC}\n${BLUE}================================================${NC}" | tee -a "$LOG_MAIN"; }

usage() {
    cat <<EOF
Huntmaster Master Test (v$VERSION)
Usage: $0 [options]
    --coverage                 Enable coverage collection (COVERAGE=true)
    --enforce-coverage[=N]     Fail if coverage < N (default env COVERAGE_TARGET=$COVERAGE_TARGET)
    --phases=a,b,c             Override phases (default: $PHASES_DEFAULT)
    --gtest-filter=PATTERN     Pass gtest filter (e.g. "*Cadence*:-*Slow*")
    --xml                      Emit gtest XML (test_logs/gtest_${TIMESTAMP}.xml)
    --fast-fail                Abort on first failure
    --timeout-suite=SEC        Override suite timeout (default $SUITE_TIMEOUT)
    --tools=list               Comma list tool names to smoke (default $TOOLS_LIST_DEFAULT)
    --no-color                 Disable ANSI colors
    --list-phases              Show phase order & exit
    --help                     Show this help
Phases: unit, diagnostics-off, tools, discovery, coverage
Environment overrides also accepted (PHASES, COVERAGE, etc.)
EOF
}

if [[ $# -gt 0 ]]; then
    for arg in "$@"; do
        case "$arg" in
            --help) usage; exit 0;;
            --coverage) COVERAGE=true;;
            --xml) XML=true;;
            --fast-fail) FAST_FAIL=true;;
            --no-color) NO_COLOR=true;;
            --list-phases) echo "$PHASES"; exit 0;;
            --phases=*) PHASES="${arg#*=}";;
            --gtest-filter=*) GTEST_FILTER="${arg#*=}";;
            --timeout-suite=*) SUITE_TIMEOUT="${arg#*=}";;
            --tools=*) TOOLS_LIST="${arg#*=}";;
            --enforce-coverage|--enforce-coverage=*) ENFORCE_COVERAGE=true; v="${arg#*=}"; [[ "$v" != "--enforce-coverage" && -n "$v" ]] && COVERAGE_TARGET="$v";;
            *) echo "Unknown option: $arg"; usage; exit 1;;
        esac
    done
fi

resolve_build_dir() {
    local candidates=("$BUILD_DIR" build/debug build/release build/asan build/coverage build/ubsan build)
    for d in "${candidates[@]}"; do
        if [[ -d "$PROJECT_ROOT/$d/bin" ]]; then BUILD_DIR="$d"; break; fi
    done
    BIN_DIR="$PROJECT_ROOT/$BUILD_DIR/bin"
    if [[ ! -d "$BIN_DIR" ]]; then
        echo "Build binaries not found in candidates. Please build the project." >&2; exit 2
    fi
}

run_cmd() { # usage: run_cmd name timeout cmd arg1 arg2 ...
    local name="$1"; local to="$2"; shift 2
        echo "[DEBUG] entering run_cmd with args: $# -> $*" | tee -a "$LOG_MAIN"
    ((TOTAL+=1))
    local log_file="$TEST_OUTPUT_DIR/${name// /_}_$TIMESTAMP.log"
    echo -e "→ $name : $* (timeout=${to}s)" | tee -a "$LOG_MAIN"
    if [[ $# -eq 0 ]]; then fail "$name (no command)"; return 1; fi
    set +e
    timeout "$to" "$@" >"$log_file" 2>&1
    local ec=$?
    set -e
    if [[ $ec -eq 0 ]]; then
        ok "$name"
        [[ "$XML" == true && "$name" == "RunEngineTests" ]] && cp "$log_file" "$TEST_OUTPUT_DIR/raw_${TIMESTAMP}.log" || true
    else
        fail "$name (exit $ec)"
        tail -n 40 "$log_file" | sed 's/^/  │ /'
        if [[ "$FAST_FAIL" == true ]]; then summary; exit 1; fi
    fi
}

phase_unit() {
    hdr "Phase: unit (RunEngineTests)"
    local gtest_bin="$BIN_DIR/RunEngineTests"
    if [[ ! -x "$gtest_bin" ]]; then skip "RunEngineTests binary missing"; return; fi
    local cmd=("$gtest_bin" "--gtest_brief=yes")
    [[ -n "$GTEST_FILTER" ]] && cmd+=("--gtest_filter=$GTEST_FILTER")
    if [[ "$XML" == true ]]; then cmd+=("--gtest_output=xml:$TEST_OUTPUT_DIR/gtest_${TIMESTAMP}.xml"); fi
    echo "[DEBUG] phase_unit invoking run_cmd with: ${cmd[*]}" | tee -a "$LOG_MAIN"
    run_cmd "RunEngineTests" "$SUITE_TIMEOUT" ${cmd[@]}
}

phase_diagnostics_off() {
    hdr "Phase: diagnostics-off"
    local bin="$BIN_DIR/RunEngineNoDiagTests"
    [[ -x "$bin" ]] || { skip "No diagnostics-off test binary"; return; }
    run_cmd "RunEngineNoDiagTests" "$SUITE_TIMEOUT" "$bin" --gtest_brief=yes
}

phase_tools() {
    hdr "Phase: tools smoke"
    IFS=',' read -r -a tools <<<"$TOOLS_LIST"
    for t in "${tools[@]}"; do
        local path="$BIN_DIR/$t"
        if [[ -x "$path" ]]; then
            run_cmd "tool:$t --help" 10 "$path" --help
        else
            skip "tool:$t (not built)"
        fi
    done
}

phase_discovery() {
    hdr "Phase: discovery"
    local known="^(RunEngineTests|RunEngineNoDiag|RunEngineNoDiagTests|${TOOLS_LIST//,/|})$"
    local found_any=false
    for f in "$BIN_DIR"/*; do
        [[ -x "$f" && -f "$f" ]] || continue
        local base="$(basename "$f")"
        if [[ "$base" =~ $known ]]; then continue; fi
        found_any=true
    run_cmd "discover:$base --help" 8 "$f" --help || true
    done
    [[ "$found_any" == false ]] && skip "no additional executables"
}

phase_coverage() {
    [[ "$COVERAGE" == true ]] || { skip "coverage disabled"; return; }
    hdr "Phase: coverage"
    # Simple gcov line coverage (fallback if gcovr not present)
    if command -v gcovr >/dev/null 2>&1; then
        (cd "$PROJECT_ROOT/$BUILD_DIR" && gcovr -r "$PROJECT_ROOT/src" --exclude '.*_deps.*' --xml -o "$TEST_OUTPUT_DIR/coverage_${TIMESTAMP}.xml" --txt >"$COVERAGE_LOG" 2>&1 || true)
        # Attempt to parse line coverage percentage from gcovr text output
        local pct=""
        if grep -E 'lines:' "$COVERAGE_LOG" >/dev/null 2>&1; then
            # Format example: lines: 83.45% (1234 out of 1480)
            pct=$(grep -E 'lines:' "$COVERAGE_LOG" | tail -1 | sed -E 's/.*lines:\s*([0-9]+\.?[0-9]*)%.*/\1/')
        elif grep -E '^TOTAL' "$COVERAGE_LOG" >/dev/null 2>&1; then
            pct=$(grep -E '^TOTAL' "$COVERAGE_LOG" | awk '{for(i=1;i<=NF;i++){if($i ~ /%/){gsub("%","",$i); print $i; exit}}}')
        fi
        if [[ -n "$pct" ]]; then
            # Truncate decimal
            local pct_int=${pct%.*}
            echo "Parsed Line Coverage (gcovr): ${pct}%" | tee -a "$COVERAGE_LOG"
            if [[ "$ENFORCE_COVERAGE" == true && $pct_int -lt $COVERAGE_TARGET ]]; then
                fail "Coverage below target (${pct_int}% < ${COVERAGE_TARGET}%)"
            else
                ok "Coverage check passed (${pct_int}% >= target ${COVERAGE_TARGET}%)"
            fi
        else
            skip "Could not parse coverage percentage from gcovr output"
        fi
    else
        (cd "$PROJECT_ROOT/$BUILD_DIR" && find . -name '*.gcno' -exec gcov {} + >/dev/null 2>&1 || true)
        local total=0; local hit=0
        for g in "$PROJECT_ROOT/$BUILD_DIR"/*.gcov; do
            [[ -f "$g" ]] || continue
            local t h
            t=$(grep -c ':' "$g" || echo 0)
            h=$(grep -c '^[ ]*[1-9]' "$g" || echo 0)
            total=$((total + t)); hit=$((hit + h))
        done
        if [[ $total -gt 0 ]]; then
            local pct=$((hit * 100 / total))
            echo "Line Coverage: ${pct}% ($hit/$total)" | tee -a "$COVERAGE_LOG"
            if [[ "$ENFORCE_COVERAGE" == true && $pct -lt $COVERAGE_TARGET ]]; then
                fail "Coverage below target (${pct}% < ${COVERAGE_TARGET}%)"; return 1
            else
                ok "Coverage check passed (${pct}% >= target ${COVERAGE_TARGET}% )"
            fi
        else
            skip "No coverage data produced"
        fi
    fi
}

summary() {
    hdr "Summary"
    local success_rate=0
    [[ $TOTAL -gt 0 ]] && success_rate=$((PASSED * 100 / TOTAL))
    echo "Total: $TOTAL  Passed: $PASSED  Failed: $FAILED  Skipped: $SKIPPED  Success: ${success_rate}%" | tee -a "$LOG_MAIN"
    if [[ $FAILED -eq 0 ]]; then
        echo -e "${GREEN}ALL GREEN${NC}" | tee -a "$LOG_MAIN"
    else
        echo -e "${RED}FAILURES PRESENT${NC}" | tee -a "$LOG_MAIN"
    fi
}

print_env() {
    hdr "Environment"
    echo "Project Root: $PROJECT_ROOT" | tee -a "$LOG_MAIN"
    echo "Build Dir:    $BUILD_DIR" | tee -a "$LOG_MAIN"
    echo "Phases:       $PHASES" | tee -a "$LOG_MAIN"
    echo "Coverage:     $COVERAGE (enforce=$ENFORCE_COVERAGE target=$COVERAGE_TARGET)" | tee -a "$LOG_MAIN"
    echo "GTest Filter: ${GTEST_FILTER:-<none>}" | tee -a "$LOG_MAIN"
    echo "Tools:        $TOOLS_LIST" | tee -a "$LOG_MAIN"
    git -C "$PROJECT_ROOT" rev-parse --short HEAD 2>/dev/null | sed 's/^/Git Commit: /' | tee -a "$LOG_MAIN" || true
    (command -v g++ >/dev/null 2>&1 && g++ --version | head -1 | sed 's/^/Compiler: /') | tee -a "$LOG_MAIN" || true
}

main() {
    resolve_build_dir
    print_env
    IFS=',' read -r -a phase_array <<<"$PHASES"
    for p in "${phase_array[@]}"; do
        case "$p" in
            unit) phase_unit;;
            diagnostics-off) phase_diagnostics_off;;
            tools) phase_tools;;
            discovery) phase_discovery;;
            coverage) phase_coverage || true;;
            "") :;;
            *) skip "unknown phase '$p'";;
        esac
    done
    summary
    [[ $FAILED -eq 0 ]] || exit 1
}

trap 'echo "Interrupted" >&2; summary; exit 130' INT TERM
main "$@"
