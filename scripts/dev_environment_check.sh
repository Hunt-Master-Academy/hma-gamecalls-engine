#!/bin/bash

# Development Environment Health Check for Huntmaster Engine Container
# This script validates that all tools and extensions are properly configured

set -e

echo "=== HUNTMASTER ENGINE DEVELOPMENT ENVIRONMENT CHECK ==="
echo "Date: $(date)"
echo "Container: $(uname -a)"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

function check_status() {
    if [ $1 -eq 0 ]; then
        echo -e "  ${GREEN}✓${NC} $2"
    else
        echo -e "  ${RED}✗${NC} $2"
        return 1
    fi
}

function check_warning() {
    echo -e "  ${YELLOW}⚠${NC} $1"
}

# 1. System Tools
echo "1. SYSTEM TOOLS"
echo "==============="

# Check essential build tools
check_status $? "Container environment: $(uname -a)"

gcc --version > /dev/null 2>&1
check_status $? "GCC: $(gcc --version | head -n1)"

g++ --version > /dev/null 2>&1
check_status $? "G++: $(g++ --version | head -n1)"

cmake --version > /dev/null 2>&1
check_status $? "CMake: $(cmake --version | head -n1)"

ninja --version > /dev/null 2>&1
check_status $? "Ninja: $(ninja --version)"

git --version > /dev/null 2>&1
check_status $? "Git: $(git --version)"

node --version > /dev/null 2>&1
check_status $? "Node.js: $(node --version)"

python3 --version > /dev/null 2>&1
check_status $? "Python: $(python3 --version)"

echo

# 2. C++ Development
echo "2. C++ DEVELOPMENT ENVIRONMENT"
echo "=============================="

# Check compiler features
echo 'int main() { return 0; }' | g++ -std=c++20 -x c++ - -o /tmp/cpp20_test 2>/dev/null
check_status $? "C++20 support"
rm -f /tmp/cpp20_test

# Check pkg-config
pkg-config --version > /dev/null 2>&1
check_status $? "pkg-config: $(pkg-config --version)"

# Check vcpkg
if [ -d "/usr/local/vcpkg" ]; then
    check_status 0 "vcpkg available at /usr/local/vcpkg"
else
    check_status 1 "vcpkg not found"
fi

echo

# 3. Project Build System
echo "3. PROJECT BUILD SYSTEM"
echo "======================="

cd /workspaces/huntmaster-engine

# Check CMake configuration
if [ -f "CMakeLists.txt" ]; then
    check_status 0 "Root CMakeLists.txt exists"
else
    check_status 1 "Root CMakeLists.txt missing"
fi

if [ -f "CMakePresets.json" ]; then
    check_status 0 "CMakePresets.json exists"
else
    check_status 1 "CMakePresets.json missing"
fi

# Check if build directory is clean
if [ -d "build" ]; then
    if [ -f "build/CMakeCache.txt" ]; then
        # Check if paths match container environment
        BUILD_SOURCE=$(grep "CMAKE_HOME_DIRECTORY" build/CMakeCache.txt | cut -d'=' -f2 2>/dev/null || echo "")
        if [[ "$BUILD_SOURCE" == "/workspaces/huntmaster-engine" ]]; then
            check_status 0 "Build cache has correct container paths"
        else
            check_warning "Build cache has non-container paths: $BUILD_SOURCE"
            echo "    Run: rm -rf build && cmake -B build -G Ninja"
        fi
    else
        check_warning "Build directory exists but not configured"
    fi
else
    check_warning "Build directory doesn't exist - will be created on first build"
fi

echo

# 4. Dependencies & Libraries
echo "4. DEPENDENCIES & LIBRARIES"
echo "==========================="

# Check for key header files
if [ -f "libs/dr_wav.h" ]; then
    check_status 0 "dr_wav.h audio library"
else
    check_status 1 "dr_wav.h audio library missing"
fi

if [ -f "libs/miniaudio.h" ]; then
    check_status 0 "miniaudio.h library"
else
    check_status 1 "miniaudio.h library missing"
fi

if [ -d "libs/kissfft" ]; then
    check_status 0 "KissFFT library directory"
else
    check_status 1 "KissFFT library directory missing"
fi

echo

# 5. Test Infrastructure
echo "5. TEST INFRASTRUCTURE"
echo "====================="

# Check if tests can be built
if [ -f "build/bin/RunEngineTests" ]; then
    check_status 0 "Test executable exists"
    
    # Quick test run
    timeout 30 ./build/bin/RunEngineTests --gtest_list_tests > /dev/null 2>&1
    check_status $? "Test executable runs and lists tests"
else
    check_warning "Test executable not found - run 'cmake --build build --target RunEngineTests'"
fi

echo

# 6. Container Environment
echo "6. CONTAINER ENVIRONMENT"
echo "======================="

# Check environment variables
if [ ! -z "$REMOTE_CONTAINERS" ]; then
    check_status 0 "Running in Remote Container: $REMOTE_CONTAINERS"
else
    check_warning "REMOTE_CONTAINERS variable not set"
fi

if [ ! -z "$VSCODE_CWD" ]; then
    check_status 0 "VS Code working directory: $VSCODE_CWD"
else
    check_warning "VSCODE_CWD not set"
fi

# Check workspace mount
if [ -d "/workspaces/huntmaster-engine" ]; then
    check_status 0 "Workspace properly mounted"
else
    check_status 1 "Workspace mount issue"
fi

# Check VS Code server
if pgrep -f "vscode-server" > /dev/null; then
    check_status 0 "VS Code server running"
else
    check_warning "VS Code server not detected"
fi

echo

# 7. Container Cache Directories
echo "7. CONTAINER CACHE DIRECTORIES"
echo "=============================="

# Check .gitignore for container-specific entries
if grep -q ".cache/" .gitignore 2>/dev/null; then
    check_status 0 ".cache/ in .gitignore"
else
    check_warning ".cache/ not in .gitignore"
fi

if grep -q ".ccache/" .gitignore 2>/dev/null; then
    check_status 0 ".ccache/ in .gitignore"
else
    check_warning ".ccache/ not in .gitignore"
fi

if grep -q ".vscode-server/" .gitignore 2>/dev/null; then
    check_status 0 ".vscode-server/ in .gitignore"
else
    check_warning ".vscode-server/ not in .gitignore"
fi

echo

# 8. Build Performance
echo "8. BUILD PERFORMANCE"
echo "==================="

# Check ccache if available
if command -v ccache > /dev/null 2>&1; then
    check_status 0 "ccache available: $(ccache --version | head -n1)"
    ccache -s 2>/dev/null | head -n5 | sed 's/^/    /'
else
    check_warning "ccache not available - compilation will be slower"
fi

# Check processor count
PROC_COUNT=$(nproc)
check_status 0 "Available processors: $PROC_COUNT"

echo

# 9. Common Issues Check
echo "9. COMMON ISSUES CHECK"
echo "====================="

# Check for path conflicts
if [ -f "build/CMakeCache.txt" ]; then
    if grep -q "/home/" build/CMakeCache.txt 2>/dev/null; then
        check_warning "CMake cache contains /home/ paths - clean build recommended"
        echo "    Run: rm -rf build && cmake -B build -G Ninja"
    fi
fi

# Check for large cache directories
CACHE_SIZE=$(du -sh .cache 2>/dev/null | cut -f1 || echo "0")
if [[ "$CACHE_SIZE" != "0" ]] && [[ "$CACHE_SIZE" != "4.0K" ]]; then
    check_warning "Cache directory size: $CACHE_SIZE"
fi

echo

# 10. Quick Build Test
echo "10. QUICK BUILD TEST"
echo "=================="

if [ ! -f "build/bin/RunEngineTests" ]; then
    echo "    Building test executable..."
    if timeout 300 cmake --build build --target RunEngineTests -j$(nproc) > /tmp/build_test.log 2>&1; then
        check_status 0 "Test build successful"
    else
        check_status 1 "Test build failed - see /tmp/build_test.log"
        echo "    Last few lines of build log:"
        tail -n 10 /tmp/build_test.log | sed 's/^/      /'
    fi
else
    check_status 0 "Test executable already built"
fi

echo

# Summary
echo "=== SUMMARY ==="
echo "Environment check completed at $(date)"
echo
echo "Next steps if issues found:"
echo "  1. Clean build cache: rm -rf build && cmake -B build -G Ninja"
echo "  2. Rebuild tests: cmake --build build --target RunEngineTests -j$(nproc)"
echo "  3. Run quick test: timeout 60 ./build/bin/RunEngineTests --gtest_brief=yes"
echo
echo "For VS Code extension issues:"
echo "  1. Reload window: Ctrl+Shift+P -> 'Developer: Reload Window'"
echo "  2. Restart extension host: Ctrl+Shift+P -> 'Developer: Restart Extension Host'"
echo

exit 0
