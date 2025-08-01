#!/bin/bash

# =============================================================================
# Enhanced Coverage Analysis - Fixed Version for All Source Files
# =============================================================================
#
# This script correctly analyzes coverage for ALL source files in the project
# by properly handling CMake build directory structure and filtering out
# system headers and test files.
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
COVERAGE_REPORT="$COVERAGE_OUTPUT_DIR/enhanced_coverage_$TIMESTAMP.log"
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

analyze_comprehensive_coverage() {
    print_section "COMPREHENSIVE SOURCE FILE COVERAGE ANALYSIS"

    cd "$PROJECT_ROOT/$BUILD_DIR" || exit 1

    echo "Analyzing coverage for all instrumented source files..." | tee -a "$COVERAGE_REPORT"

    # Coverage summary
    echo "Per-file coverage analysis:" | tee -a "$COVERAGE_REPORT"
    echo "=============================" | tee -a "$COVERAGE_REPORT"

    local total_lines=0
    local covered_lines=0
    local total_files=0
    local files_above_threshold=0
    local coverage_threshold=75  # Files with >75% coverage

    # Array to store all coverage data
    declare -a coverage_data=()

    # Process all .gcno files and get coverage for project source files only
    for gcno_file in $(find . -name "*.gcno" -path "*/CMakeFiles/UnifiedAudioEngine.dir/*" 2>/dev/null); do
        if [[ -f "$gcno_file" ]]; then
            # Generate coverage report
            local gcov_output=$(gcov -n "$gcno_file" 2>/dev/null)

            # Process each file mentioned in the gcov output
            while IFS= read -r line; do
                if [[ "$line" =~ ^File\ \'(.*)\'$ ]]; then
                    local source_file="${BASH_REMATCH[1]}"

                    # Only process our project source files (not system headers, not test files)
                    if [[ "$source_file" == *"/huntmaster-engine/src/"* && "$source_file" == *".cpp" ]]; then
                        # Read the next line to get coverage info
                        read -r coverage_line

                        if [[ "$coverage_line" =~ Lines\ executed:([0-9.]+)%\ of\ ([0-9]+) ]]; then
                            local percent="${BASH_REMATCH[1]}"
                            local total_file_lines="${BASH_REMATCH[2]}"
                            local covered_file_lines=$(echo "scale=0; $total_file_lines * $percent / 100" | bc -l 2>/dev/null || echo "0")

                            # Extract just the filename for display
                            local display_file=$(basename "$source_file")

                            # Store coverage data
                            coverage_data+=("$display_file:$percent:$covered_file_lines:$total_file_lines")

                            # Update totals
                            total_lines=$((total_lines + total_file_lines))
                            covered_lines=$((covered_lines + covered_file_lines))
                            ((total_files++))

                            # Display coverage with color coding
                            if (( $(echo "$percent >= $coverage_threshold" | bc -l) )); then
                                echo -e "✅ ${GREEN}$display_file: ${percent}% ($covered_file_lines/$total_file_lines lines)${NC}" | tee -a "$COVERAGE_REPORT"
                                ((files_above_threshold++))
                            elif (( $(echo "$percent >= 50" | bc -l) )); then
                                echo -e "⚠️  ${YELLOW}$display_file: ${percent}% ($covered_file_lines/$total_file_lines lines)${NC}" | tee -a "$COVERAGE_REPORT"
                            else
                                echo -e "❌ ${RED}$display_file: ${percent}% ($covered_file_lines/$total_file_lines lines)${NC}" | tee -a "$COVERAGE_REPORT"
                            fi
                        fi
                    fi
                fi
            done <<< "$gcov_output"
        fi
    done

    echo "" | tee -a "$COVERAGE_REPORT"
    echo -e "${CYAN}OVERALL COVERAGE SUMMARY:${NC}" | tee -a "$COVERAGE_REPORT"
    echo "=========================" | tee -a "$COVERAGE_REPORT"
    echo "Total Source Files Analyzed: $total_files" | tee -a "$COVERAGE_REPORT"
    echo "Files Above ${coverage_threshold}%: $files_above_threshold" | tee -a "$COVERAGE_REPORT"
    echo "Total Lines of Code: $total_lines" | tee -a "$COVERAGE_REPORT"
    echo "Covered Lines: $covered_lines" | tee -a "$COVERAGE_REPORT"

    if [[ $total_lines -gt 0 ]]; then
        local overall_coverage=$(echo "scale=2; $covered_lines * 100 / $total_lines" | bc -l)
        echo -e "${BLUE}Overall Project Coverage: ${overall_coverage}%${NC}" | tee -a "$COVERAGE_REPORT"

        if (( $(echo "$overall_coverage >= $COVERAGE_TARGET" | bc -l) )); then
            echo -e "${GREEN}🎯 COVERAGE TARGET ACHIEVED! ${overall_coverage}% >= ${COVERAGE_TARGET}%${NC}" | tee -a "$COVERAGE_REPORT"
        else
            local needed_coverage=$(echo "scale=2; $COVERAGE_TARGET - $overall_coverage" | bc -l)
            echo -e "${YELLOW}📊 Coverage Gap: ${needed_coverage}% to reach ${COVERAGE_TARGET}% target${NC}" | tee -a "$COVERAGE_REPORT"

            # Calculate lines needed
            local target_lines=$(echo "scale=0; $total_lines * $COVERAGE_TARGET / 100" | bc -l)
            local additional_lines_needed=$(echo "scale=0; $target_lines - $covered_lines" | bc -l)
            echo "Additional lines to cover: $additional_lines_needed" | tee -a "$COVERAGE_REPORT"
        fi
    else
        echo "❌ No coverage data available" | tee -a "$COVERAGE_REPORT"
    fi

    # Sort and display top priority files for improvement
    echo "" | tee -a "$COVERAGE_REPORT"
    echo -e "${YELLOW}TOP PRIORITY IMPROVEMENT TARGETS:${NC}" | tee -a "$COVERAGE_REPORT"
    echo "====================================" | tee -a "$COVERAGE_REPORT"
    echo "Files ranked by impact (lines * low coverage):" | tee -a "$COVERAGE_REPORT"

    # Create priority list based on potential coverage gain
    declare -a priority_files=()
    for item in "${coverage_data[@]}"; do
        IFS=':' read -r filename percent covered_lines total_lines <<< "$item"
        if (( $(echo "$percent < $coverage_threshold" | bc -l) )); then
            local potential_gain=$(echo "scale=0; $total_lines * (75 - $percent) / 100" | bc -l)
            priority_files+=("$potential_gain:$filename:$percent:$total_lines")
        fi
    done

    # Sort by potential gain (descending)
    IFS=$'\n' sorted=($(sort -t: -k1 -nr <<< "${priority_files[*]}"))

    local count=0
    for item in "${sorted[@]}"; do
        if [[ $count -lt 10 ]]; then  # Show top 10
            IFS=':' read -r potential_gain filename percent total_lines <<< "$item"
            echo "🎯 $filename: $percent% coverage, $total_lines lines (potential gain: +$potential_gain lines)" | tee -a "$COVERAGE_REPORT"
            ((count++))
        fi
    done

    cd "$PROJECT_ROOT" || exit 1
}

generate_detailed_recommendations() {
    print_section "DETAILED IMPROVEMENT STRATEGY"

    echo -e "${CYAN}SPECIFIC ACTIONS TO REACH 90% COVERAGE:${NC}" | tee -a "$COVERAGE_REPORT"
    echo "=========================================" | tee -a "$COVERAGE_REPORT"

    echo "1. 🔍 FOCUS ON HIGH-IMPACT FILES:" | tee -a "$COVERAGE_REPORT"
    echo "   - Target files with >100 lines and <50% coverage first" | tee -a "$COVERAGE_REPORT"
    echo "   - Each line covered in large files has bigger impact" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"

    echo "2. 🧪 SYSTEMATIC TEST ENHANCEMENT:" | tee -a "$COVERAGE_REPORT"
    echo "   - Add unit tests for uncovered public methods" | tee -a "$COVERAGE_REPORT"
    echo "   - Test error handling and edge cases" | tee -a "$COVERAGE_REPORT"
    echo "   - Test configuration and initialization paths" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"

    echo "3. 🎯 COVERAGE-DRIVEN TEST CREATION:" | tee -a "$COVERAGE_REPORT"
    echo "   - Use 'gcov -f filename.cpp' to see function-level coverage" | tee -a "$COVERAGE_REPORT"
    echo "   - Use 'gcov -b filename.cpp' to see branch coverage details" | tee -a "$COVERAGE_REPORT"
    echo "   - Create tests specifically for uncovered branches" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"

    echo "4. 📊 INCREMENTAL PROGRESS TRACKING:" | tee -a "$COVERAGE_REPORT"
    echo "   - Re-run this analysis after adding each new test" | tee -a "$COVERAGE_REPORT"
    echo "   - Aim for +5-10% coverage improvement per iteration" | tee -a "$COVERAGE_REPORT"
    echo "   - Focus on one component at a time for manageable progress" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"
}

main() {
    print_header "ENHANCED COVERAGE ANALYSIS - ALL SOURCE FILES"

    echo "🎯 Target Coverage: ${COVERAGE_TARGET}%" | tee "$COVERAGE_REPORT"
    echo "📅 Analysis Date: $(date)" | tee -a "$COVERAGE_REPORT"
    echo "📍 Project: Huntmaster Engine - Complete Source Analysis" | tee -a "$COVERAGE_REPORT"
    echo "" | tee -a "$COVERAGE_REPORT"

    # Verify we're in the right directory
    if [[ ! -d "$BUILD_DIR" ]]; then
        echo "❌ Build directory not found. Please run from project root after building." | tee -a "$COVERAGE_REPORT"
        exit 1
    fi

    # Check if coverage data is available
    local gcda_count=$(find "$BUILD_DIR" -name "*.gcda" | wc -l)
    if [[ $gcda_count -eq 0 ]]; then
        echo "❌ No coverage data found. Please run tests first." | tee -a "$COVERAGE_REPORT"
        exit 1
    fi

    echo "✅ Found $gcda_count coverage data files" | tee -a "$COVERAGE_REPORT"

    # Analyze coverage for all source files
    analyze_comprehensive_coverage
    generate_detailed_recommendations

    print_header "ENHANCED ANALYSIS COMPLETE"
    echo "📄 Detailed report saved to: $COVERAGE_REPORT"
    echo "📊 Enhanced analysis timestamp: $TIMESTAMP"
    echo ""
    echo "🚀 Next steps:"
    echo "   1. Focus on the top priority files listed above"
    echo "   2. Use 'gcov -f filename.cpp' for function-level details"
    echo "   3. Create targeted tests for uncovered code paths"
    echo "   4. Re-run this analysis to track incremental progress"
    echo ""
    echo "🎯 Goal: Systematic progression toward 90%+ coverage"
}

# Execute main function
main "$@"
