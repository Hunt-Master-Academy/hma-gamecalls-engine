#!/bin/bash

# PROJECT CLEANUP SCRIPT - Huntmaster Engine
# Date: August 2, 2025
# Purpose: Clean and organize project for production commit

set -e

echo "🧹 HUNTMASTER ENGINE PROJECT CLEANUP"
echo "===================================="
echo ""

PROJECT_ROOT="/workspaces/huntmaster-engine"
cd "$PROJECT_ROOT"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

# Phase 1: Build Artifacts Cleanup
print_step "Phase 1: Cleaning Build Artifacts"

echo "Current state analysis..."
COVERAGE_FILES=$(find coverage_* -type f 2>/dev/null | wc -l)
BUILD_ARTIFACTS=$(find build* -name "*.gcda" -o -name "*.gcno" 2>/dev/null | wc -l)
echo "  - Coverage analysis files: $COVERAGE_FILES"
echo "  - Build artifacts: $BUILD_ARTIFACTS"

# Archive essential coverage reports before cleanup
print_info "Archiving essential coverage reports..."
mkdir -p archive/coverage_reports_archive_$(date +%Y%m%d)

# Keep the most recent coverage analysis
if [[ -d coverage_analysis ]]; then
    LATEST_COVERAGE=$(ls -t coverage_analysis/*.log 2>/dev/null | head -1)
    if [[ -n "$LATEST_COVERAGE" ]]; then
        cp "$LATEST_COVERAGE" archive/coverage_reports_archive_$(date +%Y%m%d)/
        print_success "Archived latest coverage report"
    fi
fi

# Clean coverage data (can be regenerated)
print_info "Cleaning regeneratable coverage data..."
rm -rf coverage_data/
print_success "Cleaned coverage_data/"

# Keep only latest coverage analysis, remove old ones
if [[ -d coverage_analysis ]]; then
    cd coverage_analysis
    # Keep latest 2 files, remove the rest
    ls -t *.log 2>/dev/null | tail -n +3 | xargs rm -f 2>/dev/null || true
    cd ..
    print_success "Cleaned old coverage analysis files"
fi

# Phase 2: Test Logs Organization
print_step "Phase 2: Organizing Test Logs"

if [[ -d test_logs ]]; then
    mkdir -p archive/test_logs_archive_$(date +%Y%m%d)

    # Archive old test logs, keep only recent ones
    find test_logs -name "*.log" -mtime +7 -exec mv {} archive/test_logs_archive_$(date +%Y%m%d)/ \; 2>/dev/null || true

    # Keep only the most recent 5 test logs
    cd test_logs
    ls -t *.log 2>/dev/null | tail -n +6 | xargs rm -f 2>/dev/null || true
    cd ..
    print_success "Organized test logs"
fi

# Phase 3: Documentation Cleanup
print_step "Phase 3: Documentation Organization"

# Archive outdated documentation files
OUTDATED_DOCS=(
    "BUILD_DEBUG_CHECKLIST.md"
    "PHASE3_ACTION_PLAN.md"
    "PROJECT_STATUS_ANALYSIS.md"
    "CONTAINER_ENVIRONMENT_TEST_RESULTS.md"
)

mkdir -p archive/documentation_archive_$(date +%Y%m%d)

for doc in "${OUTDATED_DOCS[@]}"; do
    if [[ -f "$doc" ]]; then
        print_info "Archiving $doc..."
        mv "$doc" archive/documentation_archive_$(date +%Y%m%d)/
        print_success "Archived $doc"
    fi
done

# Phase 4: Temporary Files Cleanup
print_step "Phase 4: Cleaning Temporary Files"

# Remove any remaining temporary files
find . -name "*.tmp" -delete 2>/dev/null || true
find . -name "*.bak" -delete 2>/dev/null || true
find . -name "*~" -delete 2>/dev/null || true

# Clean VS Code backup if it exists
if [[ -d .vscode.backup.* ]]; then
    rm -rf .vscode.backup.*
    print_success "Cleaned VS Code backup directories"
fi

# Phase 5: Git Ignore Update
print_step "Phase 5: Updating .gitignore"

# Ensure .gitignore has proper exclusions
cat >> .gitignore << 'EOF'

# Build artifacts
build/
build-*/
*.gcda
*.gcno

# Coverage data
coverage_data/
coverage_reports/

# Temporary files
*.tmp
*.bak
*~

# IDE backups
.vscode.backup.*

# Test outputs
test_logs/*.log
!test_logs/README.md

EOF

print_success "Updated .gitignore with cleanup patterns"

# Phase 6: Final State Analysis
print_step "Phase 6: Final State Analysis"

echo ""
echo "CLEANUP RESULTS:"
echo "===================="

NEW_COVERAGE_FILES=$(find coverage_* -type f 2>/dev/null | wc -l || echo "0")
NEW_BUILD_ARTIFACTS=$(find build* -name "*.gcda" -o -name "*.gcno" 2>/dev/null | wc -l || echo "0")
ARCHIVE_FILES=$(find archive -type f 2>/dev/null | wc -l || echo "0")

echo "  - Coverage files: $COVERAGE_FILES → $NEW_COVERAGE_FILES"
echo "  - Build artifacts: $BUILD_ARTIFACTS → $NEW_BUILD_ARTIFACTS"
echo "  - Archive files: $ARCHIVE_FILES"

# Directory size analysis
echo ""
echo "📊 DIRECTORY SIZES:"
echo "==================="
du -sh src/ include/ tests/ tools/ scripts/ data/ docs/ 2>/dev/null | sort -hr

echo ""
print_success "PROJECT CLEANUP COMPLETED!"
echo ""
echo "NEXT STEPS:"
echo "  1. Review cleaned project structure"
echo "  2. Run build test to verify functionality"
echo "  3. Update documentation as needed"
echo "  4. Prepare for git commit"
echo ""
echo "📋 READY FOR PRODUCTION COMMIT"
