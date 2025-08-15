#!/bin/bash

# Test File Cleanup and Consolidation Script
# Safely removes stub files, empty files, and consolidates redirects

echo "🧹 Test File Cleanup and Consolidation"
echo "======================================"
echo

# Configuration
DRY_RUN=${1:-"--dry-run"}  # Default to dry run unless --execute is passed
BACKUP_DIR="archive/test_cleanup_$(date +%Y%m%d_%H%M%S)"

# Create backup directory if executing
if [[ "$DRY_RUN" == "--execute" ]]; then
    mkdir -p "$BACKUP_DIR"
    echo "📁 Created backup directory: $BACKUP_DIR"
    echo
fi

# Function to safely remove or move file
handle_file() {
    local action="$1"
    local file="$2"
    local reason="$3"

    if [[ "$DRY_RUN" == "--dry-run" ]]; then
        echo "🔄 WOULD $action: $file ($reason)"
    else
        case "$action" in
            "REMOVE")
                if [[ -f "$file" ]]; then
                    # Backup before removal
                    cp "$file" "$BACKUP_DIR/$(basename "$file").bak"
                    rm "$file"
                    echo "✅ REMOVED: $file ($reason)"
                fi
                ;;
            "MOVE")
                if [[ -f "$file" ]]; then
                    # Move to archive
                    mv "$file" "$BACKUP_DIR/"
                    echo "✅ MOVED: $file to $BACKUP_DIR/ ($reason)"
                fi
                ;;
        esac
    fi
}

# Function to update CMakeLists.txt
update_cmake() {
    local file_to_remove="$1"
    local cmake_file="tests/CMakeLists.txt"

    if [[ "$DRY_RUN" == "--dry-run" ]]; then
        echo "🔄 WOULD UPDATE: Remove $(basename "$file_to_remove" .cpp) from $cmake_file"
    else
        # Remove references from CMakeLists.txt
        local test_name=$(basename "$file_to_remove" .cpp)
        if grep -q "$test_name" "$cmake_file"; then
            sed -i "/$test_name/d" "$cmake_file"
            echo "✅ UPDATED: Removed $test_name from $cmake_file"
        fi
    fi
}

echo "🔍 ANALYZING FILES FOR CLEANUP"
echo "=============================="

# Counters
empty_removed=0
stub_removed=0
redirect_removed=0
minimal_consolidated=0

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
        update_cmake "$file"
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

    if [[ -f "$file" ]]; then
        content=$(head -5 "$file" 2>/dev/null || echo "")
        if echo "$content" | grep -qi "MOVED\|moved\|See.*\.cpp\|replaced by"; then
            handle_file "REMOVE" "$file" "redirect/moved file"
            update_cmake "$file"
            ((redirect_removed++))
        fi
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

    if [[ -f "$file" ]]; then
        content=$(head -20 "$file" 2>/dev/null || echo "")
        if echo "$content" | grep -qi "TODO\|FIXME\|STUB\|NOT IMPLEMENTED\|PLACEHOLDER"; then
            handle_file "REMOVE" "$file" "stub file with TODO/FIXME"
            update_cmake "$file"
            ((stub_removed++))
        fi
    fi
done < <(find tests/ -name "*.cpp" -print0 2>/dev/null)

# Find duplicate test files (same functionality in different locations)
echo
echo "🔍 Processing Potential Duplicates:"
declare -A test_names
while IFS= read -r -d '' file; do
    # Skip deprecated directory
    if [[ "$file" == *deprecated_legacy* ]]; then
        continue
    fi

    test_name=$(basename "$file" .cpp)
    # Remove common prefixes/suffixes that might indicate duplicates
    normalized_name=$(echo "$test_name" | sed 's/_comprehensive\|_fixed\|_targeted\|_coverage//g')

    if [[ -n "${test_names[$normalized_name]}" ]]; then
        # Found potential duplicate
        existing="${test_names[$normalized_name]}"
        current="$file"

        # Prefer files in more specific directories (analyzers/ over unit/)
        if [[ "$current" == *analyzers/* ]] && [[ "$existing" != *analyzers/* ]]; then
            handle_file "REMOVE" "$existing" "duplicate - keeping analyzers/ version"
            update_cmake "$existing"
            test_names[$normalized_name]="$current"
        elif [[ "$existing" == *analyzers/* ]] && [[ "$current" != *analyzers/* ]]; then
            handle_file "REMOVE" "$current" "duplicate - keeping analyzers/ version"
            update_cmake "$current"
        fi
    else
        test_names[$normalized_name]="$file"
    fi
done < <(find tests/unit -name "*.cpp" -print0 2>/dev/null)

echo
echo "📊 CLEANUP SUMMARY"
echo "=================="
echo "Empty Files Removed:     $empty_removed"
echo "Redirect Files Removed:  $redirect_removed"
echo "Stub Files Removed:      $stub_removed"
echo "Duplicates Consolidated: $minimal_consolidated"
echo

total_changes=$((empty_removed + redirect_removed + stub_removed + minimal_consolidated))

if [[ "$DRY_RUN" == "--dry-run" ]]; then
    echo "🔄 DRY RUN MODE: No files were actually modified"
    echo "📝 To execute these changes, run: $0 --execute"
    echo "⚠️  CAUTION: This will modify your test suite!"
else
    echo "✅ EXECUTED: $total_changes changes made"
    echo "💾 Backup created in: $BACKUP_DIR"
fi

echo
echo "🎯 NEXT STEPS"
echo "============"
echo "1. 🔨 Rebuild the project to verify CMakeLists.txt changes"
echo "2. 🧪 Run the test suite to ensure no regressions"
echo "3. 📊 Re-run test completeness validation"
echo "4. 📝 Review remaining test files for content quality"

if [[ "$DRY_RUN" == "--dry-run" ]]; then
    echo
    echo "💡 TIP: Review the proposed changes above, then run with --execute if satisfied"
fi
