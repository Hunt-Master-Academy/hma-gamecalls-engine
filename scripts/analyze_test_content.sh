#!/bin/bash

# Test File Content Analyzer
# Identifies empty files, stub files, and files with minimal content

echo "🔍 Analyzing Test File Content Completeness"
echo "============================================="
echo

# Counters
empty_files=0
stub_files=0
minimal_files=0
redirect_files=0
deprecated_files=0
complete_files=0

# Arrays to store file lists
empty_file_list=()
stub_file_list=()
minimal_file_list=()
redirect_file_list=()
deprecated_file_list=()

# Function to analyze file content
analyze_file() {
    local file="$1"
    local line_count=$(wc -l < "$file" 2>/dev/null || echo 0)
    local content=$(head -20 "$file" 2>/dev/null || echo "")

    # Skip if it's a clearly deprecated file
    if [[ "$file" == *deprecated* ]] || [[ "$file" == *.stub.cpp ]]; then
        ((deprecated_files++))
        deprecated_file_list+=("$file")
        return
    fi

    # Check if file is empty
    if [[ $line_count -eq 0 ]]; then
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

# Find all .cpp test files and analyze them
while IFS= read -r -d '' file; do
    analyze_file "$file"
done < <(find tests/ -name "*.cpp" -print0 2>/dev/null)

# Report results
echo "📊 ANALYSIS SUMMARY"
echo "==================="
echo "Empty Files:      $empty_files"
echo "Stub Files:       $stub_files"
echo "Minimal Files:    $minimal_files"
echo "Redirect Files:   $redirect_files"
echo "Deprecated Files: $deprecated_files"
echo "Complete Files:   $complete_files"
echo

# Detail reports
if [[ $empty_files -gt 0 ]]; then
    echo "❌ EMPTY FILES ($empty_files):"
    printf "   %s\n" "${empty_file_list[@]}"
    echo
fi

if [[ $stub_files -gt 0 ]]; then
    echo "⚠️  STUB FILES ($stub_files):"
    printf "   %s\n" "${stub_file_list[@]}"
    echo
fi

if [[ $minimal_files -gt 0 ]]; then
    echo "⚠️  MINIMAL CONTENT FILES ($minimal_files):"
    printf "   %s\n" "${minimal_file_list[@]}"
    echo
fi

if [[ $redirect_files -gt 0 ]]; then
    echo "🔀 REDIRECT/MOVED FILES ($redirect_files):"
    printf "   %s\n" "${redirect_file_list[@]}"
    echo
fi

if [[ $deprecated_files -gt 0 ]]; then
    echo "🗂️  DEPRECATED/LEGACY FILES ($deprecated_files):"
    printf "   %s\n" "${deprecated_file_list[@]}"
    echo
fi

echo "✅ COMPLETE TEST FILES: $complete_files"
echo

# Calculate percentages
total_active_files=$((empty_files + stub_files + minimal_files + redirect_files + complete_files))
if [[ $total_active_files -gt 0 ]]; then
    complete_percentage=$((complete_files * 100 / total_active_files))
    echo "📈 COMPLETION RATE: $complete_percentage% ($complete_files/$total_active_files active files)"
else
    echo "📈 COMPLETION RATE: Unable to calculate"
fi

# Recommendations
echo
echo "🛠️  RECOMMENDATIONS:"
echo "==================="

problem_files=$((empty_files + stub_files + minimal_files))
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

if [[ $deprecated_files -gt 0 ]]; then
    echo "🗂️  Review deprecated files for archival or removal"
fi

echo
echo "🎯 NEXT ACTIONS:"
echo "==============="
echo "1. Review empty files for removal or implementation"
echo "2. Complete stub files with actual test implementations"
echo "3. Expand minimal files to comprehensive test coverage"
echo "4. Clean up redirect files that are no longer needed"
echo "5. Archive or remove deprecated files"
