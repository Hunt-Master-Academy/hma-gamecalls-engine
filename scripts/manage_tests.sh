#!/bin/bash

# =============================================================================
# Unified Test Management Script - Huntmaster Audio Engine
# =============================================================================
# Consolidated test management tool combining functionality from:
# - cleanup_test_files.sh
# - reorganize_tests.sh
# - verify_reorganization.sh
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
BACKUP_DIR="archive/test_management_$TIMESTAMP"
DRY_RUN=true

# Counters
files_cleaned=0
files_reorganized=0
files_verified=0

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
    echo "Usage: $0 [command] [options]"
    echo ""
    echo "Commands:"
    echo "  cleanup        Clean up empty, stub, and deprecated test files"
    echo "  reorganize     Reorganize tests into categorized directory structure"
    echo "  verify         Verify test organization and completeness"
    echo "  all            Execute cleanup, reorganize, and verify (default)"
    echo ""
    echo "Options:"
    echo "  --execute      Actually perform changes (default: dry-run)"
    echo "  --dry-run      Show what would be done without making changes"
    echo "  --verbose      Enable verbose output"
    echo "  --help         Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                     # Dry-run of all operations"
    echo "  $0 cleanup --execute   # Actually clean up test files"
    echo "  $0 verify              # Check test organization status"
}

parse_arguments() {
    local command="all"

    while [[ $# -gt 0 ]]; do
        case $1 in
            cleanup|reorganize|verify|all)
                command="$1"
                shift
                ;;
            --execute)
                DRY_RUN=false
                shift
                ;;
            --dry-run)
                DRY_RUN=true
                shift
                ;;
            --verbose)
                VERBOSE=true
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

    echo "$command"
}

setup_backup_directory() {
    if [[ "$DRY_RUN" == false ]]; then
        mkdir -p "$PROJECT_ROOT/$BACKUP_DIR"
        print_status "Created backup directory: $BACKUP_DIR"
    fi
}

handle_file() {
    local action="$1"
    local file="$2"
    local reason="$3"
    local target_dir="$4"

    if [[ "$DRY_RUN" == true ]]; then
        echo "🔄 WOULD $action: $file ($reason)"
        return
    fi

    case "$action" in
        "REMOVE")
            if [[ -f "$file" ]]; then
                # Backup before removal
                cp "$file" "$PROJECT_ROOT/$BACKUP_DIR/$(basename "$file").bak"
                rm "$file"
                print_success "REMOVED: $file ($reason)"
                ((files_cleaned++))
            fi
            ;;
        "MOVE")
            if [[ -f "$file" ]]; then
                local target="$PROJECT_ROOT/$BACKUP_DIR/$(basename "$file")"
                if [[ -n "$target_dir" ]]; then
                    mkdir -p "$target_dir"
                    target="$target_dir/$(basename "$file")"
                fi
                mv "$file" "$target"
                print_success "MOVED: $file to $target ($reason)"
                ((files_reorganized++))
            fi
            ;;
        "COPY")
            if [[ -f "$file" && -n "$target_dir" ]]; then
                mkdir -p "$target_dir"
                cp "$file" "$target_dir/"
                print_success "COPIED: $file to $target_dir/ ($reason)"
                ((files_reorganized++))
            fi
            ;;
    esac
}

update_cmake_lists() {
    local file_to_remove="$1"
    local cmake_file="tests/CMakeLists.txt"

    if [[ "$DRY_RUN" == true ]]; then
        echo "🔄 WOULD UPDATE: Remove $(basename "$file_to_remove" .cpp) from $cmake_file"
        return
    fi

    local test_name=$(basename "$file_to_remove" .cpp)
    if [[ -f "$PROJECT_ROOT/$cmake_file" ]] && grep -q "$test_name" "$PROJECT_ROOT/$cmake_file"; then
        sed -i "/$test_name/d" "$PROJECT_ROOT/$cmake_file"
        print_success "UPDATED: Removed $test_name from $cmake_file"
    fi
}

cleanup_test_files() {
    print_section "CLEANING UP TEST FILES"

    cd "$PROJECT_ROOT"

    local empty_removed=0
    local stub_removed=0
    local redirect_removed=0
    local deprecated_removed=0

    print_status "Analyzing test files for cleanup..."

    # Find and handle empty files
    echo
    echo "📋 Processing Empty Files:"
    while IFS= read -r -d '' file; do
        # Skip deprecated directory
        if [[ "$file" == *deprecated_legacy* ]]; then
            continue
        fi

        if [[ ! -s "$file" ]]; then
            handle_file "REMOVE" "$file" "empty file"
            update_cmake_lists "$file"
            ((empty_removed++))
        fi
    done < <(find tests/ -name "*.cpp" -print0 2>/dev/null)

    # Find and handle redirect files
    echo
    echo "🔀 Processing Redirect Files:"
    while IFS= read -r -d '' file; do
        # Skip deprecated directory
        if [[ "$file" == *deprecated_legacy* ]]; then
            continue
        fi

        local content=$(cat "$file" 2>/dev/null || echo "")

        if echo "$content" | grep -qi "MOVED\|moved\|See.*\.cpp\|replaced by"; then
            handle_file "REMOVE" "$file" "redirect/moved file"
            update_cmake_lists "$file"
            ((redirect_removed++))
        fi
    done < <(find tests/ -name "*.cpp" -print0 2>/dev/null)

    # Find and handle stub files
    echo
    echo "⚠️  Processing Stub Files:"
    while IFS= read -r -d '' file; do
        # Skip deprecated directory
        if [[ "$file" == *deprecated_legacy* ]]; then
            continue
        fi

        local content=$(cat "$file" 2>/dev/null || echo "")

        if echo "$content" | grep -qi "TODO\|FIXME\|STUB\|NOT IMPLEMENTED\|PLACEHOLDER"; then
            handle_file "MOVE" "$file" "stub file" "$PROJECT_ROOT/$BACKUP_DIR/stubs"
            update_cmake_lists "$file"
            ((stub_removed++))
        fi
    done < <(find tests/ -name "*.cpp" -print0 2>/dev/null)

    # Handle deprecated files
    echo
    echo "🗂️  Processing Deprecated Files:"
    if [[ -d "tests/deprecated_legacy" ]]; then
        while IFS= read -r -d '' file; do
            handle_file "MOVE" "$file" "deprecated file" "$PROJECT_ROOT/$BACKUP_DIR/deprecated"
            ((deprecated_removed++))
        done < <(find tests/deprecated_legacy/ -name "*.cpp" -print0 2>/dev/null)

        # Remove empty deprecated directory if executing
        if [[ "$DRY_RUN" == false ]] && [[ -d "tests/deprecated_legacy" ]]; then
            rmdir tests/deprecated_legacy/ 2>/dev/null || true
        fi
    fi

    echo
    echo "🧹 CLEANUP SUMMARY:"
    echo "=================="
    echo "Empty files removed: $empty_removed"
    echo "Redirect files removed: $redirect_removed"
    echo "Stub files moved: $stub_removed"
    echo "Deprecated files moved: $deprecated_removed"

    files_cleaned=$((empty_removed + redirect_removed + stub_removed + deprecated_removed))
}

reorganize_test_structure() {
    print_section "REORGANIZING TEST STRUCTURE"

    cd "$PROJECT_ROOT"

    print_status "Creating categorized test directory structure..."

    # Define target directory structure
    local -A test_categories=(
        ["core"]="UnifiedAudioEngine AudioLevelProcessor RealtimeScorer"
        ["audio"]="MFCCProcessor DTWComparator VoiceActivityDetector"
        ["analysis"]="PitchTracker HarmonicAnalyzer CadenceAnalyzer EnhancedAnalysisProcessor"
        ["security"]="ErrorLogger DebugLogger access-controller crypto-manager"
        ["utils"]="AudioBufferPool PerformanceProfiler memory-guard input-validator"
        ["io"]="AudioRecorder AudioPlayer OptimizedAudioIO RealTimeAudioProcessor"
    )

    # Create directory structure
    for category in "${!test_categories[@]}"; do
        local target_dir="tests/unit/$category"
        if [[ "$DRY_RUN" == false ]]; then
            mkdir -p "$target_dir"
        else
            echo "🔄 WOULD CREATE: $target_dir"
        fi
    done

    # Create integration and performance test directories
    for dir in "tests/integration" "tests/performance" "tests/tools" "tests/lib"; do
        if [[ "$DRY_RUN" == false ]]; then
            mkdir -p "$dir"
        else
            echo "🔄 WOULD CREATE: $dir"
        fi
    done

    print_status "Categorizing and moving test files..."

    # Move test files to appropriate categories
    for category in "${!test_categories[@]}"; do
        local components=(${test_categories[$category]})
        local target_dir="tests/unit/$category"

        for component in "${components[@]}"; do
            # Find test files matching this component
            while IFS= read -r -d '' file; do
                if [[ "$(basename "$file")" == *"$component"* ]]; then
                    handle_file "MOVE" "$file" "categorization: $category" "$target_dir"
                fi
            done < <(find tests/ -maxdepth 1 -name "*$component*.cpp" -print0 2>/dev/null)
        done
    done

    # Move remaining test files to appropriate locations
    echo
    echo "📁 Moving remaining test files..."

    # Integration tests
    while IFS= read -r -d '' file; do
        if [[ "$(basename "$file")" == *"integration"* ]] || [[ "$(basename "$file")" == *"Integration"* ]]; then
            handle_file "MOVE" "$file" "integration test" "tests/integration"
        fi
    done < <(find tests/ -maxdepth 1 -name "*.cpp" -print0 2>/dev/null)

    # Performance tests
    while IFS= read -r -d '' file; do
        if [[ "$(basename "$file")" == *"performance"* ]] || [[ "$(basename "$file")" == *"Performance"* ]] || [[ "$(basename "$file")" == *"benchmark"* ]]; then
            handle_file "MOVE" "$file" "performance test" "tests/performance"
        fi
    done < <(find tests/ -maxdepth 1 -name "*.cpp" -print0 2>/dev/null)
}

verify_test_organization() {
    print_section "VERIFYING TEST ORGANIZATION"

    cd "$PROJECT_ROOT"

    local total_tests=0
    local categorized_tests=0
    local missing_tests=0

    print_status "Checking categorized test structure..."

    # Check each category
    local -a categories=("core" "audio" "analysis" "security" "utils" "io")

    for category in "${categories[@]}"; do
        local category_dir="tests/unit/$category"

        if [[ -d "$category_dir" ]]; then
            local count=$(find "$category_dir" -name "*.cpp" | wc -l)
            echo "✅ $category: $count test files"
            categorized_tests=$((categorized_tests + count))
        else
            echo "❌ $category: directory missing"
        fi
    done

    # Check integration tests
    if [[ -d "tests/integration" ]]; then
        local int_count=$(find tests/integration -name "*.cpp" | wc -l)
        echo "✅ integration: $int_count test files"
        categorized_tests=$((categorized_tests + int_count))
    fi

    # Check performance tests
    if [[ -d "tests/performance" ]]; then
        local perf_count=$(find tests/performance -name "*.cpp" | wc -l)
        echo "✅ performance: $perf_count test files"
        categorized_tests=$((categorized_tests + perf_count))
    fi

    # Check for uncategorized tests
    echo
    echo "🔍 Checking for uncategorized tests..."
    while IFS= read -r -d '' file; do
        echo "⚠️  Uncategorized: $file"
        ((missing_tests++))
    done < <(find tests/ -maxdepth 1 -name "*.cpp" -print0 2>/dev/null)

    total_tests=$((categorized_tests + missing_tests))

    echo
    echo "📊 ORGANIZATION SUMMARY:"
    echo "======================="
    echo "Total test files: $total_tests"
    echo "Categorized: $categorized_tests"
    echo "Uncategorized: $missing_tests"

    if [[ $missing_tests -eq 0 ]]; then
        print_success "✅ All tests are properly organized!"
    else
        print_warning "⚠️  $missing_tests tests need categorization"
    fi

    files_verified=$total_tests
}

create_organization_report() {
    local report_file="$PROJECT_ROOT/test_management_report_$TIMESTAMP.txt"

    {
        echo "# Test Management Report - $(date)"
        echo "=================================="
        echo ""
        echo "## Summary"
        echo "- Files cleaned: $files_cleaned"
        echo "- Files reorganized: $files_reorganized"
        echo "- Files verified: $files_verified"
        echo "- Dry run mode: $DRY_RUN"
        echo ""
        echo "## Actions Taken"
        if [[ "$DRY_RUN" == true ]]; then
            echo "This was a dry run - no actual changes were made."
            echo "Run with --execute to perform the changes."
        else
            echo "Backup directory created: $BACKUP_DIR"
            echo "Test files were cleaned and reorganized."
        fi
        echo ""
        echo "## Next Steps"
        echo "1. Review the changes (if executed)"
        echo "2. Update CMakeLists.txt if needed"
        echo "3. Run tests to verify everything works"
        echo "4. Commit the reorganized structure"
    } > "$report_file"

    print_success "Report saved to: test_management_report_$TIMESTAMP.txt"
}

main() {
    local command=$(parse_arguments "$@")

    print_header "UNIFIED TEST MANAGEMENT - $(echo $command | tr '[:lower:]' '[:upper:]')"

    if [[ "$DRY_RUN" == true ]]; then
        print_warning "DRY RUN MODE - No changes will be made"
        echo "Use --execute to actually perform changes"
    fi

    echo ""

    setup_backup_directory

    case "$command" in
        "cleanup")
            cleanup_test_files
            ;;
        "reorganize")
            reorganize_test_structure
            ;;
        "verify")
            verify_test_organization
            ;;
        "all")
            cleanup_test_files
            reorganize_test_structure
            verify_test_organization
            ;;
    esac

    create_organization_report

    print_header "OPERATION COMPLETE"
    echo "📊 Summary: $files_cleaned cleaned, $files_reorganized reorganized, $files_verified verified"

    if [[ "$DRY_RUN" == true ]]; then
        echo "🔄 This was a dry run - use --execute to make actual changes"
    else
        echo "✅ Changes have been applied - backup saved to: $BACKUP_DIR"
    fi
}

# Execute main function with all arguments
main "$@"
