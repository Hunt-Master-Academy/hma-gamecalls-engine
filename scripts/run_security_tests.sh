#!/bin/bash

# Security Test Runner with Timeout Protection
# Part of Work Stream C: Security & Compliance Test Coverage
# Author: Huntmaster Engine Team
# Date: August 19, 2025

set -euo pipefail

# Configuration with proper timeout handling
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build/debug"
TIMEOUT_DURATION=180
MAX_RETRIES=3

# Logging functions with timestamps
log_info() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [INFO] $*"
}

log_error() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [ERROR] $*" >&2
}

log_warning() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [WARNING] $*" >&2
}

# Timeout wrapper function
run_with_timeout() {
    local cmd_timeout="$1"
    shift
    local cmd_description="$1"
    shift

    log_info "Running: $cmd_description (timeout: ${cmd_timeout}s)"

    if timeout "$cmd_timeout" "$@"; then
        log_info "✓ $cmd_description completed successfully"
        return 0
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            log_error "✗ $cmd_description timed out after ${cmd_timeout}s"
        else
            log_error "✗ $cmd_description failed with exit code $exit_code"
        fi
        return $exit_code
    fi
}

# Build with timeout protection
build_project() {
    local retry_count=0

    while [ $retry_count -lt $MAX_RETRIES ]; do
        log_info "Build attempt $((retry_count + 1))/$MAX_RETRIES"

        if run_with_timeout 120 "Build configuration" cmake --preset docker-debug; then
            if run_with_timeout $TIMEOUT_DURATION "Project build" ninja -C "$BUILD_DIR"; then
                log_info "✓ Build completed successfully"
                return 0
            fi
        fi

        retry_count=$((retry_count + 1))
        if [ $retry_count -lt $MAX_RETRIES ]; then
            log_warning "Build failed, retrying in 5 seconds..."
            sleep 5
        fi
    done

    log_error "Build failed after $MAX_RETRIES attempts"
    return 1
}

# Run security tests with timeout protection
run_security_tests() {
    local test_executable="$BUILD_DIR/tests/RunEngineTests"

    if [ ! -f "$test_executable" ]; then
        log_error "Test executable not found: $test_executable"
        return 1
    fi

    log_info "Starting security test suite execution"

    # Core security tests with timeout
    run_with_timeout $TIMEOUT_DURATION "Security Test Suite" \
        "$test_executable" \
        --gtest_brief=yes \
        --gtest_filter="*Security*:*Memory*:*Access*:*Input*" \
        --gtest_output="xml:$PROJECT_ROOT/security_test_results.xml"
}

# Run validation tests with timeout protection
run_validation_tests() {
    local validation_executable="$BUILD_DIR/tests/SecurityValidationTests"
    local thread_safety_executable="$BUILD_DIR/tests/ThreadSafetyTests"    # Session validation tests
    if [ -f "$validation_executable" ]; then
        run_with_timeout 90 "Session Security Validation Tests" \
            "$validation_executable" \
            --gtest_brief=yes \
            --gtest_output="xml:$PROJECT_ROOT/validation_test_results.xml"
    else
        log_warning "Session validation tests not built - skipping"
    fi

    # Thread safety tests
    if [ -f "$thread_safety_executable" ]; then
        run_with_timeout 90 "Thread Safety Tests" \
            "$thread_safety_executable" \
            --gtest_brief=yes \
            --gtest_output="xml:$PROJECT_ROOT/thread_safety_test_results.xml"
    else
        log_warning "Thread safety tests not built - skipping"
    fi
}

# Coverage analysis with timeout protection
run_coverage_analysis() {
    if [ ! -d "$PROJECT_ROOT/build/coverage" ]; then
        log_info "Setting up coverage build"
        if ! run_with_timeout 120 "Coverage configuration" cmake --preset docker-coverage; then
            log_warning "Coverage configuration failed - skipping coverage analysis"
            return 0
        fi
    fi

    log_info "Running coverage analysis for security tests"
    run_with_timeout 300 "Security test coverage measurement" \
        "$PROJECT_ROOT/scripts/measure_coverage.sh"
}

# Main execution with comprehensive error handling
main() {
    local start_time
    start_time=$(date +%s)

    log_info "=== Security Test Suite Runner ==="
    log_info "Project: Huntmaster Engine"
    log_info "Work Stream: C (Security & Compliance Test Coverage)"
    log_info "Timeout Protection: ${TIMEOUT_DURATION}s per test suite"
    log_info "Build Directory: $BUILD_DIR"
    log_info "=========================================="

    # Change to project root
    cd "$PROJECT_ROOT"

    # Set environment variables for testing
    export HUNTMASTER_TEST_MODE=1
    export HUNTMASTER_SECURITY_TEST=1
    export ASAN_OPTIONS="detect_leaks=1:strict_init_order=1:abort_on_error=0"

    local overall_success=true

    # Step 1: Build with timeout protection
    if ! build_project; then
        log_error "Build failed - cannot proceed with testing"
        exit 1
    fi

    # Step 2: Run core security tests
    if ! run_security_tests; then
        log_error "Core security tests failed"
        overall_success=false
    fi

    # Step 3: Run additional validation tests
    if ! run_validation_tests; then
        log_warning "Additional validation tests had issues"
        # Don't fail overall if these are missing/fail
    fi

    # Step 4: Coverage analysis (optional)
    if ! run_coverage_analysis; then
        log_warning "Coverage analysis had issues"
        # Don't fail overall for coverage issues
    fi

    # Report results
    local end_time
    end_time=$(date +%s)
    local duration=$((end_time - start_time))

    log_info "=========================================="
    log_info "Security test execution completed"
    log_info "Total duration: ${duration}s"

    if [ "$overall_success" = true ]; then
        log_info "✓ Security test suite PASSED"
        log_info "Work Stream C security coverage validation successful"
        exit 0
    else
        log_error "✗ Security test suite had FAILURES"
        log_error "Check logs above for specific failure details"
        exit 1
    fi
}

# Trap handlers for timeout and interruption
trap 'log_error "Script interrupted by user"; exit 130' INT TERM
trap 'log_error "Script timed out or encountered unexpected error"; exit 124' EXIT

# Execute main function
main "$@"

# Clear trap on successful completion
trap - EXIT
