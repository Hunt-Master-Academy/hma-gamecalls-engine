#!/bin/bash

# Format entire Huntmaster Engine codebase with clang-format
# This script applies the project's .clang-format configuration to all C/C++ files

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Project root directory (grandparent of scripts/development directory)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$PROJECT_ROOT"

echo -e "${BLUE}🎨 Huntmaster Engine Code Formatter${NC}"
echo -e "${BLUE}=====================================${NC}"

# Check if clang-format is available
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}❌ Error: clang-format is not installed or not in PATH${NC}"
    echo -e "${YELLOW}Please install clang-format:${NC}"
    echo "  Ubuntu/Debian: sudo apt-get install clang-format"
    echo "  macOS: brew install clang-format"
    echo "  Windows: Install LLVM tools or use VS Code extension"
    exit 1
fi

# Verify .clang-format file exists
if [ ! -f ".clang-format" ]; then
    echo -e "${RED}❌ Error: .clang-format configuration file not found${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Using clang-format: $(clang-format --version)${NC}"
echo -e "${GREEN}✅ Configuration file: .clang-format found${NC}"

# Define directories to format
DIRECTORIES=(
    "src"
    "include"
    "tests"
    "tools"
)

# Directories to exclude from formatting (third-party code)
EXCLUDE_PATTERNS=(
    "*/emsdk/*"
    "*/kissfft/*"
    "*/googletest/*"
    "*/benchmark/*"
    "*/libs/*"
    "*/_deps/*"
)

# File extensions to format
EXTENSIONS=("*.cpp" "*.hpp" "*.h" "*.cc" "*.cxx" "*.c")

# Build exclude arguments for find command
EXCLUDE_ARGS=""
for pattern in "${EXCLUDE_PATTERNS[@]}"; do
    EXCLUDE_ARGS="$EXCLUDE_ARGS -not -path \"$pattern\""
done

# Count total files (excluding third-party directories)
total_files=0
for dir in "${DIRECTORIES[@]}"; do
    if [ -d "$dir" ]; then
        for ext in "${EXTENSIONS[@]}"; do
            eval "count=\$(find \"$dir\" -name \"$ext\" -type f $EXCLUDE_ARGS | wc -l)"
            total_files=$((total_files + count))
        done
    fi
done

echo -e "${BLUE}📁 Formatting ${total_files} files in directories: ${DIRECTORIES[*]}${NC}"
echo ""

# Format function
format_files() {
    local dir=$1
    local dry_run=${2:-false}

    if [ ! -d "$dir" ]; then
        echo -e "${YELLOW}⚠️  Directory $dir not found, skipping...${NC}"
        return
    fi

    echo -e "${BLUE}📂 Processing directory: $dir${NC}"

    local formatted_count=0
    for ext in "${EXTENSIONS[@]}"; do
        while IFS= read -r -d '' file; do
            if [ "$dry_run" = true ]; then
                echo -e "  ${YELLOW}Would format:${NC} $file"
            else
                echo -e "  ${GREEN}Formatting:${NC} $file"
                clang-format -i -style=file "$file"
                if [ $? -eq 0 ]; then
                    formatted_count=$((formatted_count + 1))
                else
                    echo -e "  ${RED}❌ Failed to format: $file${NC}"
                fi
            fi
        done < <(eval "find \"$dir\" -name \"$ext\" -type f $EXCLUDE_ARGS -print0")
    done

    if [ "$dry_run" = false ] && [ $formatted_count -gt 0 ]; then
        echo -e "${GREEN}✅ Formatted $formatted_count files in $dir${NC}"
    fi
}

# Check for dry run flag
DRY_RUN=false
if [ "$1" = "--dry-run" ] || [ "$1" = "-n" ]; then
    DRY_RUN=true
    echo -e "${YELLOW}🔍 DRY RUN MODE - No files will be modified${NC}"
    echo ""
fi

# Format all directories
for dir in "${DIRECTORIES[@]}"; do
    format_files "$dir" $DRY_RUN
done

if [ "$DRY_RUN" = false ]; then
    echo ""
    echo -e "${GREEN}🎉 Code formatting completed successfully!${NC}"
    echo -e "${BLUE}📋 Summary:${NC}"
    echo -e "  ${GREEN}✅ Applied Google-based style with 4-space indentation${NC}"
    echo -e "  ${GREEN}✅ 100-character line limit enforced${NC}"
    echo -e "  ${GREEN}✅ Consistent bracing and spacing applied${NC}"
    echo -e "  ${GREEN}✅ Header include order standardized${NC}"
    echo ""
    echo -e "${YELLOW}💡 Next steps:${NC}"
    echo -e "  1. Review changes: ${BLUE}git diff${NC}"
    echo -e "  2. Test build: ${BLUE}cmake --build build${NC}"
    echo -e "  3. Run tests: ${BLUE}cd build && ctest${NC}"
    echo -e "  4. Commit changes: ${BLUE}git add . && git commit -m \"style: apply clang-format to entire codebase\"${NC}"
else
    echo ""
    echo -e "${YELLOW}📋 Dry run completed. Use without --dry-run to apply formatting.${NC}"
fi

echo ""
echo -e "${BLUE}🔧 VS Code Integration:${NC}"
echo -e "  • Format on save: ${GREEN}enabled${NC}"
echo -e "  • Auto format: ${GREEN}configured${NC}"
echo -e "  • Style config: ${GREEN}.clang-format${NC}"
