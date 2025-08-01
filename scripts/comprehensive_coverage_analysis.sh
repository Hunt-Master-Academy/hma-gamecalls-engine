#!/bin/bash

# =============================================================================
# Comprehensive Coverage Analysis for 90% Target Achievement
# =============================================================================
#
# This script provides detailed coverage analysis and actionable recommendations
# to achieve the 90% coverage target for the Huntmaster Engine.
#
# USAGE:
#   ./scripts/comprehensive_coverage_analysis.sh
#
# FEATURES:
#   ✅ Per-file coverage analysis
#   ✅ Overall project coverage calculation
#   ✅ Gap identification for uncovered areas
#   ✅ Prioritized recommendations for coverage improvement
#   ✅ Test execution with coverage measurement
#   ✅ Automated reporting and tracking
#
# =============================================================================

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
BUILD_DIR="build"
PROJECT_ROOT="$(pwd)"
COVERAGE_OUTPUT_DIR="$PROJECT_ROOT/coverage_analysis"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
COVERAGE_REPORT="$COVERAGE_OUTPUT_DIR/comprehensive_coverage_$TIMESTAMP.log"
COVERAGE_TARGET=90

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

execute_tests_with_coverage() {
    print_section "EXECUTING HIGH-COVERAGE TESTS"

    # Clean previous coverage data
    cd "$PROJECT_ROOT/$BUILD_DIR" || exit 1
    find . -name "*.gcda" -delete

    echo "Running strategic tests for maximum coverage..." | tee -a "$COVERAGE_REPORT"

    # Execute key tests that provide good coverage
    local strategic_tests=(
        "BasicCoverageTest"
        "CoverageOptimizerTest"
        "UnifiedEngineTest"
        "UnifiedEngineComprehensiveTest"
        "MFCCConsistencyUnifiedTest"
        "MFCCDirectUnifiedTest"
        "DTWUnifiedTest"
        "ValidationUnifiedTest"
        "EndToEndTest"
        "MemoryManagementTest"
        "CrossPlatformTest"
        "TestHarness"
        "RecorderTest"
        "RecordingTest"
    )

    local executed_tests=0
    local successful_tests=0

    for test in "${strategic_tests[@]}"; do
        if [[ -f "./bin/$test" ]]; then
            echo "Executing: $test" | tee -a "$COVERAGE_REPORT"
            if timeout 60 "./bin/$test" >/dev/null 2>&1; then
                echo "✅ $test - SUCCESS" | tee -a "$COVERAGE_REPORT"
                ((successful_tests++))
            else
                echo "❌ $test - FAILED" | tee -a "$COVERAGE_REPORT"
            fi
            ((executed_tests++))
        else
            echo "⏭️  $test - not available" | tee -a "$COVERAGE_REPORT"
        fi
    done

    echo "Test execution summary: $successful_tests/$executed_tests successful" | tee -a "$COVERAGE_REPORT"
    cd "$PROJECT_ROOT" || exit 1
}

analyze_coverage() {
    print_section "COMPREHENSIVE COVERAGE ANALYSIS"

    cd "$PROJECT_ROOT/$BUILD_DIR" || exit 1

    echo "Analyzing coverage for all source files..." | tee -a "$COVERAGE_REPORT"

    # Generate coverage for all .gcno files
    local total_lines=0
    local covered_lines=0
    local total_files=0
    local files_above_threshold=0
    local coverage_threshold=75  # Files with >75% coverage

    # Coverage summary
    echo "Per-file coverage analysis:" | tee -a "$COVERAGE_REPORT"
    echo "=============================" | tee -a "$COVERAGE_REPORT"

    # Array to store low-coverage files for recommendations
    declare -a low_coverage_files=()

    # Process source files - search in the actual CMake build directory structure
    for gcno_file in $(find . -name "*.gcno" 2>/dev/null); do
        if [[ -f "$gcno_file" ]]; then
            # Generate coverage report
            local gcov_output=$(gcov -n "$gcno_file" 2>/dev/null)

            # Extract filename and coverage - handle CMake build paths
            local source_file=$(echo "$gcov_output" | grep "^File" | head -1 | sed "s/^File '//;s/'$//")
            local coverage_line=$(echo "$gcov_output" | grep "Lines executed:" | head -1)

            # Skip if this isn't a source file or if it's in a test directory
            if [[ -n "$coverage_line" && "$source_file" == *".cpp"* && "$source_file" != *"/tests/"* ]]; then
                # Parse coverage percentage and line counts
                local percent=$(echo "$coverage_line" | sed 's/.*executed:\([0-9.]*\)%.*/\1/')
                local lines_info=$(echo "$coverage_line" | sed 's/.*executed:[0-9.]*% of \([0-9]*\).*/\1/')

                if [[ -n "$percent" && -n "$lines_info" ]]; then
                    local lines_count="$lines_info"
                    local covered_count=$(echo "scale=0; $lines_count * $percent / 100" | bc -l 2>/dev/null || echo "0")

                    # Update totals
                    total_lines=$((total_lines + lines_count))
                    covered_lines=$((covered_lines + covered_count))
                    ((total_files++))

                    # Format source file path
                    local display_file=$(basename "$source_file")

                    if (( $(echo "$percent >= $coverage_threshold" | bc -l) )); then
                        echo "✅ $display_file: ${percent}% ($covered_count/$lines_count lines)" | tee -a "$COVERAGE_REPORT"
                        ((files_above_threshold++))
                    elif (( $(echo "$percent >= 50" | bc -l) )); then
                        echo "⚠️  $display_file: ${percent}% ($covered_count/$lines_count lines)" | tee -a "$COVERAGE_REPORT"
                        low_coverage_files+=("$display_file:$percent")
                    else
                        echo "❌ $display_file: ${percent}% ($covered_count/$lines_count lines)" | tee -a "$COVERAGE_REPORT"
                        low_coverage_files+=("$display_file:$percent")
                    fi
                fi
            fi
        fi
    done

    echo "" | tee -a "$COVERAGE_REPORT"
    echo "OVERALL COVERAGE SUMMARY:" | tee -a "$COVERAGE_REPORT"
    echo "=========================" | tee -a "$COVERAGE_REPORT"
    echo "Total Files Analyzed: $total_files" | tee -a "$COVERAGE_REPORT"
    echo "Files Above ${coverage_threshold}%: $files_above_threshold" | tee -a "$COVERAGE_REPORT"
    echo "Total Lines: $total_lines" | tee -a "$COVERAGE_REPORT"
    echo "Covered Lines: $covered_lines" | tee -a "$COVERAGE_REPORT"

    if [[ $total_lines -gt 0 ]]; then
        local overall_coverage=$(echo "scale=2; $covered_lines * 100 / $total_lines" | bc -l)
        echo "Overall Coverage: ${overall_coverage}%" | tee -a "$COVERAGE_REPORT"

        if (( $(echo "$overall_coverage >= $COVERAGE_TARGET" | bc -l) )); then
            echo -e "${GREEN}🎯 COVERAGE TARGET ACHIEVED! ${overall_coverage}% >= ${COVERAGE_TARGET}%${NC}" | tee -a "$COVERAGE_REPORT"
        else
            local needed_coverage=$(echo "scale=2; $COVERAGE_TARGET - $overall_coverage" | bc -l)
            echo -e "${YELLOW}📊 Coverage Gap: ${needed_coverage}% to reach ${COVERAGE_TARGET}% target${NC}" | tee -a "$COVERAGE_REPORT"

            # Calculate lines needed
            local target_lines=$(echo "scale=0; $total_lines * $COVERAGE_TARGET / 100" | bc -l)
            local additional_lines_needed=$(echo "scale=0; $target_lines - $covered_lines" | bc -l)
            echo "Additional lines needed: $additional_lines_needed" | tee -a "$COVERAGE_REPORT"
        fi
    else
        echo "❌ No coverage data available" | tee -a "$COVERAGE_REPORT"
    fi

    # Recommendations for improvement
    if [[ ${#low_coverage_files[@]} -gt 0 ]]; then
        echo "" | tee -a "$COVERAGE_REPORT"
        echo "PRIORITY IMPROVEMENT TARGETS:" | tee -a "$COVERAGE_REPORT"
        echo "============================" | tee -a "$COVERAGE_REPORT"
        echo "Files with coverage below ${coverage_threshold}% (prioritized by impact):" | tee -a "$COVERAGE_REPORT"

        for file_info in "${low_coverage_files[@]}"; do
            echo "🎯 $file_info" | tee -a "$COVERAGE_REPORT"
        done
    fi

    cd "$PROJECT_ROOT" || exit 1
}

generate_improvement_recommendations() {
    print_section "ACTIONABLE RECOMMENDATIONS"

    echo "STRATEGIES TO ACHIEVE 90% COVERAGE:" | tee -a "$COVERAGE_REPORT"
    echo "====================================" | tee -a "$COVERAGE_REPORT"

    echo "1. 🧪 ENHANCE EXISTING TESTS:" | tee -a "$COVERAGE_REPORT"
    echo "   - Add edge case scenarios to existing test suites" | tee -a "$COVERAGE_REPORT"
    echo "   - Increase error condition testing" | tee -a "$COVERAGE_REPORT"
    echo "   - Test concurrent access patterns" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"

    echo "2. 🎯 TARGET UNCOVERED CODE PATHS:" | tee -a "$COVERAGE_REPORT"
    echo "   - Focus on conditional branches and error handlers" | tee -a "$COVERAGE_REPORT"
    echo "   - Test initialization and cleanup code paths" | tee -a "$COVERAGE_REPORT"
    echo "   - Validate configuration and parameter validation" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"

    echo "3. 🔧 CREATE TARGETED TESTS:" | tee -a "$COVERAGE_REPORT"
    echo "   - Unit tests for low-coverage components" | tee -a "$COVERAGE_REPORT"
    echo "   - Integration tests for component interactions" | tee -a "$COVERAGE_REPORT"
    echo "   - Stress tests for memory and performance paths" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"

    echo "4. 📊 CONTINUOUS MONITORING:" | tee -a "$COVERAGE_REPORT"
    echo "   - Run this analysis after each test enhancement" | tee -a "$COVERAGE_REPORT"
    echo "   - Track coverage improvements over time" | tee -a "$COVERAGE_REPORT"
    echo "   - Maintain coverage regression protection" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"
}

main() {
    print_header "COMPREHENSIVE COVERAGE ANALYSIS FOR 90% TARGET"

    echo "🎯 Target Coverage: ${COVERAGE_TARGET}%" | tee "$COVERAGE_REPORT"
    echo "📅 Analysis Date: $(date)" | tee -a "$COVERAGE_REPORT"
    echo "📍 Project: Huntmaster Engine Unified Audio Engine" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"

    # Verify we're in the right directory
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo "❌ Build directory not found. Please run from project root after building." | tee -a "$COVERAGE_REPORT"
        exit 1
    fi

    # Check if coverage instrumentation is available
    local gcno_count=$(find "$BUILD_DIR" -name "*.gcno" | wc -l)
    if [[ $gcno_count -eq 0 ]]; then
        echo "❌ No coverage instrumentation found. Build with --coverage flags." | tee -a "$COVERAGE_REPORT"
        exit 1
    fi

    echo "✅ Found $gcno_count instrumented files" | tee -a "$COVERAGE_REPORT"

    # Execute tests and analyze coverage
    execute_tests_with_coverage
    analyze_coverage
    generate_improvement_recommendations

    print_header "ANALYSIS COMPLETE"
    echo "📄 Detailed report saved to: $COVERAGE_REPORT"
    echo "📊 Coverage analysis timestamp: $TIMESTAMP"
    echo ""
    echo "🚀 Next steps:"
    echo "   1. Review the detailed coverage report"
    echo "   2. Focus on files with <75% coverage"
    echo "   3. Add targeted tests for uncovered code paths"
    echo "   4. Re-run this analysis to track progress"
    echo ""
    echo "🎯 Goal: Achieve 90%+ overall project coverage"
}

# Execute main function
main "$@"
