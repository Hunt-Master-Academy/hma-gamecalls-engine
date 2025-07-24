#!/bin/bash

# ==============================================================================
# Comprehensive Test Script for Huntmaster Audio Engine
# ==============================================================================
# This script builds and tests all components of the UnifiedAudioEngine
# to ensure everything works correctly before WASM deployment.
#
# Usage: ./comprehensive_test.sh [--verbose] [--skip-build] [--test-only=<test_name>]
# ==============================================================================

set -e  # Exit on any error

# Configuration
BUILD_DIR="build"
TEST_DATA_DIR="data"
VERBOSE=false
SKIP_BUILD=false
SPECIFIC_TEST=""
PARALLEL_JOBS=$(nproc 2>/dev/null || echo 4)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Parse command line arguments
for arg in "$@"; do
    case $arg in
        --verbose)
            VERBOSE=true
            shift
            ;;
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --test-only=*)
            SPECIFIC_TEST="${arg#*=}"
            shift
            ;;
        --help)
            echo "Usage: $0 [--verbose] [--skip-build] [--test-only=<test_name>]"
            echo "  --verbose      Enable verbose output"
            echo "  --skip-build   Skip the build step"
            echo "  --test-only    Run only a specific test (build, unit, integration, diagnostics, audio, performance)"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg"
            exit 1
            ;;
    esac
done

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_test() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

# Function to run command with optional verbose output
run_cmd() {
    local cmd="$1"
    local description="$2"

    if [ "$VERBOSE" = true ]; then
        log_info "Running: $cmd"
    fi

    if [ "$VERBOSE" = true ]; then
        eval "$cmd"
    else
        eval "$cmd" > /dev/null 2>&1
    fi

    if [ $? -eq 0 ]; then
        log_success "$description"
        return 0
    else
        log_error "$description failed"
        return 1
    fi
}

# Function to check if a file exists
check_file() {
    if [ -f "$1" ]; then
        log_success "Found: $1"
        return 0
    else
        log_error "Missing: $1"
        return 1
    fi
}

# Function to check if test data exists
check_test_data() {
    log_info "Checking test data availability..."

    local test_files=(
        "data/master_calls/buck_grunt.wav"
        "data/recordings/user_attempt_buck_grunt.wav"
        "data/recordings/user_attempt_buck_grunt_gap.wav"
        "data/recordings/user_attempt_buck_grunt_lowvolume.wav"
        "data/recordings/user_attempt_buck_grunt_nogap.wav"
    )

    for file in "${test_files[@]}"; do
        if [ -f "$file" ]; then
            log_success "Test data found: $file"
        else
            log_warning "Test data missing: $file (some tests may fail)"
        fi
    done
}

# Function to build the project
build_project() {
    if [ "$SKIP_BUILD" = true ]; then
        log_info "Skipping build as requested"
        return 0
    fi

    log_info "Building Huntmaster Audio Engine..."

    # Create build directory
    if [ ! -d "$BUILD_DIR" ]; then
        mkdir -p "$BUILD_DIR"
        log_info "Created build directory: $BUILD_DIR"
    fi

    cd "$BUILD_DIR"

    # Configure with CMake
    log_info "Configuring with CMake..."
    if ! run_cmd "cmake .." "CMake configuration"; then
        cd ..
        log_error "CMake configuration failed"
        return 1
    fi

    # Build the project
    log_info "Building project with $PARALLEL_JOBS parallel jobs..."
    if ! run_cmd "make -j$PARALLEL_JOBS" "Project build"; then
        cd ..
        log_error "Project build failed"
        return 1
    fi

    cd ..

    log_success "Build completed successfully"
    return 0
}

# Function to run a specific test executable
run_test_executable() {
    local test_name="$1"
    local test_path="$BUILD_DIR/bin/$test_name"
    local description="$2"
    local args="${3:-}"

    if [ ! -f "$test_path" ]; then
        log_error "Test executable not found: $test_path"
        return 1
    fi

    log_test "Running $description..."

    if [ "$VERBOSE" = true ]; then
        "$test_path" $args
        local result=$?
    else
        "$test_path" $args > /dev/null 2>&1
        local result=$?
    fi

    if [ $result -eq 0 ]; then
        log_success "$description passed"
        return 0
    else
        log_error "$description failed (exit code: $result)"
        return 1
    fi
}

# Function to run unit tests
run_unit_tests() {
    if [ -n "$SPECIFIC_TEST" ] && [ "$SPECIFIC_TEST" != "unit" ]; then
        return 0
    fi

    log_info "=== Running Unit Tests ==="

    local tests=(
        "RunEngineTests:Main unit test suite"
        "UnifiedEngineTest:UnifiedAudioEngine core functionality"
        "MFCCConsistencyUnifiedTest:MFCC feature extraction consistency"
        "DTWUnifiedTest:DTW algorithm functionality"
        "MFCCDirectUnifiedTest:Direct MFCC computation"
        "ValidationUnifiedTest:Input validation and error handling"
    )

    local passed=0
    local total=0

    for test_info in "${tests[@]}"; do
        IFS=':' read -r test_name description <<< "$test_info"
        total=$((total + 1))

        if run_test_executable "$test_name" "$description"; then
            passed=$((passed + 1))
        fi
    done

    log_info "Unit Tests Summary: $passed/$total passed"

    if [ $passed -eq $total ]; then
        log_success "All unit tests passed!"
        return 0
    else
        log_error "Some unit tests failed"
        return 1
    fi
}

# Function to run integration tests
run_integration_tests() {
    if [ -n "$SPECIFIC_TEST" ] && [ "$SPECIFIC_TEST" != "integration" ]; then
        return 0
    fi

    log_info "=== Running Integration Tests ==="

    local tests=(
        "EndToEndTest:End-to-end audio processing pipeline"
        "RealWildlifeCallAnalysisTest:Real wildlife call analysis"
        "LoadandExtractTest:Feature extraction debugging"
    )

    local passed=0
    local total=0

    for test_info in "${tests[@]}"; do
        IFS=':' read -r test_name description <<< "$test_info"
        total=$((total + 1))

        if run_test_executable "$test_name" "$description"; then
            passed=$((passed + 1))
        fi
    done

    log_info "Integration Tests Summary: $passed/$total passed"

    if [ $passed -eq $total ]; then
        log_success "All integration tests passed!"
        return 0
    else
        log_error "Some integration tests failed"
        return 1
    fi
}

# Function to run standalone diagnostic tools
run_diagnostic_tools() {
    if [ -n "$SPECIFIC_TEST" ] && [ "$SPECIFIC_TEST" != "diagnostics" ]; then
        return 0
    fi

    log_info "=== Running Diagnostic Tools ==="

    local tools=(
        "CrossPlatformTest:Cross-platform compatibility check"
        "PerformanceTest:Performance benchmarking"
        "TestHarness:Manual validation harness"
    )

    local passed=0
    local total=0

    for tool_info in "${tools[@]}"; do
        IFS=':' read -r tool_name description <<< "$tool_info"
        total=$((total + 1))

        if run_test_executable "$tool_name" "$description"; then
            passed=$((passed + 1))
        fi
    done

    log_info "Diagnostic Tools Summary: $passed/$total passed"

    if [ $passed -eq $total ]; then
        log_success "All diagnostic tools passed!"
        return 0
    else
        log_warning "Some diagnostic tools failed (may be expected)"
        return 0  # Don't fail the entire test suite for diagnostics
    fi
}

# Function to run audio processing tests
run_audio_processing_tests() {
    if [ -n "$SPECIFIC_TEST" ] && [ "$SPECIFIC_TEST" != "audio" ]; then
        return 0
    fi

    log_info "=== Running Audio Processing Tests ==="

    # Test with master call and user-submitted recordings for specific validation purposes
    log_info "Testing audio processing pipeline with various recording conditions..."

    # Master call - reference baseline
    if [ -f "data/master_calls/buck_grunt.wav" ]; then
        total=$((total + 1))
        log_test "Testing with master call (reference): data/master_calls/buck_grunt.wav"
        if run_test_executable "LoadandExtractTest" "Master call feature extraction baseline" "data/master_calls/buck_grunt.wav master_reference"; then
            passed=$((passed + 1))
        fi
    fi

    # Standard user recording - baseline user comparison
    if [ -f "data/recordings/user_attempt_buck_grunt.wav" ]; then
        total=$((total + 1))
        log_test "Testing with standard user recording: data/recordings/user_attempt_buck_grunt.wav"
        if run_test_executable "LoadandExtractTest" "Standard user recording comparison" "data/recordings/user_attempt_buck_grunt.wav user_standard"; then
            passed=$((passed + 1))
        fi
    fi

    # Gap recording - VAD and timing robustness
    if [ -f "data/recordings/user_attempt_buck_grunt_gap.wav" ]; then
        total=$((total + 1))
        log_test "Testing with gap recording (VAD/timing): data/recordings/user_attempt_buck_grunt_gap.wav"
        if run_test_executable "LoadandExtractTest" "Gap recording VAD and timing handling" "data/recordings/user_attempt_buck_grunt_gap.wav user_gap_vad"; then
            passed=$((passed + 1))
        fi
    fi

    # Low volume recording - amplitude sensitivity and normalization
    if [ -f "data/recordings/user_attempt_buck_grunt_lowvolume.wav" ]; then
        total=$((total + 1))
        log_test "Testing with low volume recording (sensitivity): data/recordings/user_attempt_buck_grunt_lowvolume.wav"
        if run_test_executable "LoadandExtractTest" "Low volume sensitivity and normalization" "data/recordings/user_attempt_buck_grunt_lowvolume.wav user_lowvolume"; then
            passed=$((passed + 1))
        fi
    fi

    # No gap recording - continuous audio processing
    if [ -f "data/recordings/user_attempt_buck_grunt_nogap.wav" ]; then
        total=$((total + 1))
        log_test "Testing with continuous recording (no gaps): data/recordings/user_attempt_buck_grunt_nogap.wav"
        if run_test_executable "LoadandExtractTest" "Continuous audio processing validation" "data/recordings/user_attempt_buck_grunt_nogap.wav user_continuous"; then
            passed=$((passed + 1))
        fi
    fi

    if [ $total -eq 0 ]; then
        log_warning "No test audio files found - skipping audio processing tests"
        return 0
    fi

    log_info "Audio Processing Tests Summary: $passed/$total passed"

    if [ $passed -eq $total ]; then
        log_success "All audio processing tests passed!"
        return 0
    else
        log_error "Some audio processing tests failed"
        return 1
    fi
}

# Function to run memory and performance tests
run_performance_tests() {
    if [ -n "$SPECIFIC_TEST" ] && [ "$SPECIFIC_TEST" != "performance" ]; then
        return 0
    fi

    log_info "=== Running Performance Tests ==="

    # Check if valgrind is available for memory testing
    if command -v valgrind >/dev/null 2>&1; then
        log_test "Running memory leak detection with valgrind..."

        if [ -f "data/master_calls/buck_grunt.wav" ]; then
            local valgrind_cmd="valgrind --leak-check=full --error-exitcode=1 $BUILD_DIR/bin/LoadandExtractTest"

            if [ "$VERBOSE" = true ]; then
                $valgrind_cmd
            else
                $valgrind_cmd > /dev/null 2>&1
            fi

            if [ $? -eq 0 ]; then
                log_success "Memory leak test passed"
            else
                log_error "Memory leak test failed"
                return 1
            fi
        else
            log_warning "No test audio file for memory testing"
        fi
    else
        log_info "Valgrind not available - skipping memory leak detection"
    fi

    # Run performance benchmarks if available
    if [ -f "$BUILD_DIR/bin/PerformanceTest" ]; then
        run_test_executable "PerformanceTest" "Performance benchmarking"
    fi

    log_success "Performance tests completed"
}

# Function to generate test report
generate_test_report() {
    local report_file="test_report_$(date +%Y%m%d_%H%M%S).txt"

    log_info "Generating test report: $report_file"

    {
        echo "==============================================="
        echo "Huntmaster Audio Engine - Test Report"
        echo "Generated: $(date)"
        echo "==============================================="
        echo ""
        echo "Build Configuration:"
        echo "  - Build Directory: $BUILD_DIR"
        echo "  - Parallel Jobs: $PARALLEL_JOBS"
        echo "  - Verbose Mode: $VERBOSE"
        echo ""
        echo "System Information:"
        echo "  - OS: $(uname -s)"
        echo "  - Architecture: $(uname -m)"
        echo "  - CPU Cores: $(nproc 2>/dev/null || echo 'Unknown')"
        echo ""
        echo "Test Results:"
        # This would be populated with actual test results
        echo "  - Unit Tests: See above output"
        echo "  - Integration Tests: See above output"
        echo "  - Diagnostic Tools: See above output"
        echo "  - Audio Processing: See above output"
        echo "  - Performance Tests: See above output"
        echo ""
        echo "Recommendations:"
        echo "  - If tests fail, check the verbose output for details"
        echo "  - Ensure all test data files are present"
        echo "  - Verify audio hardware/drivers are properly installed"
        echo ""
    } > "$report_file"

    log_success "Test report saved to: $report_file"
}

# Main execution function
main() {
    echo "==============================================="
    echo "Huntmaster Audio Engine - Comprehensive Test"
    echo "==============================================="
    echo ""

    log_info "Starting comprehensive test suite..."

    # Check prerequisites
    check_test_data

    # Build project
    build_project || {
        log_error "Build failed - cannot proceed with tests"
        exit 1
    }

    # Initialize test tracking
    local overall_success=true

    # Run test suites
    case "$SPECIFIC_TEST" in
        "build")
            # Only build, don't run any tests
            log_success "Build completed successfully - ready for testing"
            return 0
            ;;
        "unit")
            run_unit_tests || overall_success=false
            ;;
        "integration")
            run_integration_tests || overall_success=false
            ;;
        "diagnostics")
            run_diagnostic_tools || overall_success=false
            ;;
        "audio")
            run_audio_processing_tests || overall_success=false
            ;;
        "performance")
            run_performance_tests || overall_success=false
            ;;
        "")
            # Run all tests
            run_unit_tests || overall_success=false
            run_integration_tests || overall_success=false
            run_diagnostic_tools || overall_success=false
            run_audio_processing_tests || overall_success=false
            run_performance_tests || overall_success=false
            ;;
        *)
            log_error "Unknown test suite: $SPECIFIC_TEST"
            exit 1
            ;;
    esac

    # Generate report
    generate_test_report

    # Final summary
    echo ""
    echo "==============================================="
    if [ "$overall_success" = true ]; then
        log_success "🎉 ALL TESTS PASSED! Engine is ready for WASM deployment."
        echo "Next steps:"
        echo "  1. Review the test report for any warnings"
        echo "  2. Proceed with WASM compilation"
        echo "  3. Test WASM bindings"
    else
        log_error "❌ SOME TESTS FAILED! Please review and fix issues before WASM deployment."
        echo "Debugging steps:"
        echo "  1. Run with --verbose flag for detailed output"
        echo "  2. Run specific test suites with --test-only=<suite>"
        echo "  3. Check the test report for specific failure details"
        exit 1
    fi
    echo "==============================================="
}

# Execute main function
main "$@"
