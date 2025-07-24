#!/bin/bash
# Debug Test Runner - Enables debug logging for tests

# Set the debug level (NONE=0, ERROR=1, WARN=2, INFO=3, DEBUG=4, TRACE=5)
DEBUG_LEVEL=${1:-4}  # Default to DEBUG level

# Set the test filter (default to all tests)
TEST_FILTER=${2:-"*"}

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

# Build the project
echo "Building project..."
cd "$BUILD_DIR"
make RunEngineTests

if [ $? -eq 0 ]; then
    echo "Build successful. Running tests with debug level $DEBUG_LEVEL..."

    # Run the tests with debug environment
    timeout 60 ./RunEngineTests --gtest_filter="$TEST_FILTER" --debug-level=$DEBUG_LEVEL
else
    echo "Build failed!"
    exit 1
fi
