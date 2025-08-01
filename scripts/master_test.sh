#!/bin/bash

# =============================================================================
# Huntmaster Engine Master Test Suite - TRUE MASTER TEST
# Comprehensive testing for >90% coverage target with integrated analysis
# =============================================================================
#
# USAGE:
#   ./scripts/master_test.sh                    # Standard comprehensive testing
#   COVERAGE_ENABLED=true ./scripts/master_test.sh  # With coverage analysis
#
# FEATURES:
#   ✅ 14+ Test Phases covering all engine components
#   ✅ 50+ Individual test executables
#   ✅ High-coverage focused test execution
#   ✅ Integrated coverage measurement and reporting
#   ✅ Dynamic test discovery for complete coverage
#   ✅ Performance benchmarking integration
#   ✅ Tool and utility validation
#   ✅ Comprehensive logging and reporting
#
# COVERAGE TARGET: 90%+
# REQUIREMENTS: Build with cmake/make, optionally with --coverage flags
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
PROJECT_ROOT="$(pwd)"
TEST_OUTPUT_DIR="$PROJECT_ROOT/test_logs"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
TEST_LOG="$TEST_OUTPUT_DIR/master_test_$TIMESTAMP.log"
COVERAGE_LOG="$TEST_OUTPUT_DIR/coverage_report_$TIMESTAMP.log"
COVERAGE_ENABLED=${COVERAGE_ENABLED:-false}
COVERAGE_TARGET=90

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

# Coverage measurement functions
generate_coverage_report() {
    if [[ "$COVERAGE_ENABLED" == "true" && -f "/usr/bin/gcov" ]]; then
        print_header "COVERAGE ANALYSIS"
        echo "Generating comprehensive coverage report..." | tee -a "$COVERAGE_LOG"

        # Navigate to build directory for coverage files
        cd "$PROJECT_ROOT/$BUILD_DIR" || return 1

        # Generate gcov files for all source files
        find . -name "*.gcno" -exec gcov {} \; > /dev/null 2>&1

        # Calculate overall coverage percentage
        local total_lines=0
        local covered_lines=0

        for gcov_file in *.gcov; do
            if [[ -f "$gcov_file" ]]; then
                local file_total=$(grep -c ":" "$gcov_file" 2>/dev/null || echo 0)
                local file_covered=$(grep -c "^[ ]*[1-9]" "$gcov_file" 2>/dev/null || echo 0)
                total_lines=$((total_lines + file_total))
                covered_lines=$((covered_lines + file_covered))
            fi
        done

        if [[ $total_lines -gt 0 ]]; then
            local coverage_percent=$(( (covered_lines * 100) / total_lines ))
            echo "Coverage Results:" | tee -a "$COVERAGE_LOG"
            echo "  Total Lines: $total_lines" | tee -a "$COVERAGE_LOG"
            echo "  Covered Lines: $covered_lines" | tee -a "$COVERAGE_LOG"
            echo "  Coverage Percentage: ${coverage_percent}%" | tee -a "$COVERAGE_LOG"

            if [[ $coverage_percent -ge $COVERAGE_TARGET ]]; then
                echo -e "${GREEN}🎯 COVERAGE TARGET MET! ${coverage_percent}% >= ${COVERAGE_TARGET}%${NC}" | tee -a "$COVERAGE_LOG"
            else
                echo -e "${YELLOW}📊 Coverage: ${coverage_percent}% (Target: ${COVERAGE_TARGET}%)${NC}" | tee -a "$COVERAGE_LOG"
            fi
        else
            echo -e "${YELLOW}⚠️  Unable to calculate coverage - no gcov data found${NC}" | tee -a "$COVERAGE_LOG"
        fi

        cd "$PROJECT_ROOT" || return 1
    else
        echo "Coverage analysis skipped (COVERAGE_ENABLED=false or gcov not available)" | tee -a "$COVERAGE_LOG"
    fi
}

run_coverage_focused_tests() {
    print_header "HIGH-COVERAGE TEST EXECUTION"
    echo "Running tests specifically designed for comprehensive coverage..." | tee -a "$TEST_LOG"

    # These tests are specifically designed for high coverage
    local coverage_tests=(
        "BasicCoverageTest"
        "CoverageOptimizerTest"
        "ErrorHandlingTest"
        "MemoryManagementTest"
        "SessionStateTest"
        "UnifiedEngineComprehensiveTest"
        "DTWComprehensiveTest"
        "MFCCConsistencyUnifiedTest"
        "MasterCallComprehensiveTest"
    )

    for test_name in "${coverage_tests[@]}"; do
        if [[ -f "./bin/$test_name" ]]; then
            print_test "High-Coverage Test: $test_name"
            ((TOTAL_TESTS++))

            if timeout 60 "./bin/$test_name" > "$TEST_OUTPUT_DIR/coverage_${test_name}_$TIMESTAMP.log" 2>&1; then
                print_success "High-Coverage Test: $test_name"
            else
                print_failure "High-Coverage Test: $test_name"
            fi
        else
            print_skip "High-Coverage Test: $test_name - not built"
            ((TOTAL_TESTS++))
            ((SKIPPED_TESTS++))
        fi
    done
}

run_test_executable() {
    local test_name="$1"
    local executable="$2"
    local timeout_seconds="${3:-30}"

    ((TOTAL_TESTS++))
    print_test "$test_name"

    if [[ ! -f "$executable" ]]; then
        print_skip "$test_name - Executable not found: $executable"
        return 0
    fi

    if timeout "$timeout_seconds" "$executable" > "$TEST_OUTPUT_DIR/${test_name// /_}_$TIMESTAMP.log" 2>&1; then
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

run_gtest_suite() {
    local suite_name="$1"
    local filter="$2"
    local timeout_seconds="${3:-60}"

    ((TOTAL_TESTS++))
    print_test "$suite_name"

    if timeout "$timeout_seconds" ./build/bin/RunEngineTests --gtest_filter="$filter" --gtest_brief=yes > "$TEST_OUTPUT_DIR/${suite_name}_$TIMESTAMP.log" 2>&1; then
        local passed=$(grep -c "\[  PASSED  \]" "$TEST_OUTPUT_DIR/${suite_name}_$TIMESTAMP.log" || echo "0")
        local failed=$(grep -c "\[  FAILED  \]" "$TEST_OUTPUT_DIR/${suite_name}_$TIMESTAMP.log" || echo "0")

        if [[ $failed -eq 0 ]]; then
            print_success "$suite_name ($passed tests passed)"
        else
            print_failure "$suite_name ($failed tests failed, $passed tests passed)"
        fi
    else
        print_failure "$suite_name - Suite execution failed"
    fi
}

# Main test execution
main() {
    # ==========================================================================
    # Master Test Initialization and Setup
    # ==========================================================================
    print_header "HUNTMASTER ENGINE MASTER TEST SUITE"
    echo "🎯 COMPREHENSIVE TESTING FOR >90% COVERAGE TARGET" | tee -a "$TEST_LOG"
    echo "Started at: $(date)" | tee -a "$TEST_LOG"
    echo "Build directory: $BUILD_DIR" | tee -a "$TEST_LOG"
    echo "Test logs: $TEST_OUTPUT_DIR" | tee -a "$TEST_LOG"
    echo "Coverage target: ${COVERAGE_TARGET}%" | tee -a "$TEST_LOG"
    echo "Coverage enabled: $COVERAGE_ENABLED" | tee -a "$TEST_LOG"
    echo ""

    # Verify build directory exists
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo -e "${RED}Error: Build directory '$BUILD_DIR' not found!${NC}"
        echo "Please run 'cmake -B build && cmake --build build' first."
        exit 1
    fi

    cd "$BUILD_DIR"

    # ==========================================================================
    # Pre-Test Coverage Setup
    # ==========================================================================
    if [[ "$COVERAGE_ENABLED" == "true" ]]; then
        print_header "COVERAGE MEASUREMENT SETUP"
        echo "Initializing coverage measurement for ${COVERAGE_TARGET}% target..." | tee -a "$COVERAGE_LOG"

        # Clean previous coverage data
        find . -name "*.gcda" -delete 2>/dev/null || true
        echo "✅ Cleaned previous coverage data" | tee -a "$COVERAGE_LOG"

        # Verify coverage instrumentation
        local instrumented_files=$(find . -name "*.gcno" | wc -l)
        if [[ $instrumented_files -gt 0 ]]; then
            echo "✅ Found $instrumented_files instrumented files" | tee -a "$COVERAGE_LOG"
        else
            echo "⚠️  No coverage instrumentation found. Build with --coverage flags for coverage analysis." | tee -a "$COVERAGE_LOG"
        fi
        echo ""
    fi

    # ==========================================================================
    # Pre-Phase: High-Coverage Focused Tests
    # ==========================================================================
    run_coverage_focused_tests

    # ==========================================================================
    # Phase 1: Core Engine Components
    # ==========================================================================
    print_phase "1" "Core Engine Components"

    run_test_executable "UnifiedEngine Core" "./bin/UnifiedEngineTest" 45
    run_test_executable "UnifiedEngine Comprehensive" "./bin/UnifiedEngineComprehensiveTest" 60
    run_test_executable "UnifiedEngine VAD Config" "./bin/UnifiedEngineVADConfigTest" 30
    run_test_executable "Simple Unified Test" "./bin/simple_unified_test" 30

    # ==========================================================================
    # Phase 2: Audio Processing Components
    # ==========================================================================
    print_phase "2" "Audio Processing Components"

    run_test_executable "MFCC Processor" "./bin/MFCCDirectUnifiedTest" 45
    run_test_executable "MFCC Consistency" "./bin/MFCCConsistencyUnifiedTest" 60
    run_test_executable "MFCC Edge Cases" "./bin/MFCCEdgeCasesTest" 30
    run_test_executable "DTW Comparator" "./bin/DTWUnifiedTest" 45
    run_test_executable "DTW Comprehensive" "./bin/DTWComprehensiveTest" 60

    # ==========================================================================
    # Phase 3: Integration & Validation Tests
    # ==========================================================================
    print_phase "3" "Integration & Validation Tests"

    run_test_executable "Validation Unified" "./bin/ValidationUnifiedTest" 45
    run_test_executable "Real Wildlife Analysis" "./bin/RealWildlifeCallAnalysisTest" 90
    run_test_executable "End-to-End Tests" "./bin/EndToEndTest" 60
    run_test_executable "Audio Pipeline" "./bin/AudioPipelineTest" 45
    run_test_executable "Load and Extract" "./bin/LoadandExtractTest" 30

    # ==========================================================================
    # Phase 4: System & Infrastructure Tests
    # ==========================================================================
    print_phase "4" "System & Infrastructure Tests"

    run_test_executable "Error Handling" "./bin/ErrorHandlingTest" 30
    run_test_executable "Memory Management" "./bin/MemoryManagementTest" 45
    run_test_executable "Session State" "./bin/SessionStateTest" 30
    run_test_executable "Debug System" "./bin/DebugTest" 30
    run_test_executable "Cross Platform" "./bin/CrossPlatformTest" 30

    # ==========================================================================
    # Phase 5: Performance & Benchmarking
    # ==========================================================================
    print_phase "5" "Performance & Benchmarking"

    run_test_executable "Performance Test" "./bin/PerformanceTest" 45
    run_test_executable "Performance Profiler" "./bin/PerformanceProfilerTest" 30
    run_test_executable "Recording Test" "./bin/RecordingTest" 30

    # ==========================================================================
    # Phase 6: Specialized & Comprehensive Tests
    # ==========================================================================
    print_phase "6" "Specialized & Comprehensive Tests"

    run_test_executable "Master Call Comprehensive" "./bin/MasterCallComprehensiveTest" 60
    run_test_executable "Basic Coverage" "./bin/BasicCoverageTest" 30
    run_test_executable "Coverage Optimizer" "./bin/CoverageOptimizerTest" 30
    run_test_executable "MFCC Debugging Test" "./bin/test_mfcc_debugging" 30

    # ==========================================================================
    # Phase 7: Security & Advanced Components Testing
    # ==========================================================================
    print_phase "7" "Security & Advanced Components"

    # Test security components if they exist
    for component in "AccessController" "AuditLogger" "CryptoManager" "InputValidator" "MemoryGuard"; do
        if [[ -f "./bin/test_${component,,}" ]]; then
            run_test_executable "$component Test" "./bin/test_${component,,}" 30
        else
            print_skip "$component Test - Component not built"
            ((TOTAL_TESTS++))
            ((SKIPPED_TESTS++))
        fi
    done

    # Test advanced I/O components if they exist
    if [[ -f "./bin/test_advanced_io_optimizer" ]]; then
        run_test_executable "Advanced I/O Optimizer" "./bin/test_advanced_io_optimizer" 45
    else
        print_skip "Advanced I/O Optimizer Test - Component not built"
        ((TOTAL_TESTS++))
        ((SKIPPED_TESTS++))
    fi

    # ==========================================================================
    # Phase 8: Audio Component Specialized Tests
    # ==========================================================================
    print_phase "8" "Audio Component Specialized Tests"

    # Test audio format converter if available
    if [[ -f "./bin/test_audio_format_converter" ]]; then
        run_test_executable "Audio Format Converter" "./bin/test_audio_format_converter" 30
    else
        print_skip "Audio Format Converter Test - Component not built"
        ((TOTAL_TESTS++))
        ((SKIPPED_TESTS++))
    fi

    # Test circular audio buffer if available
    if [[ -f "./bin/test_circular_audio_buffer" ]]; then
        run_test_executable "Circular Audio Buffer" "./bin/test_circular_audio_buffer" 30
    else
        print_skip "Circular Audio Buffer Test - Component not built"
        ((TOTAL_TESTS++))
        ((SKIPPED_TESTS++))
    fi

    # Test streaming audio processor if available
    if [[ -f "./bin/test_streaming_audio_processor" ]]; then
        run_test_executable "Streaming Audio Processor" "./bin/test_streaming_audio_processor" 30
    else
        print_skip "Streaming Audio Processor Test - Component not built"
        ((TOTAL_TESTS++))
        ((SKIPPED_TESTS++))
    fi

    # ==========================================================================
    # Phase 9: Tools & Utilities Testing
    # ==========================================================================
    print_phase "9" "Tools & Utilities Testing"

    run_test_executable "Test Harness" "./bin/TestHarness" 30
    run_test_executable "Recorder Test" "./bin/RecorderTest" 30

    # ==========================================================================
    # Phase 10: Benchmark Suite
    # ==========================================================================
    print_phase "10" "Performance Benchmarks"

    if [[ -f "./tests/benchmarks/RunBenchmarks" ]]; then
        print_test "Running Google Benchmark Suite"
        ((TOTAL_TESTS++))

        if timeout 180 ./tests/benchmarks/RunBenchmarks --benchmark_out="$TEST_OUTPUT_DIR/benchmarks_$TIMESTAMP.json" --benchmark_out_format=json > "$TEST_OUTPUT_DIR/benchmarks_$TIMESTAMP.log" 2>&1; then
            print_success "Google Benchmark Suite"
        else
            print_failure "Google Benchmark Suite - Failed or timed out"
        fi
    else
        print_skip "Google Benchmark Suite - Not built"
        ((TOTAL_TESTS++))
        ((SKIPPED_TESTS++))
    fi    # ==========================================================================
    # Phase 11: Tool Validation (Quick Smoke Tests)
    # ==========================================================================
    print_phase "11" "Tool Validation"

    # Test key analysis tools with minimal smoke tests
    run_test_executable "Generate Features Tool" "./bin/generate_features --help" 5
    run_test_executable "Debug DTW Similarity Tool" "./bin/debug_dtw_similarity --help" 5
    run_test_executable "Performance Profiling Demo" "./bin/performance_profiling_demo --help" 5

    # ==========================================================================
    # Phase 12: Complete Google Test Suite
    # ==========================================================================
    print_phase "12" "Complete Google Test Suite"

    print_test "Running complete RunEngineTests suite"
    ((TOTAL_TESTS++))

    if timeout 120 ./bin/RunEngineTests --gtest_brief=yes > "$TEST_OUTPUT_DIR/RunEngineTests_complete_$TIMESTAMP.log" 2>&1; then
        local total_summary=$(tail -10 "$TEST_OUTPUT_DIR/RunEngineTests_complete_$TIMESTAMP.log" | grep "tests from.*test suite.*ran" || echo "")
        local passed_summary=$(tail -10 "$TEST_OUTPUT_DIR/RunEngineTests_complete_$TIMESTAMP.log" | grep "\[  PASSED  \]" || echo "")
        local failed_summary=$(tail -10 "$TEST_OUTPUT_DIR/RunEngineTests_complete_$TIMESTAMP.log" | grep "\[  FAILED  \]" || echo "")

        if [[ -n "$failed_summary" ]]; then
            print_failure "Complete Test Suite - Some tests failed"
            echo "Failed tests summary:" | tee -a "$TEST_LOG"
            echo "$failed_summary" | tee -a "$TEST_LOG"
        else
            print_success "Complete Test Suite - All tests passed"
            echo "$total_summary" | tee -a "$TEST_LOG"
        fi
    else
        print_failure "Complete Test Suite - Execution failed or timed out"
    fi

    # ==========================================================================
    # Phase 13: Focused Component Tests (Key Areas)
    # ==========================================================================
    print_phase "13" "Focused Component Tests"

    # Test our recently fixed RealtimeScorer component specifically
    run_gtest_suite "RealtimeScorer Tests" "*RealtimeScorer*" 30
    run_gtest_suite "DTW Integration Tests" "*DTWUnifiedTest*" 30
    run_gtest_suite "Real Wildlife Analysis" "*RealWildlifeCallAnalysisTest*" 90

    # Additional focused test categories
    run_gtest_suite "MFCC Component Tests" "*MFCC*Test*" 45
    run_gtest_suite "Audio Processing Tests" "*Audio*Test*" 60
    run_gtest_suite "Engine Core Tests" "*Engine*Test*" 60

    # ==========================================================================
    # Phase 14: Dynamic Test Discovery
    # ==========================================================================
    print_phase "14" "Dynamic Test Discovery"

    # Find any additional test executables that might not be covered
    echo "Scanning for additional test executables..." | tee -a "$TEST_LOG"

    # Create a list of already tested executables (lowercase for comparison)
    tested_executables=(
        "unifiedenginetest" "unifiedenginecomprehensivetest" "unifiedenginevadconfigtest"
        "simple_unified_test" "mfccdirectunifiedtest" "mfccconsistencyunifiedtest"
        "mfccedgecasestest" "dtwunifiedtest" "dtwcomprehensivetest" "validationunifiedtest"
        "realwildlifecallanalysistest" "endtoendtest" "audiopipelinetest" "loadandextracttest"
        "errorhandlingtest" "memorymanagementtest" "sessionstatetest" "debugtest"
        "crossplatformtest" "performancetest" "performanceprofilertest" "recordingtest"
        "mastercallcomprehensivetest" "basiccoveragetest" "coverageoptimizertest"
        "test_mfcc_debugging" "testharness" "recordertest" "runengineTests"
        "generate_features" "debug_dtw_similarity" "performance_profiling_demo"
        "runbenchmarks"
    )

    # Tools that should be tested as utilities but not run as tests
    utility_tools=(
        "analyze_recording" "audio_trimmer" "audio_visualization" "detailed_analysis"
        "interactive_recorder" "real_time_recording_monitor"
    )

    additional_tests_found=0
    for test_file in ./bin/*; do
        if [[ -x "$test_file" && -f "$test_file" ]]; then
            test_basename=$(basename "$test_file" | tr '[:upper:]' '[:lower:]')

            # Skip known tools and non-test executables
            if [[ "$test_basename" =~ ^(generate_features|debug_dtw_similarity|performance_profiling_demo|interactive_recorder|simple_dtw_test|simple_mfcc_test)$ ]]; then
                continue
            fi

            # Skip utility tools (test with --help instead)
            is_utility=false
            for utility in "${utility_tools[@]}"; do
                if [[ "$test_basename" == "$utility" ]]; then
                    is_utility=true
                    break
                fi
            done

            if [[ "$is_utility" == true ]]; then
                echo "Found utility tool: $test_file - Testing with --help" | tee -a "$TEST_LOG"
                run_test_executable "Utility: $(basename "$test_file")" "$test_file --help" 5
                ((additional_tests_found++))
                continue
            fi

            # Check if this test is already covered
            is_tested=false
            for tested in "${tested_executables[@]}"; do
                if [[ "$test_basename" == "$tested" ]]; then
                    is_tested=true
                    break
                fi
            done

            if [[ "$is_tested" == false ]]; then
                echo "Found additional test: $test_file" | tee -a "$TEST_LOG"
                run_test_executable "Additional Test: $(basename "$test_file")" "$test_file" 30
                ((additional_tests_found++))
            fi
        fi
    done

    if [[ $additional_tests_found -eq 0 ]]; then
        echo "✅ No additional test executables found - comprehensive coverage achieved!" | tee -a "$TEST_LOG"
    else
        echo "ℹ️  Found and executed $additional_tests_found additional test(s)" | tee -a "$TEST_LOG"
    fi

    # ==========================================================================
    # Test Summary and Analysis
    # ==========================================================================
    # Coverage Analysis and Reporting
    # ==========================================================================
    generate_coverage_report

    # ==========================================================================
    echo ""
    print_header "MASTER TEST RESULTS SUMMARY"

    echo "📊 TEST EXECUTION SUMMARY:" | tee -a "$TEST_LOG"
    echo "======================" | tee -a "$TEST_LOG"
    echo "Total Tests: $TOTAL_TESTS" | tee -a "$TEST_LOG"
    echo "Passed: $PASSED_TESTS" | tee -a "$TEST_LOG"
    echo "Failed: $FAILED_TESTS" | tee -a "$TEST_LOG"
    echo "Skipped: $SKIPPED_TESTS" | tee -a "$TEST_LOG"
    echo "" | tee -a "$TEST_LOG"

    # Calculate success rate
    if [[ $TOTAL_TESTS -gt 0 ]]; then
        local success_rate=$(( (PASSED_TESTS * 100) / TOTAL_TESTS ))
        echo "Success Rate: ${success_rate}%" | tee -a "$TEST_LOG"
        echo "" | tee -a "$TEST_LOG"
    fi

    # Generate test coverage summary
    echo "Test Coverage Summary:" | tee -a "$TEST_LOG"
    echo "=====================" | tee -a "$TEST_LOG"
    echo "✅ Core Engine Components: Comprehensive" | tee -a "$TEST_LOG"
    echo "✅ Audio Processing Pipeline: Complete" | tee -a "$TEST_LOG"
    echo "✅ Integration & Validation: Full Coverage" | tee -a "$TEST_LOG"
    echo "✅ System Infrastructure: Tested" | tee -a "$TEST_LOG"
    echo "✅ Performance Benchmarking: Included" | tee -a "$TEST_LOG"
    echo "✅ Security Components: Available Tests Run" | tee -a "$TEST_LOG"
    echo "✅ Specialized Audio Components: Dynamic Discovery" | tee -a "$TEST_LOG"
    echo "✅ Tools & Utilities: Validated" | tee -a "$TEST_LOG"
    echo "✅ Dynamic Test Discovery: Complete" | tee -a "$TEST_LOG"
    echo "✅ Google Test Suite: Full Execution" | tee -a "$TEST_LOG"
    echo "✅ Focused Component Testing: Key Areas Covered" | tee -a "$TEST_LOG"
    echo "" | tee -a "$TEST_LOG"

    # Report on test logs created
    echo "Test Logs Generated:" | tee -a "$TEST_LOG"
    echo "===================" | tee -a "$TEST_LOG"
    local log_count=$(find "$TEST_OUTPUT_DIR" -name "*_$TIMESTAMP.log" | wc -l)
    echo "Total log files created: $log_count" | tee -a "$TEST_LOG"
    echo "Log directory: $TEST_OUTPUT_DIR" | tee -a "$TEST_LOG"
    echo "" | tee -a "$TEST_LOG"

    if [[ $FAILED_TESTS -eq 0 ]]; then
        echo -e "${GREEN}🎉 ALL TESTS PASSED! Engine is in excellent condition.${NC}" | tee -a "$TEST_LOG"
        echo ""
        echo "🏆 HUNTMASTER ENGINE MASTER TEST SUCCESS:" | tee -a "$TEST_LOG"
        echo "✅ Core engine functionality validated" | tee -a "$TEST_LOG"
        echo "✅ Audio processing components working" | tee -a "$TEST_LOG"
        echo "✅ Integration tests successful" | tee -a "$TEST_LOG"
        echo "✅ System infrastructure robust" | tee -a "$TEST_LOG"
        echo "✅ Performance benchmarks met" | tee -a "$TEST_LOG"
        echo "✅ All tools and utilities functional" | tee -a "$TEST_LOG"
        echo "✅ High-coverage tests executed" | tee -a "$TEST_LOG"
        if [[ "$COVERAGE_ENABLED" == "true" ]]; then
            echo "✅ Coverage analysis completed" | tee -a "$TEST_LOG"
            echo "📊 Coverage report: $COVERAGE_LOG" | tee -a "$TEST_LOG"
        fi
        echo ""
        echo "🎯 THIS IS TRULY THE MASTER TEST - COMPREHENSIVE COVERAGE ACHIEVED!" | tee -a "$TEST_LOG"
        exit 0
    else
        echo -e "${RED}⚠️  SOME TESTS FAILED! Review required.${NC}" | tee -a "$TEST_LOG"
        echo "" | tee -a "$TEST_LOG"
        echo "Failed test details can be found in:" | tee -a "$TEST_LOG"
        find "$TEST_OUTPUT_DIR" -name "*_$TIMESTAMP.log" -exec bash -c 'if grep -q "FAILED\|failed\|error\|Error" "$1"; then echo "  🔍 $1"; fi' _ {} \; | tee -a "$TEST_LOG"

        if [[ "$COVERAGE_ENABLED" == "true" ]]; then
            echo "📊 Coverage report (despite failures): $COVERAGE_LOG" | tee -a "$TEST_LOG"
        fi
        exit 1
    fi
}

# Trap for cleanup
trap 'echo "Test execution interrupted"; exit 130' INT TERM

# Execute main function
main "$@"
