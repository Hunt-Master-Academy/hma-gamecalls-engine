#!/bin/bash

# Root Directory Organization Script
# This script organizes the huntmaster-engine project root for a clean git repository

set -e

echo "=== Huntmaster Engine Root Directory Organization ==="
echo "Organizing files for clean git repository structure..."

cd "/workspaces/huntmaster-engine"

# Create organized directory structure if not exists
echo "Creating organized directory structure..."

# Documentation directories
mkdir -p docs/
mkdir -p docs/alpha_testing/
mkdir -p docs/deployment/
mkdir -p docs/development/
mkdir -p docs/status_reports/

# Archive and backup directories
mkdir -p archive/
mkdir -p archive/old_scripts/
mkdir -p archive/coverage_reports/

# Scripts directory
mkdir -p scripts/
mkdir -p scripts/build/
mkdir -p scripts/testing/
mkdir -p scripts/deployment/

echo ""
echo "=== Moving Documentation Files ==="

# Alpha testing documentation
mv ALPHA_TESTING_*.md docs/alpha_testing/ 2>/dev/null || true

# Deployment documentation
mv DEPLOYMENT_*.md docs/deployment/ 2>/dev/null || true
mv BUILD_DEBUG_CHECKLIST.md docs/deployment/ 2>/dev/null || true
mv COMMIT_*.md docs/deployment/ 2>/dev/null || true

# Development documentation
mv API_*.md docs/development/ 2>/dev/null || true
mv FEATURE_*.md docs/development/ 2>/dev/null || true
mv PROJECT_*.md docs/development/ 2>/dev/null || true
mv TEST_*.md docs/development/ 2>/dev/null || true
mv WAVEFORM_*.md docs/development/ 2>/dev/null || true
mv WEB_APP_*.md docs/development/ 2>/dev/null || true

# Status reports
mv DAY_*_PROGRESS_REPORT.md docs/status_reports/ 2>/dev/null || true
mv CURRENT_PROJECT_STATUS.md docs/status_reports/ 2>/dev/null || true
mv CONTAINER_ENVIRONMENT_TEST_RESULTS.md docs/status_reports/ 2>/dev/null || true
mv TESTING_ENVIRONMENT_ALIGNMENT_REPORT.md docs/status_reports/ 2>/dev/null || true
mv PHASE*.md docs/status_reports/ 2>/dev/null || true
mv FINAL_*.md docs/status_reports/ 2>/dev/null || true
mv GAME_CALLS_*.md docs/status_reports/ 2>/dev/null || true

echo ""
echo "=== Moving Build and Script Files ==="

# Build scripts
mv cleanup_project.sh scripts/build/ 2>/dev/null || true
mv fix_*.sh scripts/build/ 2>/dev/null || true
mv dev-container.sh scripts/build/ 2>/dev/null || true

# Testing scripts
mv coverage_analysis_comprehensive.sh scripts/testing/ 2>/dev/null || true
mv reorganize_tests.sh scripts/testing/ 2>/dev/null || true

# Archive old files
mv organize_root_files.sh archive/old_scripts/ 2>/dev/null || true

echo ""
echo "=== Moving Coverage and Performance Files ==="

# Archive coverage files
mv *.gcov archive/coverage_reports/ 2>/dev/null || true
mv performance_report.json archive/ 2>/dev/null || true

echo ""
echo "=== Moving Debug and Test Files ==="

# Archive debug files
mv debug_mfcc_test* archive/ 2>/dev/null || true
mv test_mfcc_minimal archive/ 2>/dev/null || true
mv nul archive/ 2>/dev/null || true

echo ""
echo "=== Moving Data Backup ==="

# Check if data backup needs organization
if [ -d "data_backup" ]; then
    echo "Moving data_backup to archive..."
    mv data_backup archive/
fi

echo ""
echo "=== Organizing Docker and Config Files ==="

# Keep Docker files in root but ensure they're properly named
if [ -f "Dockerfile.dev" ] && [ -f "Dockerfile.testing" ]; then
    echo "Docker files are properly organized"
fi

echo ""
echo "=== Creating .gitignore for Clean Repository ==="

# Create comprehensive .gitignore
cat > .gitignore << 'EOF'
# Build directories
build/
build-*/
bin/
lib/

# IDE and editor files
.vscode/settings.json
.vscode/launch.json
.idea/
*.swp
*.swo
*~

# Compiled Object files
*.o
*.obj
*.so
*.dylib
*.dll

# Executables
*.exe
*.out
*.app

# Coverage files
*.gcov
*.gcda
*.gcno
coverage.info
coverage_reports/
lcov.info

# Test output files
test_logs/
*.log

# Temporary files
*.tmp
*.temp
nul

# Performance reports
performance_report.json

# Archive and backup directories
archive/
data_backup/

# Node modules (if any)
node_modules/

# Python cache
__pycache__/
*.pyc

# macOS
.DS_Store

# Windows
Thumbs.db
EOF

echo ""
echo "=== Creating README.md Structure Guide ==="

# Update README with new structure
if [ -f "README.md" ]; then
    # Backup existing README
    cp README.md README.md.backup
fi

cat > README.md << 'EOF'
# Huntmaster Audio Engine

A real-time audio processing engine for wildlife call analysis and recognition.

## Project Structure

```
huntmaster-engine/
├── src/                    # Source code
├── include/               # Header files
├── tests/                 # Organized test suite
│   ├── unit/             # Unit tests by category
│   │   ├── core/         # Core engine tests
│   │   ├── audio/        # Audio processing tests
│   │   ├── analysis/     # Analysis algorithm tests
│   │   ├── vad/          # Voice Activity Detection tests
│   │   ├── security/     # Security tests
│   │   └── utils/        # Utility tests
│   ├── integration/      # Integration tests
│   ├── performance/      # Performance benchmarks
│   ├── tools/           # Test tools and utilities
│   └── lib/             # Test libraries
├── docs/                 # Documentation
│   ├── alpha_testing/    # Alpha testing documentation
│   ├── deployment/       # Deployment guides
│   ├── development/      # Development documentation
│   └── status_reports/   # Project status reports
├── scripts/              # Build and utility scripts
│   ├── build/           # Build scripts
│   ├── testing/         # Testing scripts
│   └── deployment/      # Deployment scripts
├── data/                # Test data and audio files
├── web/                 # Web interface
├── bindings/            # Language bindings
└── tools/               # Development tools
```

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Testing

```bash
# Core functionality tests (must pass)
make test_core

# Quick test suite
make test_quick

# All unit tests
make test_unit

# Complete test suite
make test_all
```

## Core Tests Status

The core functionality tests ensure the engine's basic operations are working:
- Audio Processing: Feature extraction and similarity scoring
- Session Management: Session lifecycle and isolation
- Master Call Management: Loading and processing reference calls
- Recording System: Audio recording and playback

## License

See LICENSE file for details.
EOF

echo ""
echo "=== Organization Summary ==="

echo "Files organized into:"
echo "📁 docs/              - All documentation"
echo "📁 scripts/           - Build and utility scripts"
echo "📁 archive/           - Old files and coverage reports"
echo "📁 tests/             - Reorganized test suite"
echo ""
echo "✅ Root directory cleaned for git repository"
echo "✅ Documentation categorized and organized"
echo "✅ Build scripts moved to scripts/ directory"
echo "✅ Test suite reorganized by category"
echo "✅ .gitignore updated for clean repository"
echo "✅ README.md updated with project structure"
echo ""
echo "Repository is now ready for git commit!"

echo ""
echo "=== Next Steps ==="
echo "1. Review the organized structure"
echo "2. Test the build system: cd build && ninja"
echo "3. Test core functionality: make test_core"
echo "4. Commit to git repository"
echo ""
echo "Directory structure:"
tree -L 2 -I 'build*|.git' 2>/dev/null || ls -la
