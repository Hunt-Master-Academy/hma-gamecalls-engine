#!/usr/bin/env bash
set -euo pipefail

# Consolidated Master Test Orchestrator (v3.0)
# Single entry point for all testing needs: unit/integration tests, coverage analysis,
# environment-specific testing, performance benchmarks, and comprehensive test orchestration.
# Consolidates functionality from: master_test_focused.sh, master_test_with_coverage.sh,
# docker_coverage_test.sh, test_integration.sh, and test_integration_phase1.sh

VERSION="3.0"
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

# New enhanced options
MODE="${MODE:-comprehensive}"        # comprehensive, focused, fast, integration, unit, docker
ENVIRONMENT="${ENVIRONMENT:-auto}"   # auto, docker, wsl, ci, local
JSON_OUTPUT="${JSON_OUTPUT:-false}"
QUIET="${QUIET:-false}"
VERBOSE="${VERBOSE:-false}"
INTEGRATION_TESTS="${INTEGRATION_TESTS:-true}"
PERFORMANCE_TESTS="${PERFORMANCE_TESTS:-true}"
DOCKER_OPTIMIZED="${DOCKER_OPTIMIZED:-false}"

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
ok() { echo -e "${GREEN}[PASS] $1${NC}" | tee -a "$LOG_MAIN"; ((PASSED+=1)); }
fail() { echo -e "${RED}❌ $1${NC}" | tee -a "$LOG_MAIN"; ((FAILED+=1)); }
skip() { echo -e "${YELLOW}⏭️  $1${NC}" | tee -a "$LOG_MAIN"; ((SKIPPED+=1)); }
hdr() { echo -e "${BLUE}================================================${NC}\n${BLUE}  $1${NC}\n${BLUE}================================================${NC}" | tee -a "$LOG_MAIN"; }

usage() {
    cat <<EOF
Huntmaster Master Test Orchestrator (v$VERSION)
Single entry point for all testing needs in the Huntmaster Engine

Usage: $0 [options]

Testing Modes:
    --mode=MODE                Testing mode (comprehensive, focused, fast, integration, unit, docker)
                              comprehensive: All tests (default)
                              focused: Critical tests only (replaces master_test_focused.sh)
                              fast: Developer fast loop (similar to dev_fast_test.sh)
                              integration: Integration tests only
                              unit: Unit tests only
                              docker: Docker-optimized testing

Coverage Options:
    --coverage                 Enable coverage collection
    --coverage-only           Coverage analysis only (skip other tests)
    --enforce-coverage[=N]     Fail if coverage < N (default: $COVERAGE_TARGET)
    --target=N                 Set coverage target percentage

Environment Options:
    --environment=ENV          Target environment (auto, docker, wsl, ci, local)
    --docker                   Enable Docker-specific optimizations
    --ci                       Enable CI/CD optimizations

Test Configuration:
    --phases=a,b,c             Override phases (default: $PHASES_DEFAULT)
    --gtest-filter=PATTERN     Pass gtest filter (e.g. "*Cadence*:-*Slow*")
    --timeout-suite=SEC        Override suite timeout (default $SUITE_TIMEOUT)
    --tools=list               Comma list tool names to smoke

Output Options:
    --xml                      Emit gtest XML output
    --json                     Emit JSON test results
    --verbose                  Enable verbose output
    --quiet                    Minimal output only
    --no-color                 Disable ANSI colors

Control Options:
    --fast-fail                Abort on first failure
    --skip-integration         Skip integration tests
    --skip-performance         Skip performance tests
    --list-phases              Show available phases & exit
    --help                     Show this help

Available Phases: unit, integration, diagnostics-off, performance, tools, discovery, coverage
Environment variables: PHASES, COVERAGE, BUILD_DIR, etc. (see script for full list)

Examples:
    $0                              # Comprehensive testing
    $0 --mode=focused --fast-fail   # Quick focused testing with fast failure
    $0 --coverage --target=85       # Coverage testing with 85% target
    $0 --mode=docker --ci           # Docker CI testing
    $0 --mode=integration --verbose # Integration tests with verbose output
EOF
}

if [[ $# -gt 0 ]]; then
    for arg in "$@"; do
        case "$arg" in
            --help) usage; exit 0;;
            --coverage) COVERAGE=true;;
            --coverage-only) COVERAGE=true; PHASES="coverage";;
            --xml) XML=true;;
            --json) JSON_OUTPUT=true;;
            --verbose) VERBOSE=true; NO_COLOR=false;;
            --quiet) QUIET=true;;
            --fast-fail) FAST_FAIL=true;;
            --no-color) NO_COLOR=true;;
            --docker) DOCKER_OPTIMIZED=true; ENVIRONMENT="docker";;
            --ci) ENVIRONMENT="ci"; FAST_FAIL=true; XML=true;;
            --skip-integration) INTEGRATION_TESTS=false;;
            --skip-performance) PERFORMANCE_TESTS=false;;
            --list-phases) echo "Available phases: unit, integration, diagnostics-off, performance, tools, discovery, coverage"; exit 0;;
            --mode=*) MODE="${arg#*=}";;
            --environment=*) ENVIRONMENT="${arg#*=}";;
            --phases=*) PHASES="${arg#*=}";;
            --gtest-filter=*) GTEST_FILTER="${arg#*=}";;
            --timeout-suite=*) SUITE_TIMEOUT="${arg#*=}";;
            --tools=*) TOOLS_LIST="${arg#*=}";;
            --target=*) COVERAGE_TARGET="${arg#*=}";;
            --enforce-coverage|--enforce-coverage=*) ENFORCE_COVERAGE=true; v="${arg#*=}"; [[ "$v" != "--enforce-coverage" && -n "$v" ]] && COVERAGE_TARGET="$v";;
            *) echo "Unknown option: $arg"; usage; exit 1;;
        esac
    done
fi

# Auto-detect environment if set to auto
detect_environment() {
    if [[ "$ENVIRONMENT" == "auto" ]]; then
        if [[ -f /.dockerenv ]] || [[ -n "${CONTAINER:-}" ]]; then
            ENVIRONMENT="docker"
            DOCKER_OPTIMIZED=true
        elif grep -qi microsoft /proc/version 2>/dev/null; then
            ENVIRONMENT="wsl"
        elif [[ -n "${CI:-}" ]] || [[ -n "${GITHUB_ACTIONS:-}" ]] || [[ -n "${GITLAB_CI:-}" ]]; then
            ENVIRONMENT="ci"
            FAST_FAIL=true
            XML=true
        else
            ENVIRONMENT="local"
        fi
    fi
}

# Configure phases based on mode
configure_mode() {
    case "$MODE" in
        "comprehensive")
            PHASES="unit,integration,diagnostics-off,performance,tools,discovery,coverage"
            ;;
        "focused")
            PHASES="unit,tools,coverage"
            GTEST_FILTER="${GTEST_FILTER:-*MasterCallsComparison*:*RealtimeSimilarity*:*Core*}"
            SUITE_TIMEOUT=60
            ;;
        "fast")
            PHASES="unit"
            GTEST_FILTER="${GTEST_FILTER:-*MasterCallsComparison*:*RealtimeSimilarity*}"
            SUITE_TIMEOUT=40
            FAST_FAIL=true
            ;;
        "integration")
            PHASES="integration,tools"
            ;;
        "unit")
            PHASES="unit,diagnostics-off"
            ;;
        "performance")
            PHASES="performance,tools"
            SUITE_TIMEOUT=120
            ;;
        "docker")
            PHASES="unit,integration,coverage"
            DOCKER_OPTIMIZED=true
            SUITE_TIMEOUT=180
            ;;
        *)
            echo "Unknown mode: $MODE. Available: comprehensive, focused, fast, integration, unit, performance, docker"
            exit 1
            ;;
    esac

    # Disable phases based on flags
    if [[ "$INTEGRATION_TESTS" == false ]]; then
        PHASES=$(echo "$PHASES" | sed 's/integration,\|,integration\|integration//g')
    fi
    if [[ "$PERFORMANCE_TESTS" == false ]]; then
        PHASES=$(echo "$PHASES" | sed 's/performance,\|,performance\|performance//g')
    fi
}

resolve_build_dir() {
    local candidates=("$BUILD_DIR" build/debug build/release build/asan build/coverage build/ubsan build)
    for d in "${candidates[@]}"; do
        if [[ -d "$PROJECT_ROOT/$d/bin" ]] || [[ -d "$PROJECT_ROOT/$d/tests" ]]; then
            BUILD_DIR="$d"; break;
        fi
    done

    # Check for binaries in both bin and tests directories
    BIN_DIR="$PROJECT_ROOT/$BUILD_DIR/bin"
    TEST_BIN_DIR="$PROJECT_ROOT/$BUILD_DIR/tests"

    if [[ ! -d "$BIN_DIR" ]] && [[ ! -d "$TEST_BIN_DIR" ]]; then
        echo "Build binaries not found in candidates. Please build the project." >&2; exit 2
    fi

    # Prefer tests directory if RunEngineTests is there, otherwise use bin
    if [[ -f "$TEST_BIN_DIR/RunEngineTests" ]]; then
        BIN_DIR="$TEST_BIN_DIR"
    elif [[ ! -d "$BIN_DIR" ]] && [[ -d "$TEST_BIN_DIR" ]]; then
        BIN_DIR="$TEST_BIN_DIR"
    fi
}run_cmd() { # usage: run_cmd name timeout cmd arg1 arg2 ...
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

phase_integration() {
    hdr "Phase: integration"
    [[ "$INTEGRATION_TESTS" == true ]] || { skip "integration tests disabled"; return; }

    # Test 1: Enhanced WASM Interface files
    if [[ -f "$PROJECT_ROOT/include/huntmaster/platform/wasm/EnhancedWASMInterface.h" ]] &&
       [[ -f "$PROJECT_ROOT/src/platform/wasm/EnhancedWASMInterface.cpp" ]]; then
        ok "Enhanced WASM Interface files exist"
    else
        skip "Enhanced WASM Interface files not found"
    fi

    # Test 2: Core integration test executable
    local integration_bin="$BIN_DIR/IntegrationTest"
    if [[ -x "$integration_bin" ]]; then
        run_cmd "IntegrationTest" "$SUITE_TIMEOUT" "$integration_bin" --gtest_brief=yes
    else
        skip "IntegrationTest binary not found"
    fi

    # Test 3: WASM build artifacts validation
    if [[ -d "$PROJECT_ROOT/web/dist" ]]; then
        local wasm_files=$(find "$PROJECT_ROOT/web/dist" -name "*.wasm" | wc -l)
        if [[ $wasm_files -gt 0 ]]; then
            ok "WASM build artifacts found ($wasm_files files)"
        else
            skip "No WASM artifacts in web/dist"
        fi
    else
        skip "web/dist directory not found"
    fi

    # Test 4: API integration test
    local api_test="$BIN_DIR/APIIntegrationTest"
    if [[ -x "$api_test" ]]; then
        run_cmd "APIIntegrationTest" 60 "$api_test"
    else
        skip "APIIntegrationTest binary not found"
    fi
}

phase_performance() {
    hdr "Phase: performance"
    [[ "$PERFORMANCE_TESTS" == true ]] || { skip "performance tests disabled"; return; }

    # Performance benchmarks
    local perf_bin="$BIN_DIR/PerformanceTest"
    if [[ -x "$perf_bin" ]]; then
        run_cmd "PerformanceTest" 90 "$perf_bin" --benchmark_format=json --benchmark_out="$TEST_OUTPUT_DIR/benchmark_${TIMESTAMP}.json"
    else
        skip "PerformanceTest binary not found"
    fi

    # Performance profiling demo
    local prof_demo="$BIN_DIR/performance_profiling_demo"
    if [[ -x "$prof_demo" ]]; then
        run_cmd "PerformanceProfilingDemo" 60 "$prof_demo"
    else
        skip "performance_profiling_demo not found"
    fi

    # Memory performance test
    local mem_test="$BIN_DIR/MemoryPerformanceTest"
    if [[ -x "$mem_test" ]]; then
        run_cmd "MemoryPerformanceTest" 45 "$mem_test"
    else
        skip "MemoryPerformanceTest binary not found"
    fi
}

phase_coverage() {
    [[ "$COVERAGE" == true ]] || { skip "coverage disabled"; return; }
    hdr "Phase: coverage"

    # Enhanced coverage analysis with Docker optimization
    local coverage_dir="$TEST_OUTPUT_DIR/coverage"
    mkdir -p "$coverage_dir"

    # Reset coverage counters for Docker environment
    if [[ "$DOCKER_OPTIMIZED" == true ]]; then
        find "$PROJECT_ROOT/$BUILD_DIR" -name "*.gcda" -delete 2>/dev/null || true
        lcov --directory "$PROJECT_ROOT/$BUILD_DIR" --zerocounters 2>/dev/null || true
    fi

    # Advanced coverage with gcovr (preferred)
    if command -v gcovr >/dev/null 2>&1; then
        local gcovr_opts=(
            -r "$PROJECT_ROOT/src"
            --exclude '.*_deps.*'
            --exclude '.*/tests/.*'
            --exclude '.*/build/.*'
        )

        # Generate multiple output formats
        (cd "$PROJECT_ROOT/$BUILD_DIR" && {
            gcovr "${gcovr_opts[@]}" --xml -o "$coverage_dir/coverage_${TIMESTAMP}.xml"
            gcovr "${gcovr_opts[@]}" --html-details -o "$coverage_dir/coverage_${TIMESTAMP}.html"
            gcovr "${gcovr_opts[@]}" --txt > "$COVERAGE_LOG"
        } 2>&1 || true)

        # Parse coverage percentage
        local pct=""
        if grep -E 'lines:' "$COVERAGE_LOG" >/dev/null 2>&1; then
            # Format example: lines: 83.45% (1234 out of 1480)
            pct=$(grep -E 'lines:' "$COVERAGE_LOG" | tail -1 | sed -E 's/.*lines:\s*([0-9]+\.?[0-9]*)%.*/\1/')
        elif grep -E '^TOTAL' "$COVERAGE_LOG" >/dev/null 2>&1; then
            pct=$(grep -E '^TOTAL' "$COVERAGE_LOG" | awk '{for(i=1;i<=NF;i++){if($i ~ /%/){gsub("%","",$i); print $i; exit}}}')
        fi

        if [[ -n "$pct" ]]; then
            local pct_int=${pct%.*}
            echo "Line Coverage (gcovr): ${pct}%" | tee -a "$COVERAGE_LOG"
            echo "Coverage report: $coverage_dir/coverage_${TIMESTAMP}.html" | tee -a "$COVERAGE_LOG"

            if [[ "$ENFORCE_COVERAGE" == true && $pct_int -lt $COVERAGE_TARGET ]]; then
                fail "Coverage below target (${pct_int}% < ${COVERAGE_TARGET}%)"
            else
                ok "Coverage check passed (${pct_int}% >= target ${COVERAGE_TARGET}%)"
            fi
        else
            skip "Could not parse coverage percentage from gcovr output"
        fi
    else
        # Fallback to basic gcov
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
                ok "Coverage check passed (${pct}% >= target ${COVERAGE_TARGET}%)"
            fi
        else
            skip "No coverage data produced"
        fi
    fi
}

summary() {
    hdr "Summary"
    TOTAL=$((PASSED + FAILED + SKIPPED))
    local success_rate=0
    [[ $TOTAL -gt 0 ]] && success_rate=$((PASSED * 100 / TOTAL))
    echo "Total: $TOTAL  Passed: $PASSED  Failed: $FAILED  Skipped: $SKIPPED  Success: ${success_rate}%" | tee -a "$LOG_MAIN"
    if [[ $FAILED -eq 0 ]]; then
        echo -e "${GREEN}ALL GREEN${NC}" | tee -a "$LOG_MAIN"
    else
        echo -e "${RED}FAILURES PRESENT${NC}" | tee -a "$LOG_MAIN"
    fi

    # JSON output if requested
    if [[ "$JSON_OUTPUT" == true ]]; then
        local json_file="$TEST_OUTPUT_DIR/test_results_${TIMESTAMP}.json"
        cat > "$json_file" << EOF
{
  "timestamp": "$TIMESTAMP",
  "mode": "$MODE",
  "environment": "$ENVIRONMENT",
  "phases": "$PHASES",
  "results": {
    "total": $TOTAL,
    "passed": $PASSED,
    "failed": $FAILED,
    "skipped": $SKIPPED,
    "success_rate": $success_rate
  },
  "configuration": {
    "project_root": "$PROJECT_ROOT",
    "build_dir": "$BUILD_DIR",
    "coverage": $COVERAGE,
    "coverage_target": $COVERAGE_TARGET,
    "gtest_filter": "${GTEST_FILTER:-null}",
    "tools": "$TOOLS_LIST",
    "git_commit": "$(git -C "$PROJECT_ROOT" rev-parse --short HEAD 2>/dev/null || echo "unknown")",
    "compiler": "$(g++ --version 2>/dev/null | head -1 || echo "unknown")"
  },
  "logs": {
    "main_log": "$LOG_MAIN",
    "coverage_log": "$COVERAGE_LOG"
  }
}
EOF
        echo "JSON results: $json_file"
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
            integration) phase_integration;;
            performance) phase_performance;;
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
