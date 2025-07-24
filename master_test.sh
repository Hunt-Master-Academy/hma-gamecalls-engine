#!/bin/bash

# ==============================================================================
# Master Test Runner for Huntmaster Audio Engine
# ==============================================================================
# This script orchestrates all testing phases to ensure comprehensive
# validation before WASM deployment.
#
# Usage: ./master_test.sh [--phase=<phase>] [--verbose] [--continue-on-fail]
# Phases: build, unit, integration, components, validation, all
# ==============================================================================

set -e

# Configuration
PHASE="all"
VERBOSE=true
CONTINUE_ON_FAIL=false
LOG_DIR="test_logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --phase=*)
            PHASE="${1#*=}"
            shift
            ;;
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        --quiet|-q)
            VERBOSE=false
            shift
            ;;
        --continue-on-fail)
            CONTINUE_ON_FAIL=true
            shift
            ;;
        --help|-h)
            echo "Master Test Runner for Huntmaster Audio Engine"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --phase=<phase>      Run specific phase (build, unit, integration, components, validation, all)"
            echo "  --verbose, -v        Enable verbose output (default)"
            echo "  --quiet, -q          Disable verbose output"
            echo "  --continue-on-fail   Continue testing even if a phase fails"
            echo "  --help, -h           Show this help"
            echo ""
            echo "Phases:"
            echo "  build        - Build project and verify compilation"
            echo "  unit         - Run unit tests"
            echo "  integration  - Run integration tests"
            echo "  components   - Run detailed component tests"
            echo "  validation   - Run pre-WASM validation"
            echo "  all          - Run all phases sequentially"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Logging functions
log_phase() { echo -e "${PURPLE}[PHASE]${NC} $1"; }
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[✓]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[⚠]${NC} $1"; }
log_error() { echo -e "${RED}[✗]${NC} $1"; }
log_step() { echo -e "${CYAN}[STEP]${NC} $1"; }

# Setup logging
setup_logging() {
    mkdir -p "$LOG_DIR"
    log_info "Test logs will be saved to: $LOG_DIR/"
}

# Function to run a phase with logging
run_phase() {
    local phase_name="$1"
    local script_path="$2"
    local script_args="$3"
    local log_file="$LOG_DIR/${phase_name}_${TIMESTAMP}.log"

    log_phase "Starting $phase_name phase..."

    # Check if script exists
    if [[ ! -f "$script_path" ]]; then
        log_error "Script not found: $script_path"
        return 1
    fi

    # Make script executable
    chmod +x "$script_path"

    # Run the script
    local start_time=$(date +%s)

    if [[ "$VERBOSE" == true ]]; then
        "$script_path" $script_args 2>&1 | tee "$log_file"
        local result=${PIPESTATUS[0]}
    else
        "$script_path" $script_args > "$log_file" 2>&1
        local result=$?
    fi

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    if [[ $result -eq 0 ]]; then
        log_success "$phase_name phase completed successfully (${duration}s)"
        echo "✅ $phase_name: PASSED (${duration}s)" >> "$LOG_DIR/summary_${TIMESTAMP}.txt"
        return 0
    else
        log_error "$phase_name phase failed (${duration}s)"
        echo "❌ $phase_name: FAILED (${duration}s)" >> "$LOG_DIR/summary_${TIMESTAMP}.txt"

        if [[ "$CONTINUE_ON_FAIL" == false ]]; then
            log_error "Stopping execution due to failure. Use --continue-on-fail to continue."
            log_info "Check log file for details: $log_file"
            exit 1
        else
            log_warning "Continuing despite failure..."
        fi

        return 1
    fi
}

# Build phase
run_build_phase() {
    log_step "Phase 1: Build and Compilation"
    run_phase "build" "./comprehensive_test.sh" "--test-only=build"
}

# Unit tests phase
run_unit_phase() {
    log_step "Phase 2: Unit Testing"
    run_phase "unit" "./comprehensive_test.sh" "--test-only=unit --skip-build"
}

# Integration tests phase
run_integration_phase() {
    log_step "Phase 3: Integration Testing"
    run_phase "integration" "./comprehensive_test.sh" "--test-only=integration --skip-build"
}

# Component tests phase
run_components_phase() {
    log_step "Phase 4: Component Testing"

    # Test each component individually
    local components=("engine" "mfcc" "dtw" "audio")
    local component_passed=0
    local component_total=${#components[@]}

    for component in "${components[@]}"; do
        local args="$component"
        if [[ "$VERBOSE" == true ]]; then
            args="$args --verbose"
        fi

        if run_phase "component_$component" "./component_test.sh" "$args"; then
            component_passed=$((component_passed + 1))
        fi
    done

    log_info "Component tests summary: $component_passed/$component_total components passed"

    if [[ $component_passed -eq $component_total ]]; then
        return 0
    else
        return 1
    fi
}

# Validation phase
run_validation_phase() {
    log_step "Phase 5: Pre-WASM Validation"

    local args=""
    if [[ "$VERBOSE" == true ]]; then
        args="--verbose"
    fi

    run_phase "validation" "./pre_wasm_validation.sh" "$args"
}

# Performance benchmarking
run_performance_phase() {
    log_step "Phase 6: Performance Benchmarking"
    run_phase "performance" "./comprehensive_test.sh" "--test-only=performance --skip-build"
}

# Generate comprehensive report
generate_final_report() {
    local report_file="$LOG_DIR/final_test_report_${TIMESTAMP}.md"

    log_info "Generating final test report..."

    {
        echo "# Huntmaster Audio Engine - Final Test Report"
        echo ""
        echo "**Generated:** $(date)"
        echo "**Test Session:** $TIMESTAMP"
        echo "**Test Phase:** $PHASE"
        echo ""
        echo "## Executive Summary"
        echo ""

        # Count passed/failed from summary
        if [[ -f "$LOG_DIR/summary_${TIMESTAMP}.txt" ]]; then
            local total_tests=$(wc -l < "$LOG_DIR/summary_${TIMESTAMP}.txt")
            local passed_tests=$(grep -c "PASSED" "$LOG_DIR/summary_${TIMESTAMP}.txt" || echo 0)
            local failed_tests=$(grep -c "FAILED" "$LOG_DIR/summary_${TIMESTAMP}.txt" || echo 0)

            echo "- **Total Test Phases:** $total_tests"
            echo "- **Passed:** $passed_tests"
            echo "- **Failed:** $failed_tests"
            echo "- **Success Rate:** $(( passed_tests * 100 / total_tests ))%"
        fi

        echo ""
        echo "## Test Phase Results"
        echo ""

        if [[ -f "$LOG_DIR/summary_${TIMESTAMP}.txt" ]]; then
            while read -r line; do
                echo "- $line"
            done < "$LOG_DIR/summary_${TIMESTAMP}.txt"
        fi

        echo ""
        echo "## Detailed Logs"
        echo ""
        echo "Individual phase logs can be found in the \`$LOG_DIR/\` directory:"
        echo ""

        for log_file in "$LOG_DIR"/*_${TIMESTAMP}.log; do
            if [[ -f "$log_file" ]]; then
                local basename=$(basename "$log_file")
                echo "- [\`$basename\`](./$basename)"
            fi
        done

        echo ""
        echo "## Recommendations"
        echo ""

        if [[ -f "$LOG_DIR/summary_${TIMESTAMP}.txt" ]] && grep -q "FAILED" "$LOG_DIR/summary_${TIMESTAMP}.txt"; then
            echo "### ❌ Issues Found"
            echo ""
            echo "The following phases failed and require attention:"
            echo ""
            grep "FAILED" "$LOG_DIR/summary_${TIMESTAMP}.txt" | while read -r line; do
                echo "- $line"
            done
            echo ""
            echo "**Next Steps:**"
            echo "1. Review the individual log files for failed phases"
            echo "2. Fix the identified issues"
            echo "3. Re-run the specific failed phases"
            echo "4. Once all issues are resolved, proceed with WASM deployment"
        else
            echo "### ✅ Ready for WASM Deployment"
            echo ""
            echo "All test phases completed successfully! The engine is ready for WASM compilation."
            echo ""
            echo "**Next Steps:**"
            echo "1. Run \`./scripts/build_wasm.sh\` to compile for WASM"
            echo "2. Test WASM bindings in browser environment"
            echo "3. Deploy to web platform"
        fi

        echo ""
        echo "## System Information"
        echo ""
        echo "- **OS:** $(uname -s) $(uname -r)"
        echo "- **Architecture:** $(uname -m)"
        echo "- **CPU Cores:** $(nproc 2>/dev/null || echo 'Unknown')"
        echo "- **Memory:** $(free -h 2>/dev/null | grep '^Mem:' | awk '{print $2}' || echo 'Unknown')"
        echo ""

    } > "$report_file"

    log_success "Final report generated: $report_file"
}

# Display test summary
display_summary() {
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════════════════════════╗"
    echo "║                                    TEST EXECUTION SUMMARY                                       ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════════════════════════╝"

    if [[ -f "$LOG_DIR/summary_${TIMESTAMP}.txt" ]]; then
        cat "$LOG_DIR/summary_${TIMESTAMP}.txt"
    fi

    echo ""
    log_info "Test session completed. Check $LOG_DIR/ for detailed logs."
}

# Main execution function
main() {
    echo "╔══════════════════════════════════════════════════════════════════════════════════════════════════╗"
    echo "║                        Huntmaster Audio Engine - Master Test Runner                            ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════════════════════════╝"
    echo ""

    log_info "Test Phase: $PHASE"
    log_info "Verbose Mode: $VERBOSE"
    log_info "Continue on Failure: $CONTINUE_ON_FAIL"
    log_info "Session ID: $TIMESTAMP"
    echo ""

    # Setup
    setup_logging

    # Make scripts executable
    chmod +x *.sh

    # Execute phases based on selection
    case "$PHASE" in
        "build")
            run_build_phase
            ;;
        "unit")
            run_unit_phase
            ;;
        "integration")
            run_integration_phase
            ;;
        "components")
            run_components_phase
            ;;
        "validation")
            run_validation_phase
            ;;
        "performance")
            run_performance_phase
            ;;
        "all")
            log_info "Running complete test suite..."

            # Run all phases in sequence
            run_build_phase
            run_unit_phase
            run_integration_phase
            run_components_phase
            run_validation_phase
            run_performance_phase
            ;;
        *)
            log_error "Unknown phase: $PHASE"
            echo "Valid phases: build, unit, integration, components, validation, performance, all"
            exit 1
            ;;
    esac

    # Generate final report and summary
    generate_final_report
    display_summary

    # Check overall result
    if [[ -f "$LOG_DIR/summary_${TIMESTAMP}.txt" ]] && grep -q "FAILED" "$LOG_DIR/summary_${TIMESTAMP}.txt"; then
        log_error "Some test phases failed. Review the logs and fix issues before proceeding."
        exit 1
    else
        log_success "🎉 All test phases completed successfully! Engine is ready for WASM deployment."
        exit 0
    fi
}

# Execute main function
main "$@"
