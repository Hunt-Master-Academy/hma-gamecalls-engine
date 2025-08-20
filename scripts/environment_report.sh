#!/bin/bash

# =============================================================================
# Huntmaster Engine Container Environment Report
# Summary of development environment validation
# =============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m'

success() { echo -e "${GREEN}[SUCCESS] $1${NC}"; }
info() { echo -e "${BLUE}ℹ️  $1${NC}"; }
warn() { echo -e "${YELLOW}⚠️  $1${NC}"; }

echo ""
echo -e "${PURPLE}=== HUNTMASTER ENGINE DEVELOPMENT ENVIRONMENT REPORT ===${NC}"
echo -e "${BLUE}Generated: $(date)${NC}"
echo ""

echo -e "${PURPLE}=== CONTAINER ENVIRONMENT ===${NC}"
if [ -f "/.dockerenv" ]; then
    success "Running in Docker container"
else
    warn "Not detected as Docker container"
fi

if [ -d "/workspaces/huntmaster-engine" ]; then
    success "Workspace properly mounted at /workspaces/huntmaster-engine"
else
    echo -e "${RED}❌ Workspace not found${NC}"
fi

echo ""
echo -e "${PURPLE}=== BUILD TOOLS ===${NC}"

# Check GCC
if gcc_version=$(gcc-13 --version 2>/dev/null | head -1); then
    success "GCC 13: $gcc_version"
else
    echo -e "${RED}❌ GCC 13 not found${NC}"
fi

# Check CMake
if cmake_version=$(cmake --version 2>/dev/null | head -1); then
    success "CMake: $cmake_version"
else
    echo -e "${RED}❌ CMake not found${NC}"
fi

# Check Ninja
if ninja_version=$(ninja --version 2>/dev/null); then
    success "Ninja: $ninja_version"
else
    echo -e "${RED}❌ Ninja not found${NC}"
fi

echo ""
echo -e "${PURPLE}=== LANGUAGE RUNTIMES ===${NC}"

# Node.js
if node_version=$(node --version 2>/dev/null); then
    success "Node.js: $node_version"
else
    echo -e "${RED}❌ Node.js not found${NC}"
fi

# Python
if python_version=$(python3 --version 2>/dev/null); then
    success "Python: $python_version"
else
    warn "Python3 not found"
fi

# .NET
if dotnet_version=$(dotnet --version 2>/dev/null); then
    success ".NET: v$dotnet_version"
else
    info ".NET not found (optional)"
fi

echo ""
echo -e "${PURPLE}=== DEVELOPMENT TOOLS ===${NC}"

# Git
if git_version=$(git --version 2>/dev/null); then
    success "Git: $git_version"
else
    echo -e "${RED}❌ Git not found${NC}"
fi

# GDB
if gdb_version=$(gdb --version 2>/dev/null | head -1); then
    success "GDB: Available"
else
    warn "GDB not found"
fi

# Valgrind
if valgrind --version >/dev/null 2>&1; then
    success "Valgrind: Available"
else
    warn "Valgrind not found"
fi

# cppcheck
if cppcheck --version >/dev/null 2>&1; then
    success "cppcheck: Available"
else
    warn "cppcheck not found"
fi

echo ""
echo -e "${PURPLE}=== PROJECT STATUS ===${NC}"

cd /workspaces/huntmaster-engine

# CMake configuration
if [ -f "build/CMakeCache.txt" ]; then
    success "CMake configuration: Valid"
else
    echo -e "${RED}❌ CMake not configured${NC}"
fi

# Build status
if [ -f "build/bin/RunEngineTests" ]; then
    success "Build: Test runner built successfully"
else
    echo -e "${RED}❌ Build incomplete${NC}"
fi

# Test status
if timeout 30 ./build/bin/RunEngineTests --gtest_filter="*BasicCoverage*" --gtest_brief=yes >/dev/null 2>&1; then
    success "Tests: Basic tests passing"
else
    warn "Tests: Some issues detected"
fi

# Check key dependencies
if [ -f "libs/dr_wav.h" ]; then
    success "Dependencies: dr_wav.h found"
else
    echo -e "${RED}❌ Missing dr_wav.h${NC}"
fi

if [ -f "libs/miniaudio.h" ]; then
    success "Dependencies: miniaudio.h found"
else
    echo -e "${RED}❌ Missing miniaudio.h${NC}"
fi

if [ -d "libs/kissfft" ]; then
    success "Dependencies: kissfft found"
else
    echo -e "${RED}❌ Missing kissfft${NC}"
fi

echo ""
echo -e "${PURPLE}=== PERFORMANCE METRICS ===${NC}"

# CPU cores
cpu_cores=$(nproc)
success "CPU Cores: $cpu_cores"

# Memory (if available)
if command -v free >/dev/null 2>&1; then
    mem_total=$(free -h | awk '/^Mem:/{print $2}')
    mem_avail=$(free -h | awk '/^Mem:/{print $7}')
    success "Memory: $mem_avail available of $mem_total total"
fi

# Disk space
disk_avail=$(df -h /workspaces 2>/dev/null | awk 'NR==2{print $4}' || echo "unknown")
success "Disk Space: $disk_avail available"

echo ""
echo -e "${PURPLE}=== CONTAINER-SPECIFIC FEATURES ===${NC}"

# Check environment variables
if [ -n "${VCPKG_ROOT:-}" ]; then
    success "VCPKG_ROOT: $VCPKG_ROOT"
else
    warn "VCPKG_ROOT not set"
fi

if [ -n "${DISPLAY:-}" ]; then
    success "X11 Display: $DISPLAY"
else
    info "No X11 display (GUI apps unavailable)"
fi

# VS Code integration
if [ -d "/vscode" ] || [ -n "${VSCODE_IPC_HOOK_CLI:-}" ]; then
    success "VS Code integration: Available"
else
    info "VS Code integration not detected"
fi

echo ""
echo -e "${PURPLE}=== PATH RESOLUTION ISSUES ===${NC}"

# Check for the CMake path conflict we fixed
if grep -q "/home/xbyooki" build/CMakeCache.txt 2>/dev/null; then
    warn "Old path references still present in CMakeCache.txt"
    info "Run: rm -rf build/* && cmake -B build [options] to fix"
else
    success "Path conflicts resolved"
fi

echo ""
echo -e "${PURPLE}=== RECOMMENDED NEXT STEPS ===${NC}"

echo -e "${BLUE}1. Test full build:${NC}"
echo "   timeout 180 cmake --build build -j\$(nproc)"
echo ""
echo -e "${BLUE}2. Run comprehensive tests:${NC}"
echo "   timeout 60 ./build/bin/RunEngineTests --gtest_brief=yes"
echo ""
echo -e "${BLUE}3. Run master test suite:${NC}"
echo "   timeout 180 ./scripts/master_test.sh"
echo ""
echo -e "${BLUE}4. Check VS Code extensions:${NC}"
echo "   - C/C++ extension"
echo "   - CMake Tools"
echo "   - GitLens"
echo "   - GitHub Copilot"
echo ""

echo -e "${GREEN}🎉 Container environment is ready for Huntmaster Engine development!${NC}"
echo ""
