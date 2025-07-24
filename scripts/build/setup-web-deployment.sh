#!/bin/bash

# ==============================================================================
# Web Deployment Setup Script for Huntmaster Audio Engine
# ==============================================================================
# This script sets up the web deployment environment and copies necessary files
#
# Usage: ./setup-web-deployment.sh [--clean] [--build-wasm] [--serve]
# ==============================================================================

set -e

# Configuration
CLEAN_DEPLOY=false
BUILD_WASM=false
SERVE_AFTER=false
WEB_DIR="web"
DEPLOY_DIR="dist"

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
            CLEAN_DEPLOY=true
            shift
            ;;
        --build-wasm)
            BUILD_WASM=true
            shift
            ;;
        --serve)
            SERVE_AFTER=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [--clean] [--build-wasm] [--serve] [--help]"
            echo ""
            echo "Options:"
            echo "  --clean        Clean deployment directory before setup"
            echo "  --build-wasm   Build WASM before deployment setup"
            echo "  --serve        Start development server after setup"
            echo "  --help         Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}Huntmaster Engine - Web Deployment Setup${NC}"
echo "========================================"
echo "Project root: $PROJECT_ROOT"

# Change to project root
cd "$PROJECT_ROOT"

# Clean deployment directory if requested
if [ "$CLEAN_DEPLOY" = true ]; then
    echo -e "${YELLOW}Cleaning deployment directory...${NC}"
    rm -rf "$DEPLOY_DIR"
fi

# Build WASM if requested
if [ "$BUILD_WASM" = true ]; then
    echo -e "${BLUE}Building WASM...${NC}"
    "$SCRIPT_DIR/build_wasm.sh" || {
        echo -e "${RED}❌ WASM build failed${NC}"
        exit 1
    }
fi

# Create deployment directory
echo -e "${BLUE}Setting up deployment directory...${NC}"
mkdir -p "$DEPLOY_DIR"

# Copy web assets
if [ -d "$WEB_DIR" ]; then
    echo -e "${BLUE}Copying web assets...${NC}"
    cp -r "$WEB_DIR"/* "$DEPLOY_DIR/"
else
    echo -e "${YELLOW}Warning: Web directory not found at $WEB_DIR${NC}"
fi

# Copy WASM files if they exist
if [ -d "build-wasm" ]; then
    echo -e "${BLUE}Copying WASM files...${NC}"

    # Find and copy WASM files
    find build-wasm -name "*.wasm" -exec cp {} "$DEPLOY_DIR/" \;
    find build-wasm -name "*.js" -exec cp {} "$DEPLOY_DIR/" \;

    # Count copied files
    WASM_FILES=$(find "$DEPLOY_DIR" -name "*.wasm" | wc -l)
    JS_FILES=$(find "$DEPLOY_DIR" -name "*.js" | wc -l)

    echo -e "${GREEN}Copied $WASM_FILES WASM files and $JS_FILES JavaScript files${NC}"
else
    echo -e "${YELLOW}Warning: WASM build directory not found. Run with --build-wasm to build first.${NC}"
fi

# Copy documentation if it exists
if [ -d "docs/api/html" ]; then
    echo -e "${BLUE}Copying API documentation...${NC}"
    mkdir -p "$DEPLOY_DIR/docs"
    cp -r docs/api/html "$DEPLOY_DIR/docs/api"
fi

echo -e "${GREEN}✅ Web deployment setup complete!${NC}"

# List deployment contents
echo -e "${BLUE}Deployment directory contents:${NC}"
ls -la "$DEPLOY_DIR/"

# Start development server if requested
if [ "$SERVE_AFTER" = true ]; then
    echo -e "${BLUE}Starting development server...${NC}"

    if command -v python3 &> /dev/null; then
        echo "Starting Python HTTP server on port 8000..."
        cd "$DEPLOY_DIR"
        python3 -m http.server 8000
    elif command -v python &> /dev/null; then
        echo "Starting Python HTTP server on port 8000..."
        cd "$DEPLOY_DIR"
        python -m SimpleHTTPServer 8000
    else
        echo -e "${YELLOW}Python not found. Please serve the $DEPLOY_DIR directory manually.${NC}"
    fi
fi

echo -e "${GREEN}Web deployment ready in $DEPLOY_DIR/${NC}"
