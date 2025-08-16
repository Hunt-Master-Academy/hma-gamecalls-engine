#!/bin/bash

# =============================================================================
# Unified Coverage Analysis Script - Huntmaster Audio Engine
# =============================================================================
# Consolidated coverage analysis tool combining functionality from:
# - comprehensive_coverage_analysis.sh
# - comprehensive_coverage_analysis_robust.sh
# - enhanced_coverage_analysis.sh
# - test_coverage_analysis.sh
# =============================================================================

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="build/coverage"
COVERAGE_OUTPUT_DIR="$PROJECT_ROOT/coverage_analysis"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
COVERAGE_REPORT="$COVERAGE_OUTPUT_DIR/unified_coverage_$TIMESTAMP.log"
COVERAGE_TARGET=90

# Analysis modes
MODE="comprehensive"  # Default mode
SKIP_BUILD=false
VERBOSE=false

# Create output directory
mkdir -p "$COVERAGE_OUTPUT_DIR"

print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}================================================${NC}"
}

print_section() {
    echo -e "\n${PURPLE}=== $1 ===${NC}"
}

print_status() {
    echo -e "${CYAN}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Coverage Analysis Modes:"
    echo "  --basic         Quick coverage overview"
    echo "  --comprehensive Complete coverage analysis (default)"
    echo "  --robust        Detailed analysis with debugging info"
    echo ""
    echo "Options:"
    echo "  --skip-build    Skip building coverage target"
    echo "  --verbose       Enable verbose output"
    echo "  --target=N      Set coverage target percentage (default: 90)"
    echo "  --help          Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                          # Comprehensive analysis"
    echo "  $0 --basic                  # Quick coverage check"
    echo "  $0 --robust --verbose       # Detailed analysis with debug info"
    echo "  $0 --skip-build --target=85 # Analyze existing data with 85% target"
}

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --basic)
                MODE="basic"
                shift
                ;;
            --comprehensive)
                MODE="comprehensive"
                shift
                ;;
            --robust)
                MODE="robust"
                shift
                ;;
            --skip-build)
                SKIP_BUILD=true
                shift
                ;;
            --verbose)
                VERBOSE=true
                shift
                ;;
            --target=*)
                COVERAGE_TARGET="${1#*=}"
                shift
                ;;
            --help|-h)
                show_usage
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
}

check_prerequisites() {
    print_section "CHECKING PREREQUISITES"

    if ! command -v gcov >/dev/null 2>&1; then
        print_error "gcov not found. Please install gcc development tools."
        exit 1
    fi

    if ! command -v cmake >/dev/null 2>&1; then
        print_error "cmake not found. Please install CMake."
        exit 1
    fi

    print_success "Prerequisites check passed"
}

build_coverage_target() {
    if [[ "$SKIP_BUILD" == true ]]; then
        print_status "Skipping build (--skip-build specified)"
        return
    fi

    print_section "BUILDING COVERAGE TARGET"

    cd "$PROJECT_ROOT"

    # Configure coverage build
    print_status "Configuring coverage build..."
    if ! cmake --preset docker-coverage >/dev/null 2>&1; then
        print_error "Failed to configure coverage build"
        exit 1
    fi

    # Build with coverage
    print_status "Building with coverage instrumentation..."
    if ! cmake --build "$BUILD_DIR" --parallel >/dev/null 2>&1; then
        print_error "Failed to build coverage target"
        exit 1
    fi

    print_success "Coverage build completed"
}

execute_tests() {
    print_section "EXECUTING TESTS WITH COVERAGE"

    cd "$PROJECT_ROOT"

    local test_executable="$BUILD_DIR/bin/RunEngineTests"

    if [[ ! -f "$test_executable" ]]; then
        print_error "Test executable not found: $test_executable"
        exit 1
    fi

    print_status "Running test suite..."

    # Run tests and capture results
    local test_output
    if test_output=$(timeout 120 "$test_executable" --gtest_brief=yes 2>&1); then
        local successful_tests=$(echo "$test_output" | grep -c "PASSED" || echo "0")
        local failed_tests=$(echo "$test_output" | grep -c "FAILED" || echo "0")

        print_success "Tests completed: $successful_tests passed, $failed_tests failed"

        if [[ "$VERBOSE" == true ]]; then
            echo "$test_output" | tail -20
        fi
    else
        print_warning "Some tests failed, but continuing with coverage analysis"
    fi
}

analyze_basic_coverage() {
    print_section "BASIC COVERAGE ANALYSIS"

    cd "$PROJECT_ROOT/$BUILD_DIR"

    local gcno_count=$(find . -name "*.gcno" | wc -l)
    local gcda_count=$(find . -name "*.gcda" | wc -l)

    echo "Coverage files found:" | tee -a "$COVERAGE_REPORT"
    echo "  Instrumentation files (.gcno): $gcno_count" | tee -a "$COVERAGE_REPORT"
    echo "  Data files (.gcda): $gcda_count" | tee -a "$COVERAGE_REPORT"

    if [[ $gcda_count -eq 0 ]]; then
        print_error "No coverage data found. Tests may not have run successfully."
        exit 1
    fi

    # Quick overall coverage estimate
    local coverage_estimate
    if command -v lcov >/dev/null 2>&1; then
        print_status "Generating quick coverage summary with lcov..."
        if lcov --capture --directory . --output-file coverage.info >/dev/null 2>&1; then
            coverage_estimate=$(lcov --summary coverage.info 2>/dev/null | grep "lines" | grep -o "[0-9.]*%" | head -1)
            print_success "Estimated overall coverage: $coverage_estimate"
        fi
    fi

    cd "$PROJECT_ROOT"
}

analyze_comprehensive_coverage() {
    print_section "COMPREHENSIVE COVERAGE ANALYSIS"

    cd "$PROJECT_ROOT/$BUILD_DIR"

    local total_lines=0
    local covered_lines=0
    local total_files=0
    local files_above_threshold=0
    local coverage_threshold=75

    # Core source files to analyze
    local -a core_files=(
        "UnifiedAudioEngine.cpp"
        "MFCCProcessor.cpp"
        "DTWComparator.cpp"
        "VoiceActivityDetector.cpp"
        "ErrorLogger.cpp"
        "PitchTracker.cpp"
        "HarmonicAnalyzer.cpp"
        "CadenceAnalyzer.cpp"
        "EnhancedAnalysisProcessor.cpp"
        "AudioLevelProcessor.cpp"
        "RealtimeScorer.cpp"
    )

    echo "Per-file coverage analysis:" | tee -a "$COVERAGE_REPORT"
    echo "=============================" | tee -a "$COVERAGE_REPORT"

    for source_file in "${core_files[@]}"; do
        local gcno_file=$(find . -name "${source_file%.cpp}.cpp.gcno" 2>/dev/null | head -1)

        if [[ -n "$gcno_file" ]]; then
            local gcov_output=$(gcov -n "$gcno_file" 2>/dev/null || echo "No coverage data")

            if [[ "$gcov_output" != "No coverage data" ]]; then
                local coverage_line=$(echo "$gcov_output" | grep "Lines executed:" | head -1)

                if [[ -n "$coverage_line" ]]; then
                    local percent=$(echo "$coverage_line" | sed 's/.*executed:\([0-9.]*\)%.*/\1/')
                    local lines_info=$(echo "$coverage_line" | sed 's/.*executed:[0-9.]*% of \([0-9]*\).*/\1/')
                    local covered=$(echo "$coverage_line" | sed 's/.*executed:\([0-9.]*\)% of \([0-9]*\).*/\1 \2/' | awk '{printf "%.0f", $1 * $2 / 100}')

                    printf "%-30s: %6s%% (%s/%s lines)\n" "$source_file" "$percent" "$covered" "$lines_info" | tee -a "$COVERAGE_REPORT"

                    total_lines=$((total_lines + lines_info))
                    covered_lines=$((covered_lines + covered))
                    ((total_files++))

                    if (( $(echo "$percent >= $coverage_threshold" | bc -l) )); then
                        ((files_above_threshold++))
                    fi
                else
                    printf "%-30s: %6s (no data)\n" "$source_file" "N/A" | tee -a "$COVERAGE_REPORT"
                fi
            else
                printf "%-30s: %6s (not instrumented)\n" "$source_file" "N/A" | tee -a "$COVERAGE_REPORT"
            fi
        else
            printf "%-30s: %6s (not found)\n" "$source_file" "MISSING" | tee -a "$COVERAGE_REPORT"
        fi
    done

    echo "" | tee -a "$COVERAGE_REPORT"
    echo "OVERALL COVERAGE SUMMARY:" | tee -a "$COVERAGE_REPORT"
    echo "========================" | tee -a "$COVERAGE_REPORT"

    if [[ $total_lines -gt 0 ]]; then
        local overall_coverage=$(( (covered_lines * 100) / total_lines ))

        echo "Files analyzed: $total_files" | tee -a "$COVERAGE_REPORT"
        echo "Total lines: $total_lines" | tee -a "$COVERAGE_REPORT"
        echo "Covered lines: $covered_lines" | tee -a "$COVERAGE_REPORT"
        echo "Overall coverage: ${overall_coverage}%" | tee -a "$COVERAGE_REPORT"
        echo "Files above ${coverage_threshold}%: $files_above_threshold/$total_files" | tee -a "$COVERAGE_REPORT"

        local gap=$((COVERAGE_TARGET - overall_coverage))
        if [[ $gap -gt 0 ]]; then
            echo "Gap to target (${COVERAGE_TARGET}%): ${gap}%" | tee -a "$COVERAGE_REPORT"
        else
            print_success "Coverage target achieved! (${overall_coverage}% >= ${COVERAGE_TARGET}%)"
        fi

        # Store overall coverage for return value
        echo "$overall_coverage" > "$COVERAGE_OUTPUT_DIR/last_coverage_result.txt"
    else
        print_error "Unable to calculate coverage - no valid source files found"
        exit 1
    fi

    cd "$PROJECT_ROOT"
}

analyze_robust_coverage() {
    print_section "ROBUST COVERAGE ANALYSIS WITH DEBUGGING INFO"

    # Run comprehensive analysis first
    analyze_comprehensive_coverage

    # Additional debugging information
    print_section "DEBUGGING CAPABILITY ASSESSMENT"

    local debugging_features=0

    cd "$PROJECT_ROOT"

    # Check for debugging features
    if [[ -f "$BUILD_DIR/bin/DebugTest" ]]; then
        print_success "Debug logging tests available"
        ((debugging_features++))
    fi

    if grep -r "DebugLogger" "$PROJECT_ROOT/src" >/dev/null 2>&1; then
        print_success "Debug logging system implemented"
        ((debugging_features++))
    fi

    if grep -r "ERROR\|WARN\|INFO\|DEBUG" "$PROJECT_ROOT/src" >/dev/null 2>&1; then
        print_success "Comprehensive logging implemented"
        ((debugging_features++))
    fi

    if find "$PROJECT_ROOT/tests" -name "*error*" -o -name "*debug*" | head -1 >/dev/null; then
        print_success "Error handling tests present"
        ((debugging_features++))
    fi

    if [[ -f "$PROJECT_ROOT/src/core/ErrorLogger.cpp" ]]; then
        print_success "Dedicated error logging component"
        ((debugging_features++))
    fi

    echo ""
    echo "Debugging capability score: $debugging_features/5" | tee -a "$COVERAGE_REPORT"

    if [[ $debugging_features -ge 4 ]]; then
        print_success "Excellent debugging support!"
    elif [[ $debugging_features -ge 3 ]]; then
        print_success "Good debugging support"
    else
        print_warning "Debugging support needs improvement"
    fi
}

generate_recommendations() {
    print_section "IMPROVEMENT RECOMMENDATIONS"

    echo "Based on the coverage analysis, here are actionable recommendations:" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"

    # Read the last coverage result
    local overall_coverage=0
    if [[ -f "$COVERAGE_OUTPUT_DIR/last_coverage_result.txt" ]]; then
        overall_coverage=$(cat "$COVERAGE_OUTPUT_DIR/last_coverage_result.txt")
    fi

    if [[ $overall_coverage -lt $COVERAGE_TARGET ]]; then
        echo "PRIORITY ACTIONS:" | tee -a "$COVERAGE_REPORT"
        echo "1. 🎯 Focus on files with <75% coverage" | tee -a "$COVERAGE_REPORT"
        echo "2. 🧪 Add tests for error handling paths" | tee -a "$COVERAGE_REPORT"
        echo "3. 🔍 Test edge cases and boundary conditions" | tee -a "$COVERAGE_REPORT"
        echo "4. 🏗️ Add branch coverage for conditional statements" | tee -a "$COVERAGE_REPORT"
        echo "5. 🔧 Test configuration parameter combinations" | tee -a "$COVERAGE_REPORT"
    else
        echo "MAINTENANCE ACTIONS:" | tee -a "$COVERAGE_REPORT"
        echo "1. ✅ Maintain current high coverage level" | tee -a "$COVERAGE_REPORT"
        echo "2. 🔍 Focus on code quality and test effectiveness" | tee -a "$COVERAGE_REPORT"
        echo "3. 📊 Monitor coverage regression in future changes" | tee -a "$COVERAGE_REPORT"
    fi

    echo "" | tee -a "$COVERAGE_REPORT"
    echo "NEXT STEPS:" | tee -a "$COVERAGE_REPORT"
    echo "- Review detailed report: $COVERAGE_REPORT" | tee -a "$COVERAGE_REPORT"
    echo "- Re-run analysis after improvements to track progress" | tee -a "$COVERAGE_REPORT"
    echo "- Consider integrating coverage checks into CI/CD pipeline" | tee -a "$COVERAGE_REPORT"
}

main() {
    parse_arguments "$@"

    print_header "UNIFIED COVERAGE ANALYSIS - $MODE MODE"

    echo "🎯 Coverage Target: ${COVERAGE_TARGET}%" | tee "$COVERAGE_REPORT"
    echo "📅 Analysis Date: $(date)" | tee -a "$COVERAGE_REPORT"
    echo "🔧 Analysis Mode: $MODE" | tee -a "$COVERAGE_REPORT"
    echo "📍 Project: Huntmaster Engine" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"

    check_prerequisites
    build_coverage_target
    execute_tests

    case "$MODE" in
        "basic")
            analyze_basic_coverage
            ;;
        "comprehensive")
            analyze_comprehensive_coverage
            ;;
        "robust")
            analyze_robust_coverage
            ;;
    esac

    generate_recommendations

    print_header "ANALYSIS COMPLETE"
    echo "📄 Detailed report saved to: $COVERAGE_REPORT"
    echo "📊 Analysis timestamp: $TIMESTAMP"
    echo ""

    # Return appropriate exit code based on coverage target
    if [[ -f "$COVERAGE_OUTPUT_DIR/last_coverage_result.txt" ]]; then
        local final_coverage=$(cat "$COVERAGE_OUTPUT_DIR/last_coverage_result.txt")
        if [[ $final_coverage -ge $COVERAGE_TARGET ]]; then
            print_success "Coverage target achieved: ${final_coverage}% >= ${COVERAGE_TARGET}%"
            exit 0
        else
            print_warning "Coverage below target: ${final_coverage}% < ${COVERAGE_TARGET}%"
            exit 1
        fi
    fi
}

# Execute main function with all arguments
main "$@"
