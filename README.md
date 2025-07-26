# Huntmaster Audio Engine

> ⚠️ **License: All Rights Reserved**
>
> The source code for this project is made available for **portfolio review and evaluation purposes only.** It is not licensed for use of any kind, commercial or non-commercial, without express written permission from the authorThis project uses **CMake FetchContent** for automatic dependency management:

- **KissFFT**: Fast Fourier Transform library (automatically downloaded)
- **GoogleTest**: Testing framework (automatically downloaded)
- **Google Benchmark**: Performance benchmarking (automatically downloaded)
- **Emscripten SDK**: WebAssembly compilation toolchain (`tools/emsdk`)

## Dependency Management

The project uses **FetchContent** for automatic dependency management. No manual dependency setup is required:

```bash
# Simple clone and build - no submodules needed!
git clone https://github.com/tescolopio/huntmaster-engine.git
cd huntmaster-engine

# Build with automatic dependency download
cmake -B build
cmake --build build
```

**Benefits:**

- ✅ **Zero setup friction** - dependencies downloaded automatically
- ✅ **No submodule management** - CMake handles everything
- ✅ **Consistent versions** - locked dependency versions across builds
- ✅ **CI/CD friendly** - reliable automated builds

For inquiries, please contact info@huntmasteracademy.com.

---

The Huntmaster Audio Engine is a production-ready, cross-platform C++ audio analysis engine designed for real-time wildlife call analysis and comparison. Built with modern C++20 architecture, it provides high-performance audio processing with comprehensive testing infrastructure and unified build system for desktop, web (WebAssembly), and mobile platforms.

The engine analyzes and compares audio recordings of animal calls (e.g., deer grunts, turkey gobbles) using advanced signal processing techniques including MFCC feature extraction and Dynamic Time Warping (DTW) comparison algorithms. It serves as the core processing library for the Huntmaster platform ecosystem.

## 🎉 Project Status: Core Engine Complete, Testing Framework In Progress

The Huntmaster Audio Engine's core C++ components and web interface are complete. The comprehensive testing and security framework (Phase 3) is currently in the implementation phase. The engine features a modern C++20 architecture with a unified cross-platform build system and an extensive, planned validation suite.

### Current Status: **Phase 1 & 2 Complete, Phase 3 In Progress**

✅ **Core Architecture & Infrastructure**: 100% Complete
✅ **Audio Processing Pipeline**: 100% Complete
✅ **Testing & Quality Assurance**: 100% Complete (130+ tests passing)
✅ **Build System & Development Infrastructure**: 100% Complete
✅ **Desktop Platform Integration**: 100% Complete
✅ **Web Platform (WASM)**: 98% Complete
⏳ **Testing & Security Framework**: Architecture Planned, Implementation In Progress

### 🆕 Recent Achievements: Complete Web Development Stack

#### ✨ Phase 2.2E: Theme & Utilities System Complete (July 24, 2025)

✅ **Advanced Theme System**: Complete CSS theme architecture with 7 specialized modules (3,247 lines)
✅ **Production-Ready CSS**: All syntax validated, browser-compatible, and performance-optimized
✅ **Modern Design System**: Material Design principles with audio-specific enhancements

**📦 Phase 2.2E Modules Completed:**

- **animations.css** (974 lines) - 40+ keyframe animations with audio-specific effects
- **transitions.css** (690 lines) - 25+ cubic-bezier easing functions
- **shadows.css** (703 lines) - Material Design elevation system with colored shadows
- **borders.css** (962 lines) - Comprehensive border utilities with decorative patterns
- **utilities.css** (329 lines) - Essential utility classes for rapid development
- **themes.css** (586 lines) - Complete theme system with dynamic switching
- **dark-mode.css** (Complete) - Dark mode theme with accessibility optimizations

#### 🎯 Web Interface Modularization Complete

✅ **Modular Architecture**: Successfully transformed monolithic audio-processor.js (900 lines, 118 TODOs) into **11 specialized ES6 modules** (10,340+ lines)
✅ **Enterprise-Grade Modules**: Event-driven architecture with single responsibility principle
✅ **Production Ready**: Full integration testing and verification completed
✅ **Legacy Compatibility**: Backward compatibility maintained for existing integrations

**📦 Implemented Modules:**

- **EventManager** (335 lines) - Advanced event system with rate limiting
- **WASMEngineManager** (527 lines) - WASM lifecycle and session management
- **AudioLevelMonitor** (693 lines) - Real-time audio analysis and monitoring
- **PerformanceMonitor** (701 lines) - Performance tracking and optimization
- **NoiseDetector** (762 lines) - Spectral noise analysis and VAD
- **AutomaticGainControl** (1,247 lines) - Multi-band AGC with content adaptation
- **QualityAssessor** (1,274 lines) - Multi-domain quality metrics
- **MasterCallManager** (1,247 lines) - Hunting call library with ML recommendations
- **RecordingEnhancer** (1,273 lines) - Advanced recording with multi-format export
- **FormatConverter** (1,284 lines) - Multi-format conversion with batch processing
- **AudioWorkletManager** (997 lines) - Worklet management with ScriptProcessor fallback

### 6-Phase Master Test Suite: **All Phases Complete ✅**

✅ **Phase 1**: Build System Verification - All targets compile successfully
✅ **Phase 2**: Unit Testing - 130+ tests passing across 16 test suites
✅ **Phase 3**: Integration Testing - End-to-end workflows validated
✅ **Phase 4**: Component Testing - MFCC, DTW, Engine, Audio components verified
✅ **Phase 5**: Pre-WASM Validation - Cross-platform compatibility confirmed
✅ **Phase 6**: Performance Benchmarking - Real-time requirements met

## ✨ Core Features

**🎵 Complete Audio Processing Pipeline**: Modern C++20 architecture with session-based processing, real-time audio analysis, and cross-platform compatibility

**🔧 Advanced Feature Extraction**: MFCC (Mel-Frequency Cepstral Coefficients) processing with KissFFT integration for high-performance spectral analysis

**⚡ Real-time Processing**: Lock-free audio buffer management with real-time RMS/Peak monitoring and voice activity detection (VAD)

**🎯 Intelligent Comparison**: Dynamic Time Warping (DTW) algorithms for robust audio similarity analysis with multi-dimensional scoring

**🧪 Production-Grade Testing**: Comprehensive test suite with 130+ tests across 16 test suites, achieving 100% pass rate with unified TestUtils framework

**🌐 Cross-Platform Build System**: Unified CMake configuration supporting native (Windows/Linux/macOS) and WebAssembly builds with automatic dependency management

**🔍 Comprehensive Debugging**: Professional debugging infrastructure with 10 specialized tools, component-specific logging, and performance monitoring

## 🔧 Debugging & Development Tools

The Huntmaster Engine includes a comprehensive debugging infrastructure with 10 enhanced tools, each supporting detailed debugging capabilities:

### Debug Features

- **Thread-safe logging** with component-specific levels
- **5 Debug levels**: ERROR, WARN, INFO, DEBUG, TRACE
- **14 Component categories** for targeted debugging
- **Performance monitoring** with timing metrics
- **Console and file output** options
- **Configurable timestamps** and thread IDs

### Available Tools

1. **interactive_recorder** - Interactive audio recording with live monitoring
2. **debug_dtw_similarity** - DTW similarity analysis debugging
3. **simple_unified_test** - Core engine functionality testing
4. **test_mfcc_debugging** - MFCC feature extraction analysis
5. **analyze_recording** - Audio file analysis tool
6. **audio_trimmer** - Audio preprocessing utility
7. **audio_visualization** - Audio data visualization
8. **detailed_analysis** - Comprehensive audio analysis
9. **generate_features** - Feature extraction tool
10. **real_time_recording_monitor** - Real-time audio monitoring

### Debug Usage Examples

```bash
# Basic debugging
./interactive_recorder --debug

# Trace-level debugging with performance metrics
./interactive_recorder --trace --performance

# Component-specific debugging
./interactive_recorder --engine-debug --recording-debug

# Help for any tool
./interactive_recorder --help
```

### Debug Components

- **GENERAL** - General system operations
- **UNIFIED_ENGINE** - Core engine functionality
- **MFCC_PROCESSOR** - MFCC feature extraction
- **DTW_COMPARATOR** - Dynamic Time Warping comparison
- **VAD** - Voice Activity Detection
- **REALTIME_PROCESSOR** - Real-time processing
- **AUDIO_BUFFER_POOL** - Audio buffer management
- **AUDIO_LEVEL_PROCESSOR** - Audio level processing
- **WAVEFORM_GENERATOR** - Audio waveform generation
- **REALTIME_SCORER** - Real-time scoring
- **TOOLS** - Development tools
- **AUDIO_ENGINE** - Audio engine operations
- **FEATURE_EXTRACTION** - Feature extraction processes
- **SIMILARITY_ANALYSIS** - Similarity analysis algorithms
- **PERFORMANCE** - Performance monitoring and metrics

## 📚 API Documentation

The Huntmaster Audio Engine provides comprehensive API documentation for all public interfaces to help developers integrate the engine into their applications.

### Documentation Features

- **Complete API Reference**: Doxygen-generated documentation for all public classes, methods, and functions
- **Usage Examples**: Code examples for common integration patterns
- **Algorithm Details**: Technical explanations of MFCC extraction, DTW comparison, and VAD processing
- **Integration Guides**: Platform-specific integration instructions for mobile and desktop applications
- **Best Practices**: Recommended patterns for real-time processing and memory management

### Generating Documentation

#### Prerequisites

- **Doxygen** 1.8.13 or higher
- **Graphviz** (optional, for class diagrams)

#### Generate HTML Documentation

```bash
# Linux/macOS
./scripts/generate_docs.sh

# Windows
scripts\generate_docs.bat

# Manual generation
doxygen Doxyfile
```

#### Documentation Output

Generated documentation is available at:

- **Main documentation**: `docs/api/html/index.html`
- **Class index**: `docs/api/html/annotated.html`
- **File index**: `docs/api/html/files.html`
- **Namespace index**: `docs/api/html/namespaces.html`

### Online Documentation

For the latest API documentation, visit our GitHub Pages deployment (available after merging to main branch).

### Key API Interfaces

- **`UnifiedAudioEngine`**: Main engine interface for audio analysis
- **`MFCCProcessor`**: Feature extraction from audio data
- **`DTWComparator`**: Audio similarity comparison algorithms
- **`VoiceActivityDetector`**: Audio activity detection
- **`AudioRecorder`**: Real-time audio capture
- **`AudioPlayer`**: Audio playback functionality

## 🏗️ Build System

The Huntmaster Engine uses a **modular CMake build system** that supports both native and WebAssembly builds from a unified configuration. The build system has been refactored into a hierarchical structure for improved maintainability and scalability.

### Build System Architecture

The CMake configuration is organized across multiple specialized files:

- **Root `CMakeLists.txt`**: Project-wide configuration and build coordination
- **`src/CMakeLists.txt`**: Core UnifiedAudioEngine library configuration
- **`tests/CMakeLists.txt`**: Complete test suite (unit tests, integration tests, benchmarks)
- **`tools/CMakeLists.txt`**: Command-line development tools

### Prerequisites

- **CMake** 3.16 or higher
- **C++20** compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- **Git** (dependencies are automatically downloaded by CMake)

### Quick Start

#### Native Build (Windows, Linux, macOS)

```bash
# Simple clone and build - dependencies handled automatically!
git clone https://github.com/tescolopio/huntmaster-engine.git
cd huntmaster-engine

# Configure and build with automatic dependency download
cmake -B build
cmake --build build

# Run tests
cd build && ctest
```

#### WebAssembly Build

```bash
# Setup Emscripten SDK (first time only)
cd tools/emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build for WebAssembly
cd ../..
emcmake cmake -B build-wasm
cmake --build build-wasm
```

### Build Configuration Options

| Option                   | Default | Description                                |
| ------------------------ | ------- | ------------------------------------------ |
| `HUNTMASTER_BUILD_TESTS` | `ON`    | Enable unit tests and benchmarks           |
| `HUNTMASTER_BUILD_TOOLS` | `ON`    | Enable command-line development tools      |
| `CMAKE_BUILD_TYPE`       | `Debug` | Build type: Debug, Release, RelWithDebInfo |

#### Custom Configuration Examples

```bash
# Release build with optimizations
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Build without tests for faster compilation
cmake -B build -DHUNTMASTER_BUILD_TESTS=OFF
cmake --build build

# Build only the core library
cmake -B build -DHUNTMASTER_BUILD_TESTS=OFF -DHUNTMASTER_BUILD_TOOLS=OFF
cmake --build build
```

### Build Targets

The modular build system automatically discovers and configures:

#### Core Library

- **`UnifiedAudioEngine`**: Main static library for all platforms

#### Development Tools (10 tools)

- **`interactive_recorder`**: Interactive audio recording with live monitoring
- **`debug_dtw_similarity`**: DTW similarity analysis debugging
- **`analyze_recording`**: Audio file analysis tool
- **`audio_trimmer`**: Audio preprocessing utility
- **`simple_unified_test`**: Core engine functionality testing
- And 5 additional specialized tools...

#### Test Suite

- **`RunEngineTests`**: Main Google Test runner
- **Standalone diagnostic tools**: Individual test executables
- **Integration tests**: Real wildlife call validation
- **Benchmarks**: Performance testing with Google Benchmark

### Development Scripts

The project includes organized development scripts in the `scripts/` directory:

#### Build Scripts (`scripts/build/`)

- **`scripts/build/build_wasm.sh`**: WebAssembly compilation
- **`scripts/build/test_build.sh`**: Build system validation
- **`setup-web-deployment.sh`**: Web deployment preparation

#### Testing Scripts (`scripts/testing/`)

- **`scripts/testing/master_test.sh`**: Complete 6-phase test suite orchestrator
- **`scripts/testing/comprehensive_test.sh`**: Core engine testing
- **`scripts/testing/component_test.sh`**: Component-specific testing
- **`pre_wasm_validation.sh`**: WASM deployment readiness validation

#### Development Tools (`scripts/development/`)

- **`debug_test.sh`**: Debug-enabled test execution
- **`format_code.sh`**: Code formatting with clang-format
- **`generate_docs.sh`**: Doxygen documentation generation

### Platform-Specific Notes

#### Windows

```bash
# Visual Studio 2019/2022
cmake -B build -G "Visual Studio 16 2019"
cmake --build build --config Release
```

#### Linux/macOS

```bash
# Debug build with detailed output
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=ON
make -C build -j$(nproc)
```

### Troubleshooting

#### Common Issues

1. **Submodules not initialized**: Run `git submodule update --init --recursive`
2. **CMake version too old**: Ensure CMake 3.16+
3. **C++20 not supported**: Update to GCC 10+, Clang 12+, or MSVC 2019+

#### Getting Help

```bash
# View all available targets
cmake --build build --target help

# Detailed build configuration
cmake -B build -DCMAKE_VERBOSE_MAKEFILE=ON
```

## Dependencies

This project uses git submodules for external dependencies:

- **KissFFT**: Fast Fourier Transform library (`libs/kissfft`)
- **GoogleTest**: Testing framework (`tests/lib/googletest`)
- **Google Benchmark**: Performance benchmarking (`tests/lib/benchmark`)
- **Emscripten SDK**: WebAssembly compilation (`tools/emsdk`)

### Dependency Management

```bash
# Initial clone with all dependencies
git clone --recursive https://github.com/tescolopio/huntmaster-engine.git

# Update existing repository
git pull
git submodule update --init --recursive
```

## 📚 Documentation

Comprehensive documentation is available in the [`docs/`](docs/) directory:

- **[Architecture Guide](docs/architecture.md)** - Technical design and system architecture
- **[Debugging Guide](docs/DEBUGGING.md)** - Debug tools and troubleshooting
- **[Deployment Guide](docs/DEPLOYMENT.md)** - Production deployment and configuration
- **[Feature Implementation](docs/FeatureImplementationGuide.md)** - Guide for adding new features
- **[Development Progress](docs/Dev_Progress.md)** - Project milestones and progress

For a complete documentation index, see [`docs/README.md`](docs/README.md).

## 🏗️ Project Structure

```
huntmaster-engine/
├── src/              # Core engine source code
├── include/          # Public header files
├── tests/            # Unit tests, integration tests, benchmarks
├── tools/            # Development and diagnostic tools
├── docs/             # Project documentation
├── libs/             # Third-party libraries (git submodules)
├── scripts/          # Build and utility scripts
├── .vscode/          # VS Code configuration
└── build/            # Build output directory
    ├── bin/          # Compiled executables
    └── lib/          # Static libraries
```
