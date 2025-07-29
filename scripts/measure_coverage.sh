#!/bin/bash

# Coverage measurement script for Huntmaster Engine
# This script will compile tests with coverage flags and generate coverage reports

echo "=== Huntmaster Engine Coverage Analysis ==="
echo "Building tests with coverage instrumentation..."

# Clean previous builds
make clean

# Build with coverage flags
export CXXFLAGS="--coverage -g -O0"
export LDFLAGS="--coverage"

# Build the core library with coverage
make UnifiedAudioEngine

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to build UnifiedAudioEngine with coverage"
    exit 1
fi

echo "Core library built successfully with coverage instrumentation"

# Build and run comprehensive tests
declare -a COMPREHENSIVE_TESTS=(
    "BasicCoverageTest"
    "ErrorHandlingTest"
    "MemoryManagementTest"
    "SessionStateTest"
    "UnifiedEngineComprehensiveTest"
    "DTWComprehensiveTest"
    "MasterCallComprehensiveTest"
)

echo "Building comprehensive test suite..."

for test in "${COMPREHENSIVE_TESTS[@]}"; do
    echo "Building $test..."
    make "$test"
    if [ $? -eq 0 ]; then
        echo "✓ $test built successfully"
        # Run the test
        echo "Running $test..."
        ./bin/"$test" --gtest_output=xml:coverage_results_"$test".xml
        if [ $? -eq 0 ]; then
            echo "✓ $test passed"
        else
            echo "✗ $test failed"
        fi
    else
        echo "✗ Failed to build $test"
    fi
done

# Generate coverage report
echo "Generating coverage report..."
gcov -r src/core/*.cpp
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/tests/*' '*/build/*' --output-file coverage_cleaned.info
genhtml coverage_cleaned.info --output-directory coverage_html

echo "Coverage report generated in coverage_html/"
echo "Open coverage_html/index.html to view detailed coverage analysis"

# Calculate coverage percentage
COVERAGE_PERCENT=$(lcov --summary coverage_cleaned.info 2>&1 | grep "lines......" | awk '{print $2}' | sed 's/%//')

echo ""
echo "=== COVERAGE SUMMARY ==="
echo "Total line coverage: $COVERAGE_PERCENT%"

if (( $(echo "$COVERAGE_PERCENT >= 90" | bc -l) )); then
    echo "✓ SUCCESS: 90% coverage target achieved!"
    exit 0
else
    echo "⚠ WARNING: Coverage ($COVERAGE_PERCENT%) below 90% target"
    echo "Additional tests may be needed to reach 90% coverage"
    exit 1
fi
