#!/bin/bash
# Docker Coverage Test Script - Comprehensive coverage analysis and validation
# Designed to achieve >90% test coverage with reproducible results

set -euo pipefail

# Configuration
COVERAGE_TARGET=${COVERAGE_TARGET:-90}
TEST_TIMEOUT=${TEST_TIMEOUT:-180}
BUILD_DIR="/huntmaster-engine/build"
COVERAGE_DIR="/huntmaster-engine/coverage_reports"
LOG_DIR="/huntmaster-engine/test_logs"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')] $1${NC}"
}

warn() {
    echo -e "${YELLOW}[WARNING] $1${NC}"
}

error() {
    echo -e "${RED}[ERROR] $1${NC}"
}

success() {
    echo -e "${GREEN}[SUCCESS] $1${NC}"
}

# Initialize coverage analysis
initialize_coverage() {
    log "Initializing coverage analysis environment..."
    
    # Create necessary directories
    mkdir -p "$COVERAGE_DIR"/{html,xml,raw}
    mkdir -p "$LOG_DIR"
    
    # Reset coverage counters
    find "$BUILD_DIR" -name "*.gcda" -delete 2>/dev/null || true
    
    # Initialize lcov baseline
    lcov --directory "$BUILD_DIR" --zerocounters 2>/dev/null || true
    lcov --directory "$BUILD_DIR" --capture --initial --output-file "$COVERAGE_DIR/raw/baseline.info" 2>/dev/null || true
    
    success "Coverage environment initialized"
}

# Execute comprehensive test suite
run_comprehensive_tests() {
    log "Executing comprehensive test suite..."
    
    local test_executable="$BUILD_DIR/bin/RunEngineTests"
    local exit_code=0
    
    if [[ ! -f "$test_executable" ]]; then
        error "Test executable not found: $test_executable"
        return 1
    fi
    
    # Run tests with timeout protection
    log "Running all tests with timeout protection..."
    if timeout "$TEST_TIMEOUT" "$test_executable" --gtest_brief=yes --gtest_output=xml:"$LOG_DIR/test_results.xml" 2>&1 | tee "$LOG_DIR/test_execution.log"; then
        success "Test suite completed successfully"
    else
        exit_code=$?
        if [[ $exit_code -eq 124 ]]; then
            error "Tests timed out after ${TEST_TIMEOUT} seconds"
        else
            error "Tests failed with exit code: $exit_code"
        fi
        warn "Continuing with coverage analysis of executed tests..."
    fi
    
    return $exit_code
}

# Generate coverage data
generate_coverage_data() {
    log "Generating coverage data..."
    
    # Capture coverage data
    lcov --directory "$BUILD_DIR" --capture --output-file "$COVERAGE_DIR/raw/test_coverage.info" 2>/dev/null || {
        warn "lcov capture failed, trying alternative method..."
        find "$BUILD_DIR" -name "*.gcda" -exec gcov {} \; > "$COVERAGE_DIR/raw/gcov_output.log" 2>&1 || true
    }
    
    # Filter out system and external libraries
    if [[ -f "$COVERAGE_DIR/raw/test_coverage.info" ]]; then
        lcov --remove "$COVERAGE_DIR/raw/test_coverage.info" \
            '/usr/*' \
            '*/tests/*' \
            '*/build/*' \
            '*/libs/*' \
            '*/archive/*' \
            --output-file "$COVERAGE_DIR/raw/filtered_coverage.info" 2>/dev/null || true
    fi
    
    success "Coverage data generated"
}

# Generate HTML coverage report
generate_html_report() {
    log "Generating HTML coverage report..."
    
    if [[ -f "$COVERAGE_DIR/raw/filtered_coverage.info" ]]; then
        genhtml "$COVERAGE_DIR/raw/filtered_coverage.info" \
            --output-directory "$COVERAGE_DIR/html" \
            --title "Huntmaster Engine Coverage Report" \
            --show-details \
            --legend \
            --frames \
            --sort \
            --demangle-cpp 2>/dev/null || {
            warn "genhtml failed, generating alternative report..."
        }
    fi
    
    # Alternative: gcovr HTML report
    gcovr --root /huntmaster-engine \
        --exclude-directories tests \
        --exclude-directories build \
        --exclude-directories libs \
        --exclude-directories archive \
        --html --html-details \
        --output "$COVERAGE_DIR/html/gcovr_report.html" 2>/dev/null || {
        warn "gcovr HTML report generation failed"
    }
    
    success "HTML report generated in $COVERAGE_DIR/html/"
}

# Generate XML coverage report for CI/CD
generate_xml_report() {
    log "Generating XML coverage report..."
    
    gcovr --root /huntmaster-engine \
        --exclude-directories tests \
        --exclude-directories build \
        --exclude-directories libs \
        --exclude-directories archive \
        --xml --output "$COVERAGE_DIR/xml/coverage.xml" 2>/dev/null || {
        warn "XML coverage report generation failed"
    }
    
    success "XML report generated in $COVERAGE_DIR/xml/"
}

# Calculate and validate coverage percentage
validate_coverage() {
    log "Validating coverage against target ($COVERAGE_TARGET%)..."
    
    local coverage_percent=0
    
    # Try multiple methods to get coverage percentage
    if command -v gcovr >/dev/null 2>&1; then
        coverage_percent=$(gcovr --root /huntmaster-engine \
            --exclude-directories tests \
            --exclude-directories build \
            --exclude-directories libs \
            --exclude-directories archive \
            2>/dev/null | grep "TOTAL" | awk '{print $4}' | sed 's/%//' || echo "0")
    fi
    
    # Fallback: parse lcov summary
    if [[ "$coverage_percent" == "0" ]] && [[ -f "$COVERAGE_DIR/raw/filtered_coverage.info" ]]; then
        coverage_percent=$(lcov --summary "$COVERAGE_DIR/raw/filtered_coverage.info" 2>/dev/null | \
            grep "lines" | awk '{print $2}' | sed 's/%//' || echo "0")
    fi
    
    # Ensure we have a numeric value
    coverage_percent=${coverage_percent:-0}
    
    log "Current coverage: ${coverage_percent}%"
    log "Target coverage: ${COVERAGE_TARGET}%"
    
    # Generate coverage summary
    cat > "$COVERAGE_DIR/coverage_summary.txt" << EOF
Huntmaster Engine Coverage Analysis Report
==========================================
Date: $(date)
Target Coverage: ${COVERAGE_TARGET}%
Achieved Coverage: ${coverage_percent}%
Status: $(if (( $(echo "$coverage_percent >= $COVERAGE_TARGET" | bc -l 2>/dev/null || echo "0") )); then echo "PASSED"; else echo "FAILED"; fi)

Test Execution Summary:
- Tests executed with timeout protection (${TEST_TIMEOUT}s)
- Coverage data captured and filtered
- HTML and XML reports generated
- Results available in: $COVERAGE_DIR

Next Steps:
$(if (( $(echo "$coverage_percent >= $COVERAGE_TARGET" | bc -l 2>/dev/null || echo "0") )); then
    echo "✅ Coverage target achieved! Focus on maintaining quality."
else
    echo "❌ Coverage below target. Priority areas for improvement:"
    echo "   1. Add tests for uncovered core components"
    echo "   2. Focus on error path testing"
    echo "   3. Improve integration test coverage"
    echo "   4. Add edge case testing"
fi)
EOF
    
    # Validation result
    if (( $(echo "$coverage_percent >= $COVERAGE_TARGET" | bc -l 2>/dev/null || echo "0") )); then
        success "Coverage target achieved: ${coverage_percent}% >= ${COVERAGE_TARGET}%"
        return 0
    else
        error "Coverage below target: ${coverage_percent}% < ${COVERAGE_TARGET}%"
        return 1
    fi
}

# Generate detailed analysis
generate_analysis() {
    log "Generating detailed coverage analysis..."
    
    # Create detailed breakdown
    cat > "$COVERAGE_DIR/detailed_analysis.txt" << EOF
Huntmaster Engine Detailed Coverage Analysis
============================================

Coverage by Component:
EOF
    
    # Add component-wise coverage if available
    if command -v gcovr >/dev/null 2>&1; then
        echo "" >> "$COVERAGE_DIR/detailed_analysis.txt"
        echo "Detailed Coverage Breakdown:" >> "$COVERAGE_DIR/detailed_analysis.txt"
        echo "============================" >> "$COVERAGE_DIR/detailed_analysis.txt"
        gcovr --root /huntmaster-engine \
            --exclude-directories tests \
            --exclude-directories build \
            --exclude-directories libs \
            --exclude-directories archive \
            2>/dev/null >> "$COVERAGE_DIR/detailed_analysis.txt" || true
    fi
    
    success "Detailed analysis generated"
}

# Main execution flow
main() {
    log "Starting Docker coverage test script..."
    log "Target: >$COVERAGE_TARGET% coverage in $TEST_TIMEOUT seconds"
    
    # Initialize
    initialize_coverage
    
    # Execute tests
    local test_exit_code=0
    run_comprehensive_tests || test_exit_code=$?
    
    # Generate coverage reports
    generate_coverage_data
    generate_html_report
    generate_xml_report
    generate_analysis
    
    # Validate results
    local coverage_valid=0
    validate_coverage || coverage_valid=$?
    
    # Final summary
    log "Coverage analysis complete!"
    log "Reports available in: $COVERAGE_DIR"
    cat "$COVERAGE_DIR/coverage_summary.txt"
    
    # Exit with appropriate code
    if [[ $test_exit_code -ne 0 ]]; then
        error "Test execution failed (exit code: $test_exit_code)"
        exit $test_exit_code
    elif [[ $coverage_valid -ne 0 ]]; then
        error "Coverage validation failed"
        exit 1
    else
        success "All tests passed and coverage target achieved!"
        exit 0
    fi
}

# Execute main function
main "$@"
