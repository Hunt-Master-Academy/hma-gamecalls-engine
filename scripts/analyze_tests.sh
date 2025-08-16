#!/bin/bash

# =============================================================================
# Unified Test Analysis Script - Huntmaster Audio Engine
# =============================================================================
# Consolidated test analysis tool combining functionality from:
# - analyze_test_content.sh
# - test_quality_report.sh
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
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORT_FILE="$PROJECT_ROOT/test_analysis_report_$TIMESTAMP.txt"

# Analysis counters
empty_files=0
stub_files=0
minimal_files=0
redirect_files=0
deprecated_files=0
complete_files=0

# Quality metrics
total_assertions=0
total_test_cases=0
files_with_fixtures=0
files_with_mocks=0

# File lists for detailed reporting
empty_file_list=()
stub_file_list=()
minimal_file_list=()
redirect_file_list=()
deprecated_file_list=()
quality_issues=()

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
    echo "Analysis Types:"
    echo "  --content      Analyze test file content and completeness (default)"
    echo "  --quality      Analyze test quality metrics and patterns"
    echo "  --comprehensive Both content and quality analysis"
    echo ""
    echo "Options:"
    echo "  --format=text  Output format (text, json, html)"
    echo "  --verbose      Enable verbose output"
    echo "  --save-report  Save detailed report to file"
    echo "  --help         Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                          # Basic content analysis"
    echo "  $0 --quality --verbose      # Quality analysis with details"
    echo "  $0 --comprehensive --save-report # Full analysis with report"
}

parse_arguments() {
    local analysis_type="content"
    local output_format="text"
    local save_report=false
    local verbose=false

    while [[ $# -gt 0 ]]; do
        case $1 in
            --content)
                analysis_type="content"
                shift
                ;;
            --quality)
                analysis_type="quality"
                shift
                ;;
            --comprehensive)
                analysis_type="comprehensive"
                shift
                ;;
            --format=*)
                output_format="${1#*=}"
                shift
                ;;
            --verbose)
                verbose=true
                shift
                ;;
            --save-report)
                save_report=true
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

    echo "$analysis_type:$output_format:$save_report:$verbose"
}

analyze_file_content() {
    local file="$1"

    # Skip if file doesn't exist or is not readable
    if [[ ! -r "$file" ]]; then
        return
    fi

    local content=$(cat "$file" 2>/dev/null || echo "")
    local line_count=$(echo "$content" | wc -l)

    # Check for empty files
    if [[ $line_count -eq 0 ]] || [[ -z "$content" ]]; then
        ((empty_files++))
        empty_file_list+=("$file")
        return
    fi

    # Check for redirect/moved comments
    if echo "$content" | grep -qi "MOVED\|moved\|See.*\.cpp\|replaced by"; then
        ((redirect_files++))
        redirect_file_list+=("$file")
        return
    fi

    # Check for deprecated markers
    if echo "$content" | grep -qi "DEPRECATED\|deprecated\|LEGACY\|legacy"; then
        ((deprecated_files++))
        deprecated_file_list+=("$file")
        return
    fi

    # Check for stub patterns
    if echo "$content" | grep -qi "TODO\|FIXME\|STUB\|NOT IMPLEMENTED\|PLACEHOLDER"; then
        ((stub_files++))
        stub_file_list+=("$file")
        return
    fi

    # Check for minimal content (very few lines, likely incomplete)
    if [[ $line_count -lt 10 ]]; then
        # Check if it contains actual test content
        if echo "$content" | grep -q "TEST\|ASSERT\|EXPECT"; then
            ((complete_files++))
        else
            ((minimal_files++))
            minimal_file_list+=("$file")
        fi
        return
    fi

    # Check if file has actual test content
    if echo "$content" | grep -q "TEST\|ASSERT\|EXPECT\|class.*Test"; then
        ((complete_files++))
    else
        ((minimal_files++))
        minimal_file_list+=("$file")
    fi
}

analyze_file_quality() {
    local file="$1"

    if [[ ! -r "$file" ]]; then
        return
    fi

    local content=$(cat "$file" 2>/dev/null || echo "")

    # Count test cases and assertions
    local test_count=$(echo "$content" | grep -c "TEST\|TEST_F\|TEST_P" || echo "0")
    local assert_count=$(echo "$content" | grep -c "ASSERT_\|EXPECT_" || echo "0")

    total_test_cases=$((total_test_cases + test_count))
    total_assertions=$((total_assertions + assert_count))

    # Check for test fixtures
    if echo "$content" | grep -q "class.*Test.*public.*Test"; then
        ((files_with_fixtures++))
    fi

    # Check for mocking
    if echo "$content" | grep -q "Mock\|MOCK_\|gmock"; then
        ((files_with_mocks++))
    fi

    # Quality issues detection
    local issues=()

    # No assertions in test file
    if [[ $test_count -gt 0 && $assert_count -eq 0 ]]; then
        issues+=("No assertions found despite having test cases")
    fi

    # Very few assertions per test
    if [[ $test_count -gt 0 && $assert_count -gt 0 ]]; then
        local assertions_per_test=$((assert_count / test_count))
        if [[ $assertions_per_test -lt 2 ]]; then
            issues+=("Low assertion density (${assertions_per_test} per test)")
        fi
    fi

    # Missing error handling tests
    if [[ $test_count -gt 0 ]] && ! echo "$content" | grep -q "EXPECT_THROW\|ASSERT_THROW\|EXPECT_DEATH"; then
        issues+=("No error handling tests found")
    fi

    # Missing setup/teardown for complex tests
    if [[ $test_count -gt 3 ]] && ! echo "$content" | grep -q "SetUp\|TearDown\|setUp\|tearDown"; then
        issues+=("Complex test file without setup/teardown")
    fi

    # Store issues for this file
    if [[ ${#issues[@]} -gt 0 ]]; then
        for issue in "${issues[@]}"; do
            quality_issues+=("$(basename "$file"): $issue")
        done
    fi
}

analyze_test_content() {
    print_section "ANALYZING TEST FILE CONTENT"

    cd "$PROJECT_ROOT"

    print_status "Scanning test files for content analysis..."

    # Reset counters
    empty_files=0
    stub_files=0
    minimal_files=0
    redirect_files=0
    deprecated_files=0
    complete_files=0

    # Clear file lists
    empty_file_list=()
    stub_file_list=()
    minimal_file_list=()
    redirect_file_list=()
    deprecated_file_list=()

    # Find and analyze all .cpp test files
    while IFS= read -r -d '' file; do
        analyze_file_content "$file"
    done < <(find tests/ -name "*.cpp" -print0 2>/dev/null)

    # Report results
    echo
    echo "📊 CONTENT ANALYSIS SUMMARY" | tee -a "$REPORT_FILE"
    echo "=========================" | tee -a "$REPORT_FILE"
    echo "Empty Files:      $empty_files" | tee -a "$REPORT_FILE"
    echo "Stub Files:       $stub_files" | tee -a "$REPORT_FILE"
    echo "Minimal Files:    $minimal_files" | tee -a "$REPORT_FILE"
    echo "Redirect Files:   $redirect_files" | tee -a "$REPORT_FILE"
    echo "Deprecated Files: $deprecated_files" | tee -a "$REPORT_FILE"
    echo "Complete Files:   $complete_files" | tee -a "$REPORT_FILE"
    echo "" | tee -a "$REPORT_FILE"

    # Calculate percentages
    local total_active_files=$((empty_files + stub_files + minimal_files + redirect_files + complete_files))
    if [[ $total_active_files -gt 0 ]]; then
        local complete_percentage=$((complete_files * 100 / total_active_files))
        echo "📈 COMPLETION RATE: $complete_percentage% ($complete_files/$total_active_files active files)" | tee -a "$REPORT_FILE"
    else
        echo "📈 COMPLETION RATE: Unable to calculate" | tee -a "$REPORT_FILE"
    fi
}

analyze_test_quality() {
    print_section "ANALYZING TEST QUALITY METRICS"

    cd "$PROJECT_ROOT"

    print_status "Scanning test files for quality metrics..."

    # Reset counters
    total_assertions=0
    total_test_cases=0
    files_with_fixtures=0
    files_with_mocks=0
    quality_issues=()

    local total_files=0

    # Find and analyze all .cpp test files
    while IFS= read -r -d '' file; do
        analyze_file_quality "$file"
        ((total_files++))
    done < <(find tests/ -name "*.cpp" -print0 2>/dev/null)

    echo
    echo "📊 QUALITY ANALYSIS SUMMARY" | tee -a "$REPORT_FILE"
    echo "=========================" | tee -a "$REPORT_FILE"
    echo "Total test files analyzed: $total_files" | tee -a "$REPORT_FILE"
    echo "Total test cases: $total_test_cases" | tee -a "$REPORT_FILE"
    echo "Total assertions: $total_assertions" | tee -a "$REPORT_FILE"
    echo "Files with fixtures: $files_with_fixtures" | tee -a "$REPORT_FILE"
    echo "Files with mocks: $files_with_mocks" | tee -a "$REPORT_FILE"
    echo "" | tee -a "$REPORT_FILE"

    # Calculate quality metrics
    if [[ $total_test_cases -gt 0 ]]; then
        local assertions_per_test=$((total_assertions / total_test_cases))
        echo "Average assertions per test: $assertions_per_test" | tee -a "$REPORT_FILE"
    fi

    if [[ $total_files -gt 0 ]]; then
        local tests_per_file=$((total_test_cases / total_files))
        local fixture_percentage=$((files_with_fixtures * 100 / total_files))
        local mock_percentage=$((files_with_mocks * 100 / total_files))

        echo "Average tests per file: $tests_per_file" | tee -a "$REPORT_FILE"
        echo "Files using fixtures: $fixture_percentage%" | tee -a "$REPORT_FILE"
        echo "Files using mocks: $mock_percentage%" | tee -a "$REPORT_FILE"
    fi
}

generate_detailed_report() {
    local save_report="$1"

    if [[ "$save_report" != "true" ]]; then
        return
    fi

    print_section "GENERATING DETAILED REPORT"

    {
        echo "# Test Analysis Detailed Report - $(date)"
        echo "=========================================="
        echo ""

        # Content issues
        if [[ $empty_files -gt 0 ]]; then
            echo "## Empty Files ($empty_files)"
            printf "   %s\n" "${empty_file_list[@]}"
            echo ""
        fi

        if [[ $stub_files -gt 0 ]]; then
            echo "## Stub Files ($stub_files)"
            printf "   %s\n" "${stub_file_list[@]}"
            echo ""
        fi

        if [[ $minimal_files -gt 0 ]]; then
            echo "## Minimal Content Files ($minimal_files)"
            printf "   %s\n" "${minimal_file_list[@]}"
            echo ""
        fi

        if [[ $redirect_files -gt 0 ]]; then
            echo "## Redirect/Moved Files ($redirect_files)"
            printf "   %s\n" "${redirect_file_list[@]}"
            echo ""
        fi

        if [[ $deprecated_files -gt 0 ]]; then
            echo "## Deprecated Files ($deprecated_files)"
            printf "   %s\n" "${deprecated_file_list[@]}"
            echo ""
        fi

        # Quality issues
        if [[ ${#quality_issues[@]} -gt 0 ]]; then
            echo "## Quality Issues (${#quality_issues[@]})"
            printf "   %s\n" "${quality_issues[@]}"
            echo ""
        fi

        echo "## Recommendations"
        echo "=================="

        local problem_files=$((empty_files + stub_files + minimal_files))
        if [[ $problem_files -eq 0 ]]; then
            echo "✅ All active test files have meaningful content!"
        else
            echo "❌ $problem_files files need attention:"
            echo "   - Remove or consolidate empty files"
            echo "   - Complete stub files with actual tests"
            echo "   - Expand minimal files to proper test coverage"
        fi

        if [[ $redirect_files -gt 0 ]]; then
            echo "🔄 Consider removing redirect files if they're no longer needed"
        fi

        if [[ ${#quality_issues[@]} -gt 0 ]]; then
            echo "🔧 Address quality issues to improve test effectiveness"
        fi

        echo ""
        echo "## Next Steps"
        echo "============"
        echo "1. Use scripts/manage_tests.sh to clean up problematic files"
        echo "2. Focus on expanding stub and minimal files"
        echo "3. Add more assertions to improve test coverage"
        echo "4. Consider adding test fixtures for complex test scenarios"
        echo "5. Re-run this analysis to track improvements"

    } >> "$REPORT_FILE"

    print_success "Detailed report saved to: $(basename "$REPORT_FILE")"
}

generate_json_output() {
    local output_file="test_analysis_$TIMESTAMP.json"

    {
        echo "{"
        echo "  \"timestamp\": \"$(date -Iseconds)\","
        echo "  \"analysis_type\": \"test_analysis\","
        echo "  \"content_summary\": {"
        echo "    \"empty_files\": $empty_files,"
        echo "    \"stub_files\": $stub_files,"
        echo "    \"minimal_files\": $minimal_files,"
        echo "    \"redirect_files\": $redirect_files,"
        echo "    \"deprecated_files\": $deprecated_files,"
        echo "    \"complete_files\": $complete_files"
        echo "  },"
        echo "  \"quality_summary\": {"
        echo "    \"total_test_cases\": $total_test_cases,"
        echo "    \"total_assertions\": $total_assertions,"
        echo "    \"files_with_fixtures\": $files_with_fixtures,"
        echo "    \"files_with_mocks\": $files_with_mocks,"
        echo "    \"quality_issues_count\": ${#quality_issues[@]}"
        echo "  }"
        echo "}"
    } > "$output_file"

    print_success "JSON report saved to: $output_file"
}

main() {
    local config=$(parse_arguments "$@")
    local analysis_type=$(echo "$config" | cut -d: -f1)
    local output_format=$(echo "$config" | cut -d: -f2)
    local save_report=$(echo "$config" | cut -d: -f3)
    local verbose=$(echo "$config" | cut -d: -f4)

    print_header "UNIFIED TEST ANALYSIS - $(echo $analysis_type | tr '[:lower:]' '[:upper:]')"

    echo "📅 Analysis Date: $(date)" | tee "$REPORT_FILE"
    echo "🎯 Analysis Type: $analysis_type" | tee -a "$REPORT_FILE"
    echo "📍 Project: Huntmaster Engine" | tee -a "$REPORT_FILE"
    echo "" | tee -a "$REPORT_FILE"

    case "$analysis_type" in
        "content")
            analyze_test_content
            ;;
        "quality")
            analyze_test_quality
            ;;
        "comprehensive")
            analyze_test_content
            analyze_test_quality
            ;;
    esac

    case "$output_format" in
        "json")
            generate_json_output
            ;;
        "html")
            print_warning "HTML output not yet implemented, using text format"
            ;;
    esac

    generate_detailed_report "$save_report"

    print_header "ANALYSIS COMPLETE"

    local total_active=$((empty_files + stub_files + minimal_files + redirect_files + complete_files))
    if [[ $total_active -gt 0 ]]; then
        local completion_rate=$((complete_files * 100 / total_active))
        echo "📊 Test completion rate: $completion_rate%"

        if [[ $completion_rate -ge 90 ]]; then
            print_success "Excellent test file quality!"
        elif [[ $completion_rate -ge 75 ]]; then
            print_success "Good test file quality"
        elif [[ $completion_rate -ge 50 ]]; then
            print_warning "Test file quality needs improvement"
        else
            print_error "Poor test file quality - significant cleanup needed"
        fi
    fi
}

# Execute main function with all arguments
main "$@"
