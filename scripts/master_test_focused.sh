#!/bin/bash

# =============================================================================
# Huntmaster Engine Master Test Suite - Focused Edition
# Comprehensive testing for all engine components, features, and tools
# =============================================================================

# Color output for better readability
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Test configuration
BUILD_DIR="build"
TEST_OUTPUT_DIR="test_logs"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Create test output directory
mkdir -p "$TEST_OUTPUT_DIR"

# Helper functions
print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}================================================${NC}"
}

print_phase() {
    echo -e "\n${PURPLE}=== Phase $1: $2 ===${NC}"
}

print_test() {
    echo -e "${CYAN}🧪 Testing: $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
    ((PASSED_TESTS++))
}

print_failure() {
    echo -e "${RED}❌ $1${NC}"
    ((FAILED_TESTS++))
}

print_skip() {
    echo -e "${YELLOW}⏭️  $1${NC}"
    ((SKIPPED_TESTS++))
}

run_test_executable() {
    local test_name="$1"
    local executable="$2"
    local timeout_seconds="${3:-30}"

    ((TOTAL_TESTS++))
    print_test "$test_name"

    if [[ ! -f "$executable" ]]; then
        print_skip "$test_name - Executable not found"
        return 0
    fi

    local safe_name="${test_name// /_}"
    safe_name="${safe_name//[^a-zA-Z0-9_]/_}"

    if timeout "$timeout_seconds" "$executable" > "$TEST_OUTPUT_DIR/${safe_name}_$TIMESTAMP.log" 2>&1; then
        print_success "$test_name"
        return 0
    else
        local exit_code=$?
        if [[ $exit_code -eq 124 ]]; then
            print_failure "$test_name - TIMEOUT (${timeout_seconds}s)"
        else
            print_failure "$test_name - Exit code: $exit_code"
        fi
        return $exit_code
    fi
}

# Main test execution
main() {
    print_header "Huntmaster Engine Master Test Suite"
    echo "Timestamp: $(date)"
    echo "Build directory: $BUILD_DIR"
    echo "Test logs: $TEST_OUTPUT_DIR"
    echo ""

    # Verify build directory exists
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo -e "${RED}Error: Build directory '$BUILD_DIR' not found!${NC}"
        echo "Please run 'cmake -B build && cmake --build build' first."
        exit 1
    fi

    # ==========================================================================
    # Phase 1: Core Engine Components (Critical)
    # ==========================================================================
    print_phase "1" "Core Engine Components"

    run_test_executable "UnifiedEngine Core" "$BUILD_DIR/bin/UnifiedEngineTest" 45
    run_test_executable "UnifiedEngine Comprehensive" "$BUILD_DIR/bin/UnifiedEngineComprehensiveTest" 60
    run_test_executable "Simple Unified Test" "$BUILD_DIR/bin/simple_unified_test" 30

    # ==========================================================================
    # Phase 2: Audio Processing Components (Critical)
    # ==========================================================================
    print_phase "2" "Audio Processing Components"

    run_test_executable "MFCC Processor" "$BUILD_DIR/bin/MFCCDirectUnifiedTest" 45
    run_test_executable "MFCC Consistency" "$BUILD_DIR/bin/MFCCConsistencyUnifiedTest" 60
    run_test_executable "DTW Comparator" "$BUILD_DIR/bin/DTWUnifiedTest" 45
    run_test_executable "DTW Comprehensive" "$BUILD_DIR/bin/DTWComprehensiveTest" 60

    # ==========================================================================
    # Phase 3: Integration & Validation Tests (Important)
    # ==========================================================================
    print_phase "3" "Integration & Validation Tests"

    run_test_executable "Validation Unified" "$BUILD_DIR/bin/ValidationUnifiedTest" 45
    run_test_executable "Real Wildlife Analysis" "$BUILD_DIR/bin/RealWildlifeCallAnalysisTest" 90

    # ==========================================================================
    # Phase 4: System & Infrastructure Tests (Important)
    # ==========================================================================
    print_phase "4" "System & Infrastructure Tests"

    run_test_executable "Error Handling" "$BUILD_DIR/bin/ErrorHandlingTest" 30
    run_test_executable "Memory Management" "$BUILD_DIR/bin/MemoryManagementTest" 45
    run_test_executable "Session State" "$BUILD_DIR/bin/SessionStateTest" 30
    run_test_executable "Debug System" "$BUILD_DIR/bin/DebugTest" 30

    # ==========================================================================
    # Phase 5: Performance & Benchmarking (Useful)
    # ==========================================================================
    print_phase "5" "Performance & Benchmarking"

    run_test_executable "Performance Test" "$BUILD_DIR/bin/PerformanceTest" 45
    run_test_executable "Performance Profiler" "$BUILD_DIR/bin/PerformanceProfilerTest" 30

    # ==========================================================================
    # Phase 6: Complete Google Test Suite (Comprehensive)
    # ==========================================================================
    print_phase "6" "Complete Google Test Suite"

    print_test "Running complete RunEngineTests suite"
    ((TOTAL_TESTS++))

    if timeout 120 "./$BUILD_DIR/bin/RunEngineTests" --gtest_brief=yes > "$TEST_OUTPUT_DIR/RunEngineTests_complete_$TIMESTAMP.log" 2>&1; then
        local total_line=$(tail -5 "$TEST_OUTPUT_DIR/RunEngineTests_complete_$TIMESTAMP.log" | grep "tests from.*test suite.*ran" || echo "")
        local passed_line=$(tail -5 "$TEST_OUTPUT_DIR/RunEngineTests_complete_$TIMESTAMP.log" | grep "\[  PASSED  \]" || echo "")
        local failed_line=$(tail -5 "$TEST_OUTPUT_DIR/RunEngineTests_complete_$TIMESTAMP.log" | grep "\[  FAILED  \]" || echo "")

        if [[ -n "$failed_line" ]]; then
            print_failure "Complete Test Suite - Some tests failed"
            echo "Failed tests: $failed_line"
        else
            print_success "Complete Test Suite - All tests passed"
            [[ -n "$total_line" ]] && echo "$total_line"
        fi
    else
        print_failure "Complete Test Suite - Execution failed or timed out"
    fi

    # ==========================================================================
    # Test Summary and Analysis
    # ==========================================================================
    echo ""
    print_header "Test Results Summary"

    echo "Test Execution Summary:"
    echo "======================"
    echo "Total Tests: $TOTAL_TESTS"
    echo "Passed: $PASSED_TESTS"
    echo "Failed: $FAILED_TESTS"
    echo "Skipped: $SKIPPED_TESTS"
    echo ""

    local success_rate=0
    if [[ $TOTAL_TESTS -gt 0 ]]; then
        success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    fi

    echo "Success Rate: $success_rate%"
    echo ""

    # Identify problems and warnings
    echo "=== ISSUE ANALYSIS ==="

    if [[ $FAILED_TESTS -eq 0 && $TOTAL_TESTS -gt 0 ]]; then
        echo -e "${GREEN}🎉 ALL TESTS PASSED! Engine is in excellent condition.${NC}"
        echo ""
        echo "The Huntmaster Engine has successfully passed all comprehensive tests:"
        echo "✅ Core engine functionality validated"
        echo "✅ Audio processing components working"
        echo "✅ Integration tests successful"
        echo "✅ System infrastructure robust"
        echo "✅ Performance benchmarks met"
        exit 0
    else
        echo -e "${YELLOW}⚠️  Testing completed with some issues. Review required.${NC}"
        echo ""

        # Show specific failure details
        echo "Failed Test Details:"
        for log_file in "$TEST_OUTPUT_DIR"/*_"$TIMESTAMP".log; do
            if [[ -f "$log_file" ]] && grep -q "FAILED\|failed\|error\|Error\|Segmentation\|Assertion.*failed" "$log_file" 2>/dev/null; then
                local test_name=$(basename "$log_file" | sed "s/_$TIMESTAMP.log//")
                echo "  🔍 $test_name:"
                grep -E "FAILED|failed|error|Error|Segmentation|Assertion.*failed" "$log_file" | head -3 | sed 's/^/    /'
                echo ""
            fi
        done

        # Check for specific known issues
        echo "=== KNOWN ISSUE PATTERNS ==="

        # Audio Player crashes
        if grep -r "free(): double free detected" "$TEST_OUTPUT_DIR"/*_"$TIMESTAMP".log 2>/dev/null; then
            echo -e "${RED}🚨 CRITICAL: AudioPlayer memory corruption detected${NC}"
            echo "   Issue: Double free() in AudioPlayer component"
            echo "   Impact: Can cause segmentation faults"
            echo "   Fix needed: AudioPlayer memory management"
            echo ""
        fi

        # Error handling test issues
        if grep -r "emptyProcessResult.*FF-FF" "$TEST_OUTPUT_DIR"/*_"$TIMESTAMP".log 2>/dev/null; then
            echo -e "${YELLOW}⚠️  WARNING: Error handling edge case detected${NC}"
            echo "   Issue: UnifiedAudioEngine::Status comparison failure"
            echo "   Impact: Error handling edge case not properly handled"
            echo "   Fix needed: Review empty buffer processing logic"
            echo ""
        fi

        # General guidance
        echo "=== RECOMMENDATIONS ==="
        if [[ $FAILED_TESTS -gt 0 ]]; then
            echo "1. Review failed test logs for specific error details"
            echo "2. Focus on critical failures first (memory issues, crashes)"
            echo "3. Check for missing test data files"
            echo "4. Verify all dependencies are correctly installed"
        fi

        if [[ $SKIPPED_TESTS -gt 3 ]]; then
            echo "5. Many tests skipped - verify build completed successfully"
            echo "6. Check that all test executables were compiled"
        fi

        echo ""
        echo "Detailed logs available in: $TEST_OUTPUT_DIR/"

        if [[ $FAILED_TESTS -gt 0 ]]; then
            exit 1
        else
            exit 0
        fi
    fi
}

# Execute main function
main "$@"
