#!/bin/bash

# Comprehensive Test Coverage Analysis for Huntmaster Engine
# Target: Achieve and validate >90% test coverage

echo "========================================================"
echo "    HUNTMASTER ENGINE COMPREHENSIVE COVERAGE ANALYSIS"
echo "========================================================"
echo "Date: $(date)"
echo "Target: >90% test coverage with robust debugging"
echo ""

PROJECT_ROOT="/workspaces/huntmaster-engine"
BUILD_DIR="build-coverage"
COVERAGE_DIR="$PROJECT_ROOT/coverage_reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Create coverage output directory
mkdir -p "$COVERAGE_DIR"

cd "$PROJECT_ROOT/$BUILD_DIR" || exit 1

echo "🎯 STEP 1: COMPREHENSIVE TEST EXECUTION"
echo "================================================"

# Define comprehensive test suite for maximum coverage
declare -a COVERAGE_TESTS=(
    "BasicCoverageTest"
    "CoverageOptimizerTest"
    "UnifiedEngineComprehensiveTest"
    "ErrorHandlingTest"
    "MemoryManagementTest"
    "SessionStateTest"
    "DTWComprehensiveTest"
    "MFCCConsistencyUnifiedTest"
    "DebugTest"
    "EndToEndTest"
    "CrossPlatformTest"
    "EnhancedAnalyzersIntegrationTest"
    "EnhancedAnalyzersPerformanceTest"
)

echo "Running comprehensive test suite for maximum coverage..."
executed_tests=0
successful_tests=0

for test in "${COVERAGE_TESTS[@]}"; do
    if [[ -f "./bin/$test" ]]; then
        echo "Executing: $test"
        if timeout 120 "./bin/$test" --gtest_brief=yes >/dev/null 2>&1; then
            echo "✅ $test - SUCCESS"
            ((successful_tests++))
        else
            echo "❌ $test - FAILED (continuing for coverage)"
        fi
        ((executed_tests++))
    else
        echo "⏭️  $test - not available"
    fi
done

echo ""
echo "Test execution summary: $successful_tests/$executed_tests successful"
echo ""

echo "🔍 STEP 2: COVERAGE DATA ANALYSIS"
echo "================================================"

# Count instrumentation and data files
gcno_count=$(find . -name "*.gcno" | wc -l)
gcda_count=$(find . -name "*.gcda" | wc -l)

echo "Coverage instrumentation files (.gcno): $gcno_count"
echo "Coverage data files (.gcda): $gcda_count"
echo "Coverage data generation: $(( gcda_count * 100 / gcno_count ))%"

echo ""

echo "📊 STEP 3: GENERATE COVERAGE REPORTS"
echo "================================================"

# Generate coverage reports for key source files
echo "Analyzing coverage for core source files..."

declare -a CORE_FILES=(
    "UnifiedAudioEngine.cpp"
    "MFCCProcessor.cpp"
    "VoiceActivityDetector.cpp"
    "ErrorLogger.cpp"
    "PitchTracker.cpp"
    "HarmonicAnalyzer.cpp"
    "CadenceAnalyzer.cpp"
    "EnhancedAnalysisProcessor.cpp"
)

total_lines=0
covered_lines=0
total_files=0

echo "Per-file coverage analysis:" | tee "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
echo "=========================" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"

for source_file in "${CORE_FILES[@]}"; do
    # Find corresponding .gcno file
    gcno_file=$(find . -name "${source_file%.cpp}.cpp.gcno" 2>/dev/null | head -1)

    if [[ -n "$gcno_file" ]]; then
        # Generate coverage report
        gcov_output=$(gcov -n "$gcno_file" 2>/dev/null || echo "No coverage data")

        if [[ "$gcov_output" != "No coverage data" ]]; then
            # Extract coverage information
            coverage_line=$(echo "$gcov_output" | grep "Lines executed:" | head -1)

            if [[ -n "$coverage_line" ]]; then
                # Parse coverage percentage
                percent=$(echo "$coverage_line" | sed 's/.*executed:\([0-9.]*\)%.*/\1/')
                lines_info=$(echo "$coverage_line" | sed 's/.*executed:[0-9.]*% of \([0-9]*\).*/\1/')
                covered=$(echo "$coverage_line" | sed 's/.*executed:\([0-9.]*\)% of \([0-9]*\).*/\1 \2/' | awk '{printf "%.0f", $1 * $2 / 100}')

                echo "$source_file: ${percent}% (${covered}/${lines_info} lines)" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"

                # Add to totals
                total_lines=$((total_lines + lines_info))
                covered_lines=$((covered_lines + covered))
                ((total_files++))
            else
                echo "$source_file: No coverage data available" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
            fi
        else
            echo "$source_file: No instrumentation found" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
        fi
    else
        echo "$source_file: Not found in build" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    fi
done

echo "" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"

# Calculate overall coverage
if [[ $total_lines -gt 0 ]]; then
    overall_coverage=$(( (covered_lines * 100) / total_lines ))
    echo "OVERALL COVERAGE SUMMARY:" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    echo "========================" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    echo "Total Files Analyzed: $total_files" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    echo "Total Lines: $total_lines" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    echo "Covered Lines: $covered_lines" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    echo "Overall Coverage: ${overall_coverage}%" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"

    if [[ $overall_coverage -ge 90 ]]; then
        echo "🎉 COVERAGE TARGET ACHIEVED! ${overall_coverage}% >= 90%" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    else
        echo "📈 Progress toward 90% target: ${overall_coverage}%" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
        gap=$((90 - overall_coverage))
        echo "Coverage gap to close: ${gap}%" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    fi
else
    echo "❌ Unable to calculate coverage - no data available" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
fi

echo ""

echo "🛠️ STEP 4: DEBUGGING AND ROBUSTNESS ASSESSMENT"
echo "================================================"

echo "Test robustness indicators:" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
echo ""

# Analyze test types for robustness
declare -a ROBUSTNESS_INDICATORS=(
    "Error handling tests: ErrorHandlingTest"
    "Memory management: MemoryManagementTest"
    "Concurrent access: SessionStateTest"
    "Edge cases: CoverageOptimizerTest"
    "Integration: EndToEndTest"
    "Performance: EnhancedAnalyzersPerformanceTest"
    "Real-world data: EnhancedAnalyzersIntegrationTest"
    "Cross-platform: CrossPlatformTest"
)

for indicator in "${ROBUSTNESS_INDICATORS[@]}"; do
    test_name=$(echo "$indicator" | cut -d':' -f2 | tr -d ' ')
    if [[ -f "./bin/$test_name" ]]; then
        echo "✅ $indicator" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    else
        echo "❌ $indicator" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    fi
done

echo ""

echo "🧪 STEP 5: TEST SUITE COMPREHENSIVENESS"
echo "================================================"

# Count total test files and cases
echo "Test suite analysis:" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"

unit_test_count=$(find "$PROJECT_ROOT/tests/unit" -name "*.cpp" | wc -l)
integration_test_count=$(find "$PROJECT_ROOT/tests/integration" -name "*.cpp" | wc -l)
total_test_files=$((unit_test_count + integration_test_count))

echo "Unit test files: $unit_test_count" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
echo "Integration test files: $integration_test_count" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
echo "Total test files: $total_test_files" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"

# Estimate test case count
total_test_cases=$(find "$PROJECT_ROOT/tests" -name "*.cpp" -exec grep -l "TEST_F\|TEST(" {} \; | wc -l)
echo "Estimated test cases: $total_test_cases" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"

echo ""

echo "📋 STEP 6: RECOMMENDATIONS FOR >90% COVERAGE"
echo "================================================"

echo "Recommendations for achieving >90% coverage:" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"

if [[ $overall_coverage -lt 90 ]]; then
    echo "1. 🎯 Focus on low-coverage files identified above" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    echo "2. 🧪 Add more edge case testing" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    echo "3. 🔄 Test error recovery paths" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    echo "4. 🏗️ Add branch coverage for conditional statements" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    echo "5. 🔧 Test configuration parameter combinations" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
else
    echo "1. ✅ Maintain current high coverage level" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    echo "2. 🔍 Focus on code quality and test effectiveness" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
    echo "3. 📊 Monitor coverage regression in future changes" | tee -a "$COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
fi

echo ""

echo "🎯 DEBUGGING CAPABILITY ASSESSMENT"
echo "================================================"

# Check for debugging features
debugging_features=0

if [[ -f "./bin/DebugTest" ]]; then
    echo "✅ Debug logging tests available"
    ((debugging_features++))
fi

if grep -r "DebugLogger" "$PROJECT_ROOT/src" >/dev/null 2>&1; then
    echo "✅ Debug logging system implemented"
    ((debugging_features++))
fi

if grep -r "ERROR\|WARN\|INFO\|DEBUG" "$PROJECT_ROOT/src" >/dev/null 2>&1; then
    echo "✅ Comprehensive logging implemented"
    ((debugging_features++))
fi

if find "$PROJECT_ROOT/tests" -name "*error*" -o -name "*debug*" | head -1 >/dev/null; then
    echo "✅ Error handling tests present"
    ((debugging_features++))
fi

if [[ -f "$PROJECT_ROOT/src/core/ErrorLogger.cpp" ]]; then
    echo "✅ Dedicated error logging component"
    ((debugging_features++))
fi

echo ""
echo "Debugging capability score: $debugging_features/5"

if [[ $debugging_features -ge 4 ]]; then
    echo "🎉 Excellent debugging support!"
elif [[ $debugging_features -ge 3 ]]; then
    echo "✅ Good debugging support"
else
    echo "⚠️  Debugging support needs improvement"
fi

echo ""

echo "========================================================"
echo "    COVERAGE ANALYSIS COMPLETE"
echo "========================================================"
echo "📄 Detailed report saved to: $COVERAGE_DIR/coverage_summary_$TIMESTAMP.txt"
echo "📊 Test execution summary: $successful_tests/$executed_tests tests passed"

if [[ $overall_coverage -ge 90 ]]; then
    echo "🎯 SUCCESS: Coverage target achieved (${overall_coverage}% >= 90%)"
    exit 0
else
    echo "📈 Progress: ${overall_coverage}% coverage (target: 90%)"
    echo "Next: Focus on low-coverage files and edge cases"
    exit 1
fi
