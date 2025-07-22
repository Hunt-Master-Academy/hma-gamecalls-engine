# Huntmaster Audio Engine

> ⚠️ **License: All Rights Reserved**
>
> The source code for this project is made available for **portfolio review and evaluation purposes only.** It is not licensed for use of any kind, commercial or non-commercial, without express written permission from the author. For licensing inquiries, please contact time@3dtechsolutions.us.

---

The Huntmaster Audio Engine is a cross-platform C++ audio analysis engine designed to be the core of the Huntmaster platform. Its primary function is to analyze and compare audio recordings of animal calls, such as deer grunts or turkey gobbles, by providing a similarity score. It is built to be a self-contained, high-performance static library that can be easily integrated into various client applications (desktop, mobile, etc.).

## 🚧 Project Status: In Development

This engine is currently under active development for its Minimum Viable Product (MVP). The core architecture is in place, and development is focused on implementing the master call processing pipeline.

### Phase 1: Game Calls MVP

✅ Sprint 1: Environment & Core Dependencies (100%)

🚧 Sprint 2: Master Call Pipeline & Feature Extraction (50%)

⏳ Sprint 3: Real-time Processing & DTW Comparison (0%)

⏳ Sprint 4: API Implementation & Unit Testing (25%)

⏳ Sprint 5: Build System & Integration Prep (25%)

## ✨ Features

**MFCC Extraction**: Converts raw audio signals into Mel-Frequency Cepstral Coefficients, a standard representation for audio feature analysis.

**Offline Processing**: Designed to pre-process and store feature data from master call audio files.

**Real-time Analysis**: (In Progress) Will support processing live audio chunks from a microphone.

**Audio Comparison**: (In Progress) Will use Dynamic Time Warping (DTW) to compare the features of a user's call against a master call.

**Cross-Platform**: Built with CMake to support compilation across different operating systems.

**Static Library**: Designed to be compiled into a .lib or .a file for easy integration with other projects.

**Comprehensive API Documentation**: Full Doxygen-style documentation for all public interfaces with examples and best practices.

**Comprehensive Debugging Infrastructure**: Thread-safe, component-specific debugging system with 5 log levels and performance monitoring across all tools.

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
- **Git** for submodule management

### Quick Start

#### Native Build (Windows, Linux, macOS)

```bash
# Clone with submodules
git clone --recursive https://github.com/tescolopio/huntmaster-engine.git
cd huntmaster-engine

# Configure and build
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
