#!/bin/bash
# File: check_dev_environment.sh
# Comprehensive development environment checker for Huntmaster Audio Engine

echo "========================================"
echo "Huntmaster Audio Engine Environment Check"
echo "========================================"
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to check if a command exists
check_command() {
    if command -v $1 &> /dev/null; then
        echo -e "${GREEN}✓${NC} $2: $(command -v $1)"
        if [ ! -z "$3" ]; then
            version=$($3)
            echo "    Version: $version"
        fi
        return 0
    else
        echo -e "${RED}✗${NC} $2: Not found"
        return 1
    fi
}

# Function to check version meets minimum requirement
check_version() {
    local cmd=$1
    local name=$2
    local version_cmd=$3
    local min_version=$4
    
    if command -v $cmd &> /dev/null; then
        version=$($version_cmd 2>&1 | head -n1)
        echo -e "${GREEN}✓${NC} $name: $version"
        # You might want to add actual version comparison here
        return 0
    else
        echo -e "${RED}✗${NC} $name: Not found (minimum: $min_version)"
        return 1
    fi
}

echo "=== Core Build Tools ==="
check_version "cmake" "CMake" "cmake --version" "3.15"
check_command "make" "Make" "make --version | head -n1"
check_command "ninja" "Ninja (optional)" "ninja --version"

echo ""
echo "=== C++ Compilers ==="
check_version "g++" "G++" "g++ --version" "10.0"
check_version "clang++" "Clang++" "clang++ --version" "12.0"

# Check C++20 support
echo ""
echo "=== C++20 Support Check ==="
cat > /tmp/cpp20_test.cpp << 'EOF'
#include <concepts>
#include <ranges>
#include <span>
#include <expected>
#include <semaphore>
#include <latch>
#include <barrier>
#include <bit>
#include <numbers>

int main() {
    // Test C++20 features
    std::span<int> s;
    std::counting_semaphore<10> sem(10);
    
    auto even = [](int i) { return i % 2 == 0; };
    auto square = [](int i) { return i * i; };
    
    std::vector<int> v{1, 2, 3, 4, 5};
    auto result = v | std::views::filter(even) | std::views::transform(square);
    
    return 0;
}
EOF

if g++ -std=c++20 -c /tmp/cpp20_test.cpp -o /tmp/cpp20_test.o &> /dev/null; then
    echo -e "${GREEN}✓${NC} C++20 features supported"
else
    echo -e "${YELLOW}⚠${NC} C++20 features may not be fully supported"
    echo "  Some features might need C++23 or experimental flags"
fi
rm -f /tmp/cpp20_test.cpp /tmp/cpp20_test.o

echo ""
echo "=== Platform-Specific Tools ==="

# Emscripten for WASM
if command -v emcc &> /dev/null; then
    echo -e "${GREEN}✓${NC} Emscripten: $(emcc --version | head -n1)"
    check_command "emcmake" "emcmake" ""
    check_command "emmake" "emmake" ""
else
    echo -e "${YELLOW}⚠${NC} Emscripten: Not found (required for WASM builds)"
    echo "    Install: https://emscripten.org/docs/getting_started/downloads.html"
fi

# Android NDK
if [ ! -z "$ANDROID_NDK_HOME" ] || [ ! -z "$ANDROID_NDK" ]; then
    ndk_path=${ANDROID_NDK_HOME:-$ANDROID_NDK}
    echo -e "${GREEN}✓${NC} Android NDK: $ndk_path"
else
    echo -e "${YELLOW}⚠${NC} Android NDK: Not configured (required for Android builds)"
    echo "    Set ANDROID_NDK_HOME environment variable"
fi

# iOS/macOS tools (only on macOS)
if [[ "$OSTYPE" == "darwin"* ]]; then
    check_command "xcodebuild" "Xcode" "xcodebuild -version | head -n1"
    check_command "xcrun" "xcrun" ""
fi

echo ""
echo "=== Testing & Analysis Tools ==="
check_command "ctest" "CTest" "ctest --version | head -n1"
check_command "valgrind" "Valgrind (memory checker)" "valgrind --version"
check_command "gdb" "GDB (debugger)" "gdb --version | head -n1"
check_command "lldb" "LLDB (debugger)" "lldb --version"
check_command "clang-tidy" "clang-tidy (static analysis)" "clang-tidy --version"
check_command "clang-format" "clang-format (code formatter)" "clang-format --version"
check_command "cppcheck" "cppcheck (static analysis)" "cppcheck --version"

echo ""
echo "=== Performance Tools ==="
check_command "perf" "perf (Linux profiler)" "perf --version"
check_command "instruments" "Instruments (macOS profiler)" ""
check_command "heaptrack" "heaptrack (memory profiler)" "heaptrack --version"

echo ""
echo "=== Documentation Tools ==="
check_command "doxygen" "Doxygen" "doxygen --version"
check_command "dot" "Graphviz (for diagrams)" "dot -V 2>&1"

echo ""
echo "=== Python Environment (for scripts) ==="
check_version "python3" "Python 3" "python3 --version" "3.8"
check_command "pip3" "pip3" "pip3 --version"

# Check Python packages
echo "  Checking Python packages..."
python3 -c "import numpy" 2>/dev/null && echo -e "    ${GREEN}✓${NC} numpy" || echo -e "    ${YELLOW}⚠${NC} numpy (for audio analysis scripts)"
python3 -c "import matplotlib" 2>/dev/null && echo -e "    ${GREEN}✓${NC} matplotlib" || echo -e "    ${YELLOW}⚠${NC} matplotlib (for visualizations)"
python3 -c "import scipy" 2>/dev/null && echo -e "    ${GREEN}✓${NC} scipy" || echo -e "    ${YELLOW}⚠${NC} scipy (for signal processing)"

echo ""
echo "=== Git & Version Control ==="
check_command "git" "Git" "git --version"
check_command "git-lfs" "Git LFS (for large files)" "git lfs version"

echo ""
echo "=== Recommended VS Code Extensions ==="
echo "Check if you have these VS Code extensions installed:"
echo "  - C/C++ Extension Pack (ms-vscode.cpptools)"
echo "  - CMake Tools (ms-vscode.cmake-tools)"
echo "  - Clang-Format (xaver.clang-format)"
echo "  - CodeLLDB (vadimcn.vscode-lldb)"
echo "  - C++ TestMate (matepek.vscode-catch2-test-adapter)"

echo ""
echo "=== Environment Summary ==="
echo ""

# Check for specific C++ standard library features
echo "Checking C++ standard library features..."
cat > /tmp/check_features.cpp << 'EOF'
#include <iostream>
#include <version>

int main() {
    #ifdef __cpp_lib_expected
    std::cout << "huntmaster::expected: supported" << std::endl;
    #else
    std::cout << "huntmaster::expected: NOT supported (need C++23)" << std::endl;
    #endif
    
    #ifdef __cpp_lib_semaphore
    std::cout << "std::counting_semaphore: supported" << std::endl;
    #else
    std::cout << "std::counting_semaphore: NOT supported" << std::endl;
    #endif
    
    #ifdef __cpp_lib_jthread
    std::cout << "std::jthread: supported" << std::endl;
    #else
    std::cout << "std::jthread: NOT supported" << std::endl;
    #endif
    
    #ifdef __cpp_lib_ranges
    std::cout << "std::ranges: supported" << std::endl;
    #else
    std::cout << "std::ranges: NOT supported" << std::endl;
    #endif
    
    return 0;
}
EOF

if g++ -std=c++20 /tmp/check_features.cpp -o /tmp/check_features &> /dev/null; then
    /tmp/check_features
else
    echo "Could not compile feature check"
fi
rm -f /tmp/check_features.cpp /tmp/check_features

echo ""
echo "=== Recommendations ==="
echo ""
echo "For full development capability, ensure you have:"
echo "1. C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)"
echo "2. CMake 3.20+ for modern features"
echo "3. Emscripten for WASM builds"
echo "4. Android NDK for Android builds"
echo "5. Xcode for iOS/macOS builds"
echo "6. Testing tools (Valgrind, sanitizers)"
echo "7. Python 3.8+ with audio processing libraries"
echo ""
echo "Missing tools marked with ⚠ are optional but recommended."
echo "Missing tools marked with ✗ are required for core functionality."