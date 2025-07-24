#!/bin/bash

# Simple build test script for LoadandExtractTest
echo "=== Testing LoadandExtractTest Compilation ==="

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Run cmake if CMakeCache doesn't exist
if [ ! -f "CMakeCache.txt" ]; then
    echo "Running cmake configuration..."
    cmake .. || { echo "CMake configuration failed"; exit 1; }
fi

# Build the specific test
echo "Building LoadandExtractTest..."
make LoadandExtractTest -j4

if [ $? -eq 0 ]; then
    echo "✅ LoadandExtractTest compiled successfully!"

    # Check if the executable was created
    if [ -f "bin/LoadandExtractTest" ]; then
        echo "✅ Executable created at bin/LoadandExtractTest"
        ls -la bin/LoadandExtractTest
    else
        echo "⚠️  Executable not found in expected location"
        echo "Searching for LoadandExtractTest executable..."
        find . -name "LoadandExtractTest" -type f
    fi
else
    echo "❌ LoadandExtractTest compilation failed"
    exit 1
fi
