#!/bin/bash

# ==============================================================================
# Test Build Script for Huntmaster Audio Engine
# ==============================================================================
# This script performs a quick test build to verify the build system is working
#
# Usage: ./test_build.sh [--clean] [--debug] [--target=<target>]
# ==============================================================================

set -e

# Configuration
BUILD_TYPE="Release"
CLEAN_BUILD=false
TARGET="all"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --target=*)
            TARGET="${1#*=}"
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [--clean] [--debug] [--target=<target>] [--help]"
            echo ""
            echo "Options:"
            echo "  --clean          Clean build directory before building"
            echo "  --debug          Build in Debug mode (default: Release)"
            echo "  --target=<name>  Specific target to build (default: all)"
            echo "  --help           Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}Huntmaster Engine - Test Build${NC}"
echo "=============================="
echo "Build type: $BUILD_TYPE"
echo "Target: $TARGET"
echo "Project root: $PROJECT_ROOT"

# Change to project root
cd "$PROJECT_ROOT"

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf build
fi

# Create and configure build directory
if [ ! -d "build" ]; then
    echo -e "${BLUE}Creating build directory...${NC}"
    mkdir build
fi

cd build

# Run CMake configuration if needed
if [ ! -f "CMakeCache.txt" ]; then
    echo -e "${BLUE}Running CMake configuration...${NC}"
    cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE" || {
        echo -e "${RED}❌ CMake configuration failed${NC}"
        exit 1
    }
fi

# Build the target
echo -e "${BLUE}Building target: $TARGET${NC}"
if [ "$TARGET" = "all" ]; then
    cmake --build . --config "$BUILD_TYPE" || {
        echo -e "${RED}❌ Build failed${NC}"
        exit 1
    }
else
    cmake --build . --target "$TARGET" --config "$BUILD_TYPE" || {
        echo -e "${RED}❌ Build failed for target: $TARGET${NC}"
        exit 1
    }
fi

echo -e "${GREEN}✅ Build completed successfully!${NC}"

# Show built targets
if [ -d "bin" ]; then
    echo -e "${BLUE}Built executables in bin/:${NC}"
    ls -la bin/ 2>/dev/null || echo "No executables found"
fi

if [ -d "lib" ]; then
    echo -e "${BLUE}Built libraries in lib/:${NC}"
    ls -la lib/ 2>/dev/null || echo "No libraries found"
fi

echo -e "${GREEN}Test build complete!${NC}"
