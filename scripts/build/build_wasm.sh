#!/bin/bash

# ==============================================================================
# WebAssembly Build Script for Huntmaster Audio Engine
# ==============================================================================
# This script builds the Huntmaster Engine for WebAssembly using Emscripten
#
# Prerequisites:
# - Emscripten SDK installed and activated
# - CMake 3.16 or higher
#
# Usage: ./build_wasm.sh [--clean] [--debug] [--help]
# ==============================================================================

set -e

# Configuration
BUILD_TYPE="Release"
CLEAN_BUILD=false
EMSDK_PATH="tools/emsdk"

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
        --help|-h)
            echo "Usage: $0 [--clean] [--debug] [--help]"
            echo ""
            echo "Options:"
            echo "  --clean    Clean build directory before building"
            echo "  --debug    Build in Debug mode (default: Release)"
            echo "  --help     Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}Huntmaster Engine - WebAssembly Build${NC}"
echo "====================================="
echo "Build type: $BUILD_TYPE"
echo "Project root: $PROJECT_ROOT"

# Change to project root
cd "$PROJECT_ROOT"

# Check if Emscripten is available
if ! command -v emcmake &> /dev/null; then
    echo -e "${YELLOW}Emscripten not found in PATH. Attempting to activate...${NC}"

    if [ -f "$EMSDK_PATH/emsdk_env.sh" ]; then
        echo "Activating Emscripten SDK..."
        source "$EMSDK_PATH/emsdk_env.sh"
    else
        echo -e "${RED}Error: Emscripten SDK not found at $EMSDK_PATH${NC}"
        echo "Please install Emscripten SDK or update the EMSDK_PATH variable"
        exit 1
    fi
fi

# Verify Emscripten is working
echo -e "${GREEN}Emscripten version: $(emcc --version | head -n1)${NC}"

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning WASM build directory...${NC}"
    rm -rf build-wasm
fi

# Create build directory
mkdir -p build-wasm

# Configure with Emscripten
echo -e "${BLUE}Configuring WASM build...${NC}"
emcmake cmake -B build-wasm \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DHUNTMASTER_BUILD_TESTS=OFF \
    -DHUNTMASTER_BUILD_TOOLS=OFF

# Build
echo -e "${BLUE}Building WASM target...${NC}"
cmake --build build-wasm --config "$BUILD_TYPE"

# Check if build was successful
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ WASM build completed successfully!${NC}"

    # List generated files
    echo -e "${BLUE}Generated WASM files:${NC}"
    find build-wasm -name "*.wasm" -o -name "*.js" | head -10

    echo ""
    echo -e "${GREEN}WASM build complete! Files are in build-wasm/${NC}"
else
    echo -e "${RED}✗ WASM build failed!${NC}"
    exit 1
fi
