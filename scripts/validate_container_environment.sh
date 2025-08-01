#!/bin/bash

# =============================================================================
# Huntmaster Engine Container Environment Validation Script
# Comprehensive testing for container development environment setup
# =============================================================================

set -euo pipefail

# Colors for better output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

log() { echo -e "${BLUE}[$(date '+%H:%M:%S')] $1${NC}"; }
success() { echo -e "${GREEN}✅ $1${NC}"; }
warning() { echo -e "${YELLOW}⚠️  $1${NC}"; }
error() { echo -e "${RED}❌ $1${NC}"; }
section() { echo -e "\n${PURPLE}=== $1 ===${NC}"; }

# Test results tracking
PASSED=0
FAILED=0
WARNINGS=0

test_result() {
    local name="$1"
    local status="$2"
    local details="${3:-}"
    
    if [ "$status" = "PASS" ]; then
        success "$name"
        ((PASSED++))
    elif [ "$status" = "WARN" ]; then
        warning "$name - $details"
        ((WARNINGS++))
    else
        error "$name - $details"
        ((FAILED++))
    fi
}

# =============================================================================
# 1. CONTAINER ENVIRONMENT BASICS
# =============================================================================

section "Container Environment Basics"

# Check we're in the right container
if [ -f "/.dockerenv" ] || grep -q 'docker\|lxc' /proc/1/cgroup 2>/dev/null; then
    test_result "Container Environment" "PASS"
else
    test_result "Container Environment" "WARN" "Not detected as container"
fi

# Check workspace directory
if [ -d "/workspaces/huntmaster-engine" ] && [ -f "/workspaces/huntmaster-engine/CMakeLists.txt" ]; then
    test_result "Workspace Directory" "PASS"
else
    test_result "Workspace Directory" "FAIL" "Workspace not properly mounted"
fi

# Check user permissions
if [ -w "/workspaces/huntmaster-engine" ]; then
    test_result "Write Permissions" "PASS"
else
    test_result "Write Permissions" "FAIL" "Cannot write to workspace"
fi

# =============================================================================
# 2. SYSTEM TOOLS AND COMPILERS
# =============================================================================

section "System Tools and Compilers"

# Essential system tools
declare -A SYSTEM_TOOLS=(
    ["git"]="git --version"
    ["curl"]="curl --version"
    ["wget"]="wget --version"
    ["unzip"]="unzip -v"
    ["make"]="make --version"
    ["ninja"]="ninja --version"
    ["pkg-config"]="pkg-config --version"
)

for tool in "${!SYSTEM_TOOLS[@]}"; do
    if command -v "$tool" >/dev/null 2>&1; then
        version=$(${SYSTEM_TOOLS[$tool]} 2>&1 | head -1 || echo "unknown")
        test_result "$tool" "PASS" "$version"
    else
        test_result "$tool" "FAIL" "Not found in PATH"
    fi
done

# Compilers with version requirements
section "Compilers and Build Tools"

# GCC
if command -v gcc-13 >/dev/null 2>&1; then
    gcc_version=$(gcc-13 --version | head -1)
    test_result "gcc-13" "PASS" "$gcc_version"
else
    test_result "gcc-13" "FAIL" "GCC 13 not found"
fi

if command -v g++-13 >/dev/null 2>&1; then
    gpp_version=$(g++-13 --version | head -1)
    test_result "g++-13" "PASS" "$gpp_version"
else
    test_result "g++-13" "FAIL" "G++ 13 not found"
fi

# Clang
if command -v clang >/dev/null 2>&1; then
    clang_version=$(clang --version | head -1)
    test_result "clang" "PASS" "$clang_version"
else
    test_result "clang" "WARN" "Clang not found"
fi

# CMake with version check
if command -v cmake >/dev/null 2>&1; then
    cmake_version=$(cmake --version | head -1)
    cmake_major=$(echo "$cmake_version" | grep -o '[0-9]\+\.[0-9]\+' | head -1 | cut -d. -f1)
    cmake_minor=$(echo "$cmake_version" | grep -o '[0-9]\+\.[0-9]\+' | head -1 | cut -d. -f2)
    
    if (( cmake_major > 3 || (cmake_major == 3 && cmake_minor >= 20) )); then
        test_result "cmake" "PASS" "$cmake_version"
    else
        test_result "cmake" "WARN" "Version $cmake_version may be too old (need 3.20+)"
    fi
else
    test_result "cmake" "FAIL" "CMake not found"
fi

# =============================================================================
# 3. DEVELOPMENT TOOLS
# =============================================================================

section "Development and Debugging Tools"

declare -A DEV_TOOLS=(
    ["gdb"]="gdb --version"
    ["valgrind"]="valgrind --version"
    ["cppcheck"]="cppcheck --version"
    ["ccache"]="ccache --version"
    ["gcov"]="gcov --version"
    ["lcov"]="lcov --version"
)

for tool in "${!DEV_TOOLS[@]}"; do
    if command -v "$tool" >/dev/null 2>&1; then
        version=$(${DEV_TOOLS[$tool]} 2>&1 | head -1 || echo "unknown")
        test_result "$tool" "PASS" "$version"
    else
        test_result "$tool" "WARN" "Not found (optional but recommended)"
    fi
done

# =============================================================================
# 4. LANGUAGE RUNTIMES
# =============================================================================

section "Language Runtimes"

# Node.js and npm
if command -v node >/dev/null 2>&1; then
    node_version=$(node --version)
    test_result "node.js" "PASS" "$node_version"
else
    test_result "node.js" "FAIL" "Node.js not found"
fi

if command -v npm >/dev/null 2>&1; then
    npm_version=$(npm --version)
    test_result "npm" "PASS" "v$npm_version"
else
    test_result "npm" "FAIL" "npm not found"
fi

# Python
if command -v python3 >/dev/null 2>&1; then
    python_version=$(python3 --version)
    test_result "python3" "PASS" "$python_version"
else
    test_result "python3" "WARN" "Python3 not found"
fi

if command -v pip3 >/dev/null 2>&1; then
    pip_version=$(pip3 --version)
    test_result "pip3" "PASS" "$pip_version"
else
    test_result "pip3" "WARN" "pip3 not found"
fi

# .NET (if available)
if command -v dotnet >/dev/null 2>&1; then
    dotnet_version=$(dotnet --version)
    test_result "dotnet" "PASS" "v$dotnet_version"
else
    test_result "dotnet" "WARN" ".NET not found (optional)"
fi

# =============================================================================
# 5. VCPKG AND PACKAGE MANAGEMENT
# =============================================================================

section "Package Management"

# vcpkg
if [ -n "${VCPKG_ROOT:-}" ] && [ -d "$VCPKG_ROOT" ]; then
    if [ -x "$VCPKG_ROOT/vcpkg" ]; then
        vcpkg_version=$("$VCPKG_ROOT/vcpkg" version 2>&1 | head -1 || echo "unknown")
        test_result "vcpkg" "PASS" "$vcpkg_version"
    else
        test_result "vcpkg" "FAIL" "vcpkg binary not executable at $VCPKG_ROOT"
    fi
else
    test_result "vcpkg" "FAIL" "VCPKG_ROOT not set or directory not found"
fi

# =============================================================================
# 6. AUDIO LIBRARIES AND DEPENDENCIES
# =============================================================================

section "Audio Libraries and Dependencies"

# Check for common audio libraries
declare -A AUDIO_LIBS=(
    ["libasound2-dev"]="dpkg -l | grep libasound2-dev"
    ["libpulse-dev"]="dpkg -l | grep libpulse-dev" 
    ["libjack-dev"]="dpkg -l | grep libjack-dev"
)

for lib in "${!AUDIO_LIBS[@]}"; do
    if eval "${AUDIO_LIBS[$lib]}" >/dev/null 2>&1; then
        test_result "$lib" "PASS"
    else
        test_result "$lib" "WARN" "Audio library not found (may need for recording)"
    fi
done

# =============================================================================
# 7. PROJECT-SPECIFIC DEPENDENCIES
# =============================================================================

section "Project Dependencies"

# Check for project-required headers
if [ -f "/workspaces/huntmaster-engine/libs/dr_wav.h" ]; then
    test_result "dr_wav.h" "PASS"
else
    test_result "dr_wav.h" "FAIL" "Missing required header"
fi

if [ -f "/workspaces/huntmaster-engine/libs/miniaudio.h" ]; then
    test_result "miniaudio.h" "PASS"
else
    test_result "miniaudio.h" "FAIL" "Missing required header"
fi

if [ -d "/workspaces/huntmaster-engine/libs/kissfft" ]; then
    test_result "kissfft" "PASS"
else
    test_result "kissfft" "FAIL" "Missing required library"
fi

# =============================================================================
# 8. BUILD SYSTEM TEST
# =============================================================================

section "Build System Validation"

cd /workspaces/huntmaster-engine

# Clean any existing problematic build artifacts
log "Cleaning build directory to fix path conflicts..."
if [ -d "build" ]; then
    rm -rf build/*
    test_result "Build Clean" "PASS"
else
    mkdir -p build
    test_result "Build Directory Created" "PASS"
fi

# Test CMake configuration
log "Testing CMake configuration..."
if timeout 30 cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_C_COMPILER=gcc-13 -DUSE_CCACHE=ON 2>&1; then
    test_result "CMake Configuration" "PASS"
else
    test_result "CMake Configuration" "FAIL" "CMake configuration failed"
fi

# Test build (just a small portion)
log "Testing sample build..."
if [ -f "build/build.ninja" ]; then
    if timeout 60 cmake --build build --target TestUtils -j$(nproc) 2>&1; then
        test_result "Sample Build" "PASS"
    else
        test_result "Sample Build" "WARN" "Build issues detected"
    fi
else
    test_result "Sample Build" "FAIL" "No build file generated"
fi

# =============================================================================
# 9. ENVIRONMENT VARIABLES
# =============================================================================

section "Environment Variables"

declare -A REQUIRED_ENV=(
    ["PATH"]="Essential for finding tools"
    ["HOME"]="User home directory"
    ["USER"]="Current user"
    ["SHELL"]="Default shell"
)

declare -A OPTIONAL_ENV=(
    ["VCPKG_ROOT"]="vcpkg package manager"
    ["CCACHE_DIR"]="ccache directory"
    ["CMAKE_GENERATOR"]="Default CMake generator"
    ["CC"]="Default C compiler"
    ["CXX"]="Default C++ compiler"
)

for var in "${!REQUIRED_ENV[@]}"; do
    if [ -n "${!var:-}" ]; then
        test_result "ENV: $var" "PASS" "${!var}"
    else
        test_result "ENV: $var" "FAIL" "${REQUIRED_ENV[$var]}"
    fi
done

for var in "${!OPTIONAL_ENV[@]}"; do
    if [ -n "${!var:-}" ]; then
        test_result "ENV: $var" "PASS" "${!var}"
    else
        test_result "ENV: $var" "WARN" "${OPTIONAL_ENV[$var]} not set"
    fi
done

# =============================================================================
# 10. PERFORMANCE AND RESOURCE CHECKS
# =============================================================================

section "Performance and Resources"

# CPU cores
cpu_cores=$(nproc)
if (( cpu_cores >= 2 )); then
    test_result "CPU Cores" "PASS" "$cpu_cores cores available"
else
    test_result "CPU Cores" "WARN" "Only $cpu_cores core(s) - may be slow"
fi

# Memory
if command -v free >/dev/null 2>&1; then
    mem_gb=$(free -g | awk '/^Mem:/{print $2}')
    if (( mem_gb >= 4 )); then
        test_result "Memory" "PASS" "${mem_gb}GB available"
    else
        test_result "Memory" "WARN" "Only ${mem_gb}GB - may be insufficient"
    fi
else
    test_result "Memory Check" "WARN" "Cannot determine memory"
fi

# Disk space
disk_avail=$(df -h /workspaces 2>/dev/null | awk 'NR==2{print $4}' || echo "unknown")
test_result "Disk Space" "PASS" "$disk_avail available"

# =============================================================================
# 11. CONTAINER-SPECIFIC FEATURES
# =============================================================================

section "Container Features"

# Check if we have audio device access
if ls /dev/snd/* >/dev/null 2>&1; then
    test_result "Audio Devices" "PASS" "Audio devices accessible"
else
    test_result "Audio Devices" "WARN" "No audio devices (expected in container)"
fi

# Check X11 forwarding for GUI apps
if [ -n "${DISPLAY:-}" ]; then
    test_result "X11 Display" "PASS" "DISPLAY=$DISPLAY"
else
    test_result "X11 Display" "WARN" "No X11 display (GUI apps won't work)"
fi

# Check if we can access the host filesystem
if [ -d "/mnt" ] || [ -d "/host" ]; then
    test_result "Host Access" "PASS" "Host filesystem accessible"
else
    test_result "Host Access" "WARN" "No apparent host filesystem access"
fi

# =============================================================================
# SUMMARY
# =============================================================================

section "Test Summary"

total=$((PASSED + FAILED + WARNINGS))
echo ""
log "Environment Validation Results:"
success "$PASSED tests passed"
if [ $WARNINGS -gt 0 ]; then
    warning "$WARNINGS warnings"
fi
if [ $FAILED -gt 0 ]; then
    error "$FAILED tests failed"
fi

echo ""
if [ $FAILED -eq 0 ]; then
    success "🎉 Environment validation completed successfully!"
    success "Your container development environment is ready for Huntmaster Engine development."
    echo ""
    log "Next steps:"
    echo "  1. Run: timeout 60 cmake --build build"
    echo "  2. Run: timeout 60 ./build/bin/RunEngineTests"
    echo "  3. Check VS Code extensions are working"
else
    error "❌ Environment validation failed with $FAILED critical issues."
    echo ""
    log "Please address the failed tests before continuing development."
fi

echo ""
log "For detailed troubleshooting, see: docs/DEBUGGING.md"

exit $(( FAILED > 0 ? 1 : 0 ))
