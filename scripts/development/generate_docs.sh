#!/bin/bash

# Documentation Generation Script for Huntmaster Audio Engine
# This script generates HTML documentation using Doxygen

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DOCS_OUTPUT_DIR="$PROJECT_ROOT/docs/api"
DOXYFILE="$PROJECT_ROOT/Doxyfile"

echo -e "${BLUE}Huntmaster Audio Engine - Documentation Generator${NC}"
echo "=================================================="

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo -e "${RED}Error: Doxygen is not installed or not in PATH${NC}"
    echo "Please install Doxygen:"
    echo "  Ubuntu/Debian: sudo apt-get install doxygen"
    echo "  macOS: brew install doxygen"
    echo "  Windows: Download from https://www.doxygen.nl/download.html"
    exit 1
fi

# Display Doxygen version
DOXYGEN_VERSION=$(doxygen --version)
echo -e "${GREEN}Found Doxygen version: $DOXYGEN_VERSION${NC}"

# Check if Doxyfile exists
if [ ! -f "$DOXYFILE" ]; then
    echo -e "${RED}Error: Doxyfile not found at $DOXYFILE${NC}"
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p "$DOCS_OUTPUT_DIR"

# Clean previous documentation
if [ -d "$DOCS_OUTPUT_DIR/html" ]; then
    echo -e "${YELLOW}Cleaning previous documentation...${NC}"
    rm -rf "$DOCS_OUTPUT_DIR/html"
fi

# Change to project root directory
cd "$PROJECT_ROOT"

# Generate documentation
echo -e "${BLUE}Generating documentation...${NC}"
echo "Working directory: $(pwd)"
echo "Output directory: $DOCS_OUTPUT_DIR"

if doxygen "$DOXYFILE"; then
    echo -e "${GREEN}✓ Documentation generated successfully!${NC}"

    # Check if HTML was generated
    if [ -d "$DOCS_OUTPUT_DIR/html" ]; then
        echo -e "${GREEN}✓ HTML documentation available at: $DOCS_OUTPUT_DIR/html/index.html${NC}"

        # Count generated files
        HTML_FILES=$(find "$DOCS_OUTPUT_DIR/html" -name "*.html" | wc -l)
        echo -e "${BLUE}Generated $HTML_FILES HTML files${NC}"

        # Display main entry points
        echo ""
        echo -e "${BLUE}Main documentation entry points:${NC}"
        echo "  • Main page: $DOCS_OUTPUT_DIR/html/index.html"
        echo "  • Class index: $DOCS_OUTPUT_DIR/html/annotated.html"
        echo "  • File index: $DOCS_OUTPUT_DIR/html/files.html"
        echo "  • Namespace index: $DOCS_OUTPUT_DIR/html/namespaces.html"

        # Optional: Open documentation in browser (uncomment if desired)
        # if command -v xdg-open &> /dev/null; then
        #     echo -e "${YELLOW}Opening documentation in browser...${NC}"
        #     xdg-open "$DOCS_OUTPUT_DIR/html/index.html"
        # elif command -v open &> /dev/null; then
        #     echo -e "${YELLOW}Opening documentation in browser...${NC}"
        #     open "$DOCS_OUTPUT_DIR/html/index.html"
        # fi

    else
        echo -e "${YELLOW}Warning: HTML directory not found after generation${NC}"
    fi

else
    echo -e "${RED}✗ Documentation generation failed!${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}Documentation generation complete!${NC}"
