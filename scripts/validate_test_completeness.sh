#!/bin/bash

# Comprehensive Test File Validator
# Validates test files for actual meaningful content and functionality

echo "🧪 Comprehensive Test File Validation"
echo "====================================="
echo

# Function to validate test file content
validate_test_file() {
    local file="$1"
    local filename=$(basename "$file")
    local content=$(cat "$file" 2>/dev/null || echo "")
    local line_count=$(echo "$content" | wc -l)

    # Skip deprecated files
    if [[ "$file" == *deprecated* ]] || [[ "$file" == *.stub.cpp ]]; then
        return 0
    fi

    # Check if file is empty
    if [[ $line_count -eq 0 ]]; then
        echo "❌ EMPTY: $file"
        return 1
    fi

    # Check for redirect/moved comments
    if echo "$content" | grep -qi "MOVED\|moved\|See.*\.cpp\|replaced by"; then
        echo "🔀 REDIRECT: $file"
        return 1
    fi

    # Check for stub patterns
    if echo "$content" | grep -qi "TODO\|FIXME\|STUB\|NOT IMPLEMENTED\|PLACEHOLDER"; then
        echo "⚠️  STUB: $file"
        return 1
    fi

    # Check for minimal test requirements
    local has_includes=$(echo "$content" | grep -q "#include.*gtest\|#include.*gmock\|#include.*TestUtils" && echo 1 || echo 0)
    local has_test_case=$(echo "$content" | grep -q "TEST\|TEST_F\|TEST_P" && echo 1 || echo 0)
    local has_assertions=$(echo "$content" | grep -q "ASSERT_\|EXPECT_" && echo 1 || echo 0)
    local has_namespace=$(echo "$content" | grep -q "namespace\|using namespace" && echo 1 || echo 0)

    # Score the file
    local score=0
    [[ $has_includes -eq 1 ]] && ((score++))
    [[ $has_test_case -eq 1 ]] && ((score++))
    [[ $has_assertions -eq 1 ]] && ((score++))
    [[ $has_namespace -eq 1 ]] && ((score++))

    # Detailed analysis
    if [[ $score -eq 4 ]]; then
        echo "✅ COMPLETE: $file (score: $score/4)"
        return 0
    elif [[ $score -ge 2 ]]; then
        echo "⚠️  PARTIAL: $file (score: $score/4)"
        [[ $has_includes -eq 0 ]] && echo "    Missing: Test includes"
        [[ $has_test_case -eq 0 ]] && echo "    Missing: Test cases"
        [[ $has_assertions -eq 0 ]] && echo "    Missing: Assertions"
        [[ $has_namespace -eq 0 ]] && echo "    Missing: Namespace usage"
        return 1
    else
        echo "❌ INCOMPLETE: $file (score: $score/4, lines: $line_count)"
        return 1
    fi
}

# Function to check if file is actually compiled and linked
check_compilation() {
    local file="$1"
    local relative_path=$(realpath --relative-to="/home/xbyooki/projects/huntmaster-engine" "$file")

    # Check if file is referenced in CMakeLists.txt
    if grep -q "$(basename "$file" .cpp)" tests/CMakeLists.txt 2>/dev/null; then
        return 0
    else
        echo "🚫 NOT COMPILED: $file (not in CMakeLists.txt)"
        return 1
    fi
}

# Counters
total_files=0
complete_files=0
partial_files=0
problem_files=0
compiled_files=0

echo "📋 DETAILED FILE ANALYSIS"
echo "========================="

# Analyze all test files
while IFS= read -r -d '' file; do
    ((total_files++))

    if validate_test_file "$file"; then
        ((complete_files++))
        if check_compilation "$file"; then
            ((compiled_files++))
        fi
    else
        case "$(validate_test_file "$file" | cut -d' ' -f1)" in
            "⚠️")
                ((partial_files++))
                ;;
            *)
                ((problem_files++))
                ;;
        esac
    fi
done < <(find tests/ -name "*.cpp" -not -path "*/deprecated_legacy/*" -not -name "*.stub.cpp" -print0 2>/dev/null)

echo
echo "📊 VALIDATION SUMMARY"
echo "===================="
echo "Total Test Files:    $total_files"
echo "Complete Files:      $complete_files"
echo "Partial Files:       $partial_files"
echo "Problem Files:       $problem_files"
echo "Compiled Files:      $compiled_files"
echo

# Calculate percentages
if [[ $total_files -gt 0 ]]; then
    complete_percentage=$((complete_files * 100 / total_files))
    compiled_percentage=$((compiled_files * 100 / total_files))
    echo "📈 COMPLETION RATE: $complete_percentage% ($complete_files/$total_files)"
    echo "🔨 COMPILATION RATE: $compiled_percentage% ($compiled_files/$total_files)"
else
    echo "📈 No files to analyze"
fi

echo
echo "🎯 QUALITY GATES"
echo "================"

# Quality gates
if [[ $complete_percentage -ge 80 ]]; then
    echo "✅ COMPLETION: PASSING ($complete_percentage% >= 80%)"
else
    echo "❌ COMPLETION: FAILING ($complete_percentage% < 80%)"
fi

if [[ $compiled_percentage -ge 90 ]]; then
    echo "✅ COMPILATION: PASSING ($compiled_percentage% >= 90%)"
else
    echo "❌ COMPILATION: FAILING ($compiled_percentage% < 90%)"
fi

if [[ $problem_files -eq 0 ]]; then
    echo "✅ NO PROBLEM FILES: PASSING"
else
    echo "❌ PROBLEM FILES: FAILING ($problem_files files need attention)"
fi

echo
echo "🛠️  ACTION PLAN"
echo "==============="

if [[ $problem_files -gt 0 ]]; then
    echo "1. 🧹 CLEANUP: Remove or fix $problem_files problem files"
fi

if [[ $partial_files -gt 0 ]]; then
    echo "2. 📝 COMPLETE: Enhance $partial_files partial files"
fi

missing_compiled=$((complete_files - compiled_files))
if [[ $missing_compiled -gt 0 ]]; then
    echo "3. 🔨 COMPILE: Add $missing_compiled complete files to build system"
fi

echo "4. ✅ VALIDATE: Run test suite to ensure functionality"
echo "5. 📊 MONITOR: Track completion rate improvements"
