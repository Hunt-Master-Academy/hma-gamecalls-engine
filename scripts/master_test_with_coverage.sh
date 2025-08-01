#!/bin/bash

# =============================================================================
# Huntmaster Engine Coverage Master Test - Quick Coverage Analysis
# Builds with coverage instrumentation and runs comprehensive tests
# =============================================================================

echo "🎯 HUNTMASTER ENGINE COVERAGE MASTER TEST"
echo "=========================================="
echo "Building with coverage instrumentation and running comprehensive tests..."
echo ""

# Change to project root
cd "$(dirname "$0")/.." || exit 1

echo "📦 Building with coverage instrumentation..."
export CXXFLAGS="--coverage -g -O0"
export LDFLAGS="--coverage"

# Clean and rebuild with coverage
if ! cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage -g -O0" -DCMAKE_EXE_LINKER_FLAGS="--coverage"; then
    echo "❌ Failed to configure build with coverage"
    exit 1
fi

if ! cmake --build build; then
    echo "❌ Failed to build with coverage instrumentation"
    exit 1
fi

echo "✅ Build completed with coverage instrumentation"
echo ""

echo "🧪 Running Master Test Suite with coverage analysis..."
COVERAGE_ENABLED=true ./scripts/master_test.sh

exit_code=$?

echo ""
echo "📊 Coverage analysis completed!"
echo "Check test_logs/ directory for detailed coverage reports."

if [[ $exit_code -eq 0 ]]; then
    echo "🎉 Master test with coverage analysis SUCCESSFUL!"
else
    echo "⚠️  Some issues found - check logs for details"
fi

exit $exit_code
