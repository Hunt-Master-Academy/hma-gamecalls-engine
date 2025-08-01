# Huntmaster Engine - Project Structure

## 📁 Project Directory Layout

```
huntmaster-engine/
├── 📄 CMakeLists.txt                   # Main CMake configuration
├── 📄 README.md                        # Project overview and setup
├── 📄 LICENSE                          # MIT license
├── 📄 BUILD_DEBUG_CHECKLIST.md         # Current build status and debugging info
├── 📄 PROJECT_STRUCTURE.md             # This file - project organization
│
├── 📁 include/huntmaster/               # Public header files
│   ├── 📁 core/                        # Core audio processing headers
│   ├── 📁 security/                    # Security and validation headers
│   └── 📁 utils/                       # Utility headers
│
├── 📁 src/                              # Source code implementation
│   ├── 📁 core/                        # Core audio processing implementation
│   ├── 📁 security/                    # Security components implementation
│   └── 📁 utils/                       # Utility implementations
│
├── 📁 tests/                            # Test suite
│   ├── 📄 CMakeLists.txt               # Test configuration
│   ├── 📁 unit/                        # Unit tests
│   ├── 📁 integration/                 # Integration tests
│   ├── 📁 lib/                         # Test utilities
│   └── 📁 benchmarks/                  # Performance benchmarks
│
├── 📁 tools/                            # Command-line tools and utilities
├── 📁 scripts/                          # Build and automation scripts
├── 📁 data/                             # Test data and audio samples
│   ├── 📁 master_calls/                # Reference audio calls
│   ├── 📁 test_audio/                  # Test audio files
│   ├── 📁 recordings/                  # Sample recordings
│   └── 📁 test_vectors/                # Test vectors for validation
│
├── 📁 docs/                             # Documentation
│   ├── 📁 api/                         # API documentation (Doxygen)
│   └── 📁 architecture/                # Architecture documentation
│
├── 📁 build/                            # Build output directory (git-ignored)
│   ├── 📁 bin/                         # Compiled executables
│   ├── 📁 lib/                         # Compiled libraries
│   └── 📁 tests/                       # Test executables
│
├── 📁 coverage_analysis/                # Coverage analysis reports
├── 📁 test_logs/                        # Test execution logs
│
├── 📁 archive/                          # Archived files
│   ├── 📁 reports/                     # Old documentation reports
│   └── 📁 legacy/                      # Legacy code and components
│
├── 📁 web/                              # Web interface components
├── 📁 bindings/                         # Language bindings
│   └── 📁 wasm/                        # WebAssembly bindings
│
└── 📁 libs/                             # Third-party libraries
```

## 🎯 Key Components

### Core Engine (`src/core/`)
- **UnifiedAudioEngine.cpp** - Main audio processing engine
- **RealtimeScorer.cpp** - Real-time audio comparison
- **MFCCProcessor.cpp** - MFCC feature extraction
- **DTWComparator.cpp** - Dynamic Time Warping comparison
- **WaveformAnalyzer.cpp** - Waveform analysis tools

### Test Infrastructure (`tests/`)
- **Unit Tests** - Component-level testing
- **Integration Tests** - End-to-end testing
- **Benchmarks** - Performance testing
- **Test Utilities** - Shared testing infrastructure

### Tools & Scripts (`tools/`, `scripts/`)
- **master_test.sh** - Comprehensive test runner
- **enhanced_coverage_analysis.sh** - Coverage analysis script
- **Command-line utilities** - Analysis and debugging tools

## 📊 Current Project Status

### Build Status: ✅ 100% SUCCESS
- All components building successfully
- No compilation errors
- All test executables functional

### Test Coverage: 📈 20.96%
- **Total Lines**: 4,975
- **Covered Lines**: 1,043
- **Target**: 90% coverage

### Priority Files for Coverage:
1. **WaveformAnalyzer.cpp** - 541 lines (0% coverage)
2. **PerformanceProfiler.cpp** - 490 lines (0% coverage)
3. **UnifiedAudioEngine.cpp** - 873 lines (46.51% coverage)

## 🧹 Recent Cleanup (July 31, 2025)

### Removed:
- ❌ `debug_path_test*` - Temporary debug files
- ❌ `debug_realtime_scorer.cpp` - Debug utility
- ❌ Old test logs from July 24th
- ❌ 60 outdated test log files
- ❌ 48 gcov coverage files from build/

### Archived:
- 📦 `DEBUGGING_SUCCESS_REPORT.md` → `archive/reports/`
- 📦 `MASTER_TEST_ENHANCEMENT_SUMMARY.md` → `archive/reports/`

### Organized:
- 📁 Test audio files moved to `data/test_audio/`
- 📁 Updated `.gitignore` for cleaned structure
- 📁 Enhanced project documentation

## 🚀 Next Steps

1. **Coverage Enhancement** - Target high-impact files for testing
2. **API Completion** - Implement missing component methods
3. **Documentation** - Complete API documentation with Doxygen
4. **Performance** - Optimize real-time processing pipeline

---
*Last Updated: July 31, 2025*
*Status: Project structure cleaned and organized*
