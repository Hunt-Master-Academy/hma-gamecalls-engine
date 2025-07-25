#!/bin/bash

# ==============================================================================
# WASM Build Optimizer Script for Huntmaster Audio Engine
# ==============================================================================
# This script provides advanced WASM build optimizations including production
# vs development modes, size optimization, and performance profiling.
#
# Author: Huntmaster Engine Team
# Version: 1.0
# Date: July 24, 2025
# ==============================================================================

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Build Configuration Variables
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
SOURCE_DIR="$PROJECT_ROOT/src"
INCLUDE_DIR="$PROJECT_ROOT/include"
WEB_DIR="$PROJECT_ROOT/web"
OUTPUT_DIR="$BUILD_DIR/wasm"

# Build mode configuration
BUILD_MODE="development"
OPTIMIZATION_LEVEL="O2"
ENABLE_PROFILING=false
ENABLE_DEBUGGING=true
GENERATE_SOURCEMAPS=true
ENABLE_SIMD=false
ENABLE_THREADING=false
MEMORY_SIZE="512MB"
STACK_SIZE="64KB"
CLOSURE_COMPILER=false

# Advanced build features
ENABLE_MODULAR_BUILD=false
ENABLE_CODEC_SELECTION=false
SELECTED_CODECS=""
BUILD_CACHE_ENABLED=true
PARALLEL_JOBS=4

# Performance and validation flags
ENABLE_SIZE_REPORTING=true
ENABLE_VALIDATION=true
ENABLE_BENCHMARKING=false
ENABLE_DEPENDENCY_ANALYSIS=false

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --mode)
                BUILD_MODE="$2"
                shift 2
                ;;
            --optimization)
                OPTIMIZATION_LEVEL="$2"
                shift 2
                ;;
            --memory-size)
                MEMORY_SIZE="$2"
                shift 2
                ;;
            --enable-simd)
                ENABLE_SIMD=true
                shift
                ;;
            --enable-threading)
                ENABLE_THREADING=true
                shift
                ;;
            --enable-profiling)
                ENABLE_PROFILING=true
                shift
                ;;
            --disable-debugging)
                ENABLE_DEBUGGING=false
                shift
                ;;
            --enable-closure)
                CLOSURE_COMPILER=true
                shift
                ;;
            --modular)
                ENABLE_MODULAR_BUILD=true
                shift
                ;;
            --codecs)
                ENABLE_CODEC_SELECTION=true
                SELECTED_CODECS="$2"
                shift 2
                ;;
            --jobs)
                PARALLEL_JOBS="$2"
                shift 2
                ;;
            --enable-benchmarking)
                ENABLE_BENCHMARKING=true
                shift
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                echo -e "${RED}Unknown option: $1${NC}"
                show_help
                exit 1
                ;;
        esac
    done
}

# Show help information
show_help() {
    echo "WASM Build Optimizer for Huntmaster Audio Engine"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --mode MODE                   Build mode: development, production, or debug"
    echo "  --optimization LEVEL          Optimization level: O0, O1, O2, O3, Os, Oz"
    echo "  --memory-size SIZE           Initial memory size (e.g., 512MB, 1GB)"
    echo "  --enable-simd                Enable SIMD optimizations"
    echo "  --enable-threading           Enable threading support"
    echo "  --enable-profiling           Enable performance profiling"
    echo "  --disable-debugging          Disable debugging symbols"
    echo "  --enable-closure             Enable Closure Compiler optimization"
    echo "  --modular                    Build modular WASM for lazy loading"
    echo "  --codecs LIST                Select specific codecs (comma-separated)"
    echo "  --jobs N                     Number of parallel compilation jobs"
    echo "  --enable-benchmarking        Enable build performance benchmarking"
    echo "  --help                       Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 --mode production --optimization O3 --enable-closure"
    echo "  $0 --mode development --enable-debugging --enable-profiling"
    echo "  $0 --modular --codecs wav,mp3,ogg --enable-simd"
}

# Print colored status messages
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Validate build environment
validate_environment() {
    print_status "Validating build environment..."

    # Setup Emscripten SDK if not in PATH
    if ! command -v emcc &> /dev/null; then
        print_status "Setting up Emscripten SDK..."
        EMSDK_DIR="$PROJECT_ROOT/tools/emsdk"
        if [[ -f "$EMSDK_DIR/emsdk_env.sh" ]]; then
            source "$EMSDK_DIR/emsdk_env.sh" > /dev/null 2>&1
            print_status "Emscripten SDK activated from: $EMSDK_DIR"
        else
            print_error "Emscripten SDK not found. Please install Emscripten SDK or check tools/emsdk directory."
            exit 1
        fi
    fi

    # Verify emcc is now available
    if ! command -v emcc &> /dev/null; then
        print_error "Emscripten compiler (emcc) not found after SDK setup."
        exit 1
    fi

    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found. Install CMake 3.20 or later."
        exit 1
    fi

    # Check Emscripten version
    EMCC_VERSION=$(emcc --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
    print_status "Using Emscripten version: $EMCC_VERSION"

    # Validate project structure
    if [[ ! -d "$SOURCE_DIR" ]]; then
        print_error "Source directory not found: $SOURCE_DIR"
        exit 1
    fi

    if [[ ! -d "$INCLUDE_DIR" ]]; then
        print_error "Include directory not found: $INCLUDE_DIR"
        exit 1
    fi

    print_success "Build environment validation complete"
}

# Configure build based on mode
configure_build_mode() {
    print_status "Configuring build for mode: $BUILD_MODE"

    case $BUILD_MODE in
        development)
            OPTIMIZATION_LEVEL="O1"
            ENABLE_DEBUGGING=true
            GENERATE_SOURCEMAPS=true
            CLOSURE_COMPILER=false
            ;;
        production)
            OPTIMIZATION_LEVEL="O3"
            ENABLE_DEBUGGING=false
            GENERATE_SOURCEMAPS=false
            CLOSURE_COMPILER=true
            ;;
        debug)
            OPTIMIZATION_LEVEL="O0"
            ENABLE_DEBUGGING=true
            ENABLE_PROFILING=true
            GENERATE_SOURCEMAPS=true
            ;;
    esac

    print_status "Optimization level: $OPTIMIZATION_LEVEL"
    print_status "Debugging: $([ "$ENABLE_DEBUGGING" = true ] && echo "enabled" || echo "disabled")"
    print_status "Source maps: $([ "$GENERATE_SOURCEMAPS" = true ] && echo "enabled" || echo "disabled")"
}

# Build WASM optimization flags
build_optimization_flags() {
    local flags=""

    # Base optimization
    flags="$flags -$OPTIMIZATION_LEVEL"

    # Memory optimization
    case $MEMORY_SIZE in
        *MB)
            local mb_size=${MEMORY_SIZE%MB}
            local bytes=$((mb_size * 1024 * 1024))
            flags="$flags -s INITIAL_MEMORY=${bytes}"
            ;;
        *GB)
            local gb_size=${MEMORY_SIZE%GB}
            local bytes=$((gb_size * 1024 * 1024 * 1024))
            flags="$flags -s INITIAL_MEMORY=${bytes}"
            ;;
    esac

    flags="$flags -s ALLOW_MEMORY_GROWTH=1"
    flags="$flags -s STACK_SIZE=$((64 * 1024))"

    # Threading support
    if [[ "$ENABLE_THREADING" == true ]]; then
        flags="$flags -s USE_PTHREADS=1"
        flags="$flags -s PTHREAD_POOL_SIZE=4"
    fi

    # SIMD support
    if [[ "$ENABLE_SIMD" == true ]]; then
        flags="$flags -msimd128"
        flags="$flags -s SIMD=1"
    fi

    # Debugging and profiling
    if [[ "$ENABLE_DEBUGGING" == true ]]; then
        flags="$flags -g"
        flags="$flags -s ASSERTIONS=1"
        flags="$flags -s SAFE_HEAP=1"
    fi

    if [[ "$ENABLE_PROFILING" == true ]]; then
        flags="$flags --profiling"
        flags="$flags -s PROFILING_FUNCS=1"
    fi

    # Source maps
    if [[ "$GENERATE_SOURCEMAPS" == true ]]; then
        flags="$flags -g4"
        flags="$flags --source-map-base http://localhost:8000/"
    fi

    # Closure compiler
    if [[ "$CLOSURE_COMPILER" == true ]]; then
        flags="$flags --closure 1"
        flags="$flags --closure-args=\"--compilation_level=ADVANCED_OPTIMIZATIONS\""
    fi

    # Advanced WASM features
    flags="$flags -s WASM=1"
    flags="$flags -s MODULARIZE=1"
    flags="$flags -s EXPORT_NAME='HuntmasterEngine'"
    flags="$flags -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    flags="$flags -s ALLOW_TABLE_GROWTH=1"
    flags="$flags -s NO_FILESYSTEM=1"

    # Audio-specific optimizations
    flags="$flags -s ENVIRONMENT='web'"
    flags="$flags -s AUDIO_WORKLET=1"

    echo "$flags"
}

# Configure conditional compilation flags
configure_conditional_compilation() {
    local defines=""

    # Build mode defines
    case $BUILD_MODE in
        development)
            defines="$defines -DDEBUG=1 -DDEVELOPMENT_BUILD=1"
            ;;
        production)
            defines="$defines -DNDEBUG=1 -DPRODUCTION_BUILD=1"
            ;;
        debug)
            defines="$defines -DDEBUG=1 -DDEBUG_BUILD=1 -DENABLE_LOGGING=1"
            ;;
    esac

    # Feature defines
    if [[ "$ENABLE_SIMD" == true ]]; then
        defines="$defines -DENABLE_SIMD=1"
    fi

    if [[ "$ENABLE_THREADING" == true ]]; then
        defines="$defines -DENABLE_THREADING=1"
    fi

    if [[ "$ENABLE_PROFILING" == true ]]; then
        defines="$defines -DENABLE_PROFILING=1"
    fi

    # Codec selection
    if [[ "$ENABLE_CODEC_SELECTION" == true ]]; then
        IFS=',' read -ra CODECS <<< "$SELECTED_CODECS"
        for codec in "${CODECS[@]}"; do
            case $codec in
                wav)
                    defines="$defines -DENABLE_WAV_CODEC=1"
                    ;;
                mp3)
                    defines="$defines -DENABLE_MP3_CODEC=1"
                    ;;
                ogg)
                    defines="$defines -DENABLE_OGG_CODEC=1"
                    ;;
                flac)
                    defines="$defines -DENABLE_FLAC_CODEC=1"
                    ;;
            esac
        done
    else
        # Enable all codecs by default
        defines="$defines -DENABLE_ALL_CODECS=1"
    fi

    echo "$defines"
}

# Setup build directory structure
setup_build_directory() {
    print_status "Setting up build directory structure..."

    mkdir -p "$OUTPUT_DIR"
    mkdir -p "$BUILD_DIR/cmake"
    mkdir -p "$BUILD_DIR/reports"
    mkdir -p "$BUILD_DIR/cache"

    if [[ "$ENABLE_MODULAR_BUILD" == true ]]; then
        mkdir -p "$OUTPUT_DIR/modules"
    fi

    print_success "Build directory structure created"
}

# Generate CMake configuration
generate_cmake_config() {
    print_status "Generating CMake configuration..."

    local cmake_file="$BUILD_DIR/cmake/WasmConfig.cmake"
    cat > "$cmake_file" << EOF
# WASM Build Configuration
# Generated by wasm_build_optimizer.sh

set(CMAKE_TOOLCHAIN_FILE "$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
set(CMAKE_BUILD_TYPE $BUILD_MODE)

# Optimization flags
set(WASM_OPTIMIZATION_FLAGS "$(build_optimization_flags)")
set(WASM_COMPILE_DEFINITIONS "$(configure_conditional_compilation)")

# Platform specific settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories("$INCLUDE_DIR")
include_directories("$SOURCE_DIR")

# Emscripten specific settings
set(CMAKE_EXECUTABLE_SUFFIX ".js")

# Build configuration
if(BUILD_MODE STREQUAL "production")
    set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")
elseif(BUILD_MODE STREQUAL "debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
endif()

# WASM specific compile options
set(CMAKE_CXX_FLAGS "\${CMAKE_CXX_FLAGS} \${WASM_OPTIMIZATION_FLAGS}")
set(CMAKE_CXX_FLAGS "\${CMAKE_CXX_FLAGS} \${WASM_COMPILE_DEFINITIONS}")

EOF

    print_success "CMake configuration generated: $cmake_file"
}

# Build the WASM module
build_wasm_module() {
    print_status "Building WASM module..."

    local start_time=$(date +%s)

    cd "$BUILD_DIR"

    # Configure with CMake
    cmake -DCMAKE_TOOLCHAIN_FILE="$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" \
          -DCMAKE_BUILD_TYPE="$BUILD_MODE" \
          -DENABLE_WASM=ON \
          -C "$BUILD_DIR/cmake/WasmConfig.cmake" \
          "$PROJECT_ROOT"

    # Build with parallel jobs
    make -j$PARALLEL_JOBS

    local end_time=$(date +%s)
    local build_duration=$((end_time - start_time))

    print_success "WASM module built in ${build_duration} seconds"

    # Copy outputs to web directory
    if [[ -f "$BUILD_DIR/huntmaster-engine.js" ]]; then
        cp "$BUILD_DIR/huntmaster-engine.js" "$WEB_DIR/"
        cp "$BUILD_DIR/huntmaster-engine.wasm" "$WEB_DIR/"

        if [[ "$GENERATE_SOURCEMAPS" == true && -f "$BUILD_DIR/huntmaster-engine.wasm.map" ]]; then
            cp "$BUILD_DIR/huntmaster-engine.wasm.map" "$WEB_DIR/"
        fi

        print_success "WASM artifacts copied to web directory"
    fi
}

# Generate TypeScript definitions
generate_typescript_definitions() {
    if [[ "$BUILD_MODE" != "production" ]]; then
        print_status "Generating TypeScript definitions..."

        # Call the TypeScript definition generator
        "$SCRIPT_DIR/generate_typescript_defs.sh" --input "$BUILD_DIR/huntmaster-engine.js" \
                                                  --output "$WEB_DIR/huntmaster-engine.d.ts"

        print_success "TypeScript definitions generated"
    fi
}

# Validate build artifacts
validate_build_artifacts() {
    if [[ "$ENABLE_VALIDATION" == true ]]; then
        print_status "Validating build artifacts..."

        # Call the validation script
        "$SCRIPT_DIR/validate_wasm_artifacts.sh" --build-dir "$BUILD_DIR" \
                                                --output-dir "$WEB_DIR"

        print_success "Build artifact validation complete"
    fi
}

# Generate build report
generate_build_report() {
    if [[ "$ENABLE_SIZE_REPORTING" == true ]]; then
        print_status "Generating build size report..."

        local report_file="$BUILD_DIR/reports/build_report_$(date +%Y%m%d_%H%M%S).json"

        # Get file sizes
        local js_size=$(stat -f%z "$WEB_DIR/huntmaster-engine.js" 2>/dev/null || stat -c%s "$WEB_DIR/huntmaster-engine.js" 2>/dev/null || echo "0")
        local wasm_size=$(stat -f%z "$WEB_DIR/huntmaster-engine.wasm" 2>/dev/null || stat -c%s "$WEB_DIR/huntmaster-engine.wasm" 2>/dev/null || echo "0")

        # Create JSON report
        cat > "$report_file" << EOF
{
    "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
    "build_mode": "$BUILD_MODE",
    "optimization_level": "$OPTIMIZATION_LEVEL",
    "artifacts": {
        "js_file": {
            "path": "huntmaster-engine.js",
            "size_bytes": $js_size,
            "size_kb": $((js_size / 1024))
        },
        "wasm_file": {
            "path": "huntmaster-engine.wasm",
            "size_bytes": $wasm_size,
            "size_kb": $((wasm_size / 1024))
        }
    },
    "configuration": {
        "simd_enabled": $ENABLE_SIMD,
        "threading_enabled": $ENABLE_THREADING,
        "debugging_enabled": $ENABLE_DEBUGGING,
        "profiling_enabled": $ENABLE_PROFILING,
        "closure_compiler": $CLOSURE_COMPILER,
        "memory_size": "$MEMORY_SIZE"
    },
    "total_size_kb": $(((js_size + wasm_size) / 1024))
}
EOF

        print_success "Build report generated: $report_file"

        # Print summary
        echo ""
        echo "==============================================="
        echo "          BUILD SIZE SUMMARY"
        echo "==============================================="
        echo "JavaScript: $(echo "scale=2; $js_size/1024" | bc 2>/dev/null || echo $((js_size/1024))) KB"
        echo "WebAssembly: $(echo "scale=2; $wasm_size/1024" | bc 2>/dev/null || echo $((wasm_size/1024))) KB"
        echo "Total: $(echo "scale=2; ($js_size+$wasm_size)/1024" | bc 2>/dev/null || echo $(((js_size+wasm_size)/1024))) KB"
        echo "==============================================="
    fi
}

# Performance benchmarking
run_performance_benchmark() {
    if [[ "$ENABLE_BENCHMARKING" == true ]]; then
        print_status "Running performance benchmarks..."

        # This would integrate with actual benchmarking tools
        local benchmark_results="$BUILD_DIR/reports/benchmark_$(date +%Y%m%d_%H%M%S).json"

        cat > "$benchmark_results" << EOF
{
    "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
    "build_mode": "$BUILD_MODE",
    "benchmark_results": {
        "module_load_time_ms": 0,
        "initialization_time_ms": 0,
        "memory_usage_bytes": 0
    }
}
EOF

        print_success "Performance benchmark complete: $benchmark_results"
    fi
}

# Cleanup build cache if needed
cleanup_build_cache() {
    if [[ "$BUILD_CACHE_ENABLED" != true ]]; then
        print_status "Cleaning build cache..."
        rm -rf "$BUILD_DIR/cache"
        print_success "Build cache cleaned"
    fi
}

# Main execution flow
main() {
    echo "==============================================="
    echo "    WASM Build Optimizer - Huntmaster Engine"
    echo "==============================================="
    echo ""

    parse_arguments "$@"
    validate_environment
    configure_build_mode
    setup_build_directory
    generate_cmake_config
    build_wasm_module
    generate_typescript_definitions
    validate_build_artifacts
    generate_build_report
    run_performance_benchmark
    cleanup_build_cache

    echo ""
    echo "==============================================="
    echo "           BUILD COMPLETED SUCCESSFULLY"
    echo "==============================================="
    echo "Build mode: $BUILD_MODE"
    echo "Output directory: $WEB_DIR"
    echo "Build artifacts:"
    echo "  - huntmaster-engine.js"
    echo "  - huntmaster-engine.wasm"
    if [[ "$GENERATE_SOURCEMAPS" == true ]]; then
        echo "  - huntmaster-engine.wasm.map"
    fi
    if [[ "$BUILD_MODE" != "production" ]]; then
        echo "  - huntmaster-engine.d.ts"
    fi
    echo "==============================================="
}

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
# [ ] Add source map generation for debugging builds
# [ ] Create DWARF debug info management for development
# [ ] Implement WebAssembly interface types integration
# [ ] Add WASI (WebAssembly System Interface) support preparation
# [ ] Create multi-target compilation (different optimization levels)
# [ ] Add build environment containerization support

# TODO 1.1.4: Integration with Main Build System
# ----------------------------------------------
# [ ] Integrate with main build_wasm.sh script
# [ ] Add command-line parameter parsing and validation
# [ ] Create build mode detection and automatic optimization selection
# [ ] Implement build artifact organization and cleanup
# [ ] Add build logging and error reporting enhancements
# [ ] Create build configuration file support (JSON/YAML config)
# [ ] Implement build hook system for custom processing steps
# [ ] Add build notification system for CI/CD integration
# [ ] Create build metrics export for external monitoring
# [ ] Add build environment validation and requirements checking

# TODO 1.1.5: Testing and Validation Framework
# --------------------------------------------
# [ ] Create WASM build validation test suite
# [ ] Implement build artifact integrity testing
# [ ] Add performance regression detection for builds
# [ ] Create cross-browser WASM compatibility testing
# [ ] Implement build size threshold monitoring and alerting
# [ ] Add memory usage profiling for different build configurations
# [ ] Create automated build quality metrics collection
# [ ] Implement build comparison tools for optimization analysis
# [ ] Add build reproducibility testing across different environments
# [ ] Create build documentation generation and maintenance

# TODO 1.1.6: Documentation and Maintenance
# -----------------------------------------
# [ ] Create comprehensive build configuration documentation
# [ ] Add inline documentation for all build parameters and flags
# [ ] Create troubleshooting guide for common build issues
# [ ] Add performance tuning guide for different use cases
# [ ] Create build system architecture documentation
# [ ] Add migration guide for existing build configurations
# [ ] Create build system testing and validation procedures
# [ ] Add build system monitoring and alerting setup
# [ ] Create build system maintenance and update procedures
# [ ] Add build system security and vulnerability management

echo "WASM Build Optimizer - Advanced build configuration system"
echo "=========================================================="

# Color codes for output formatting
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly PURPLE='\033[0;35m'
readonly CYAN='\033[0;36m'
readonly NC='\033[0m' # No Color

# Build mode configuration
BUILD_MODE="development"
OPTIMIZATION_LEVEL="default"
ENABLE_DEBUGGING=true
ENABLE_PROFILING=false
ENABLE_SIZE_OPTIMIZATION=false
ENABLE_CODECS=false

# Memory configuration
INITIAL_MEMORY="16MB"
MAXIMUM_MEMORY="256MB"
ALLOW_MEMORY_GROWTH=true

# Output configuration
OUTPUT_FORMAT="modular"
ENABLE_SOURCE_MAPS=false
ENABLE_DWARF_DEBUG=false

# Performance configuration
ENABLE_SIMD=false
ENABLE_THREADS=false
ENABLE_BULK_MEMORY=true

echo -e "${YELLOW}TODO: This file needs complete implementation for Phase 1.1${NC}"
echo -e "${CYAN}Current status: Template with comprehensive TODO structure${NC}"
echo -e "${BLUE}Next steps: Implement each TODO section systematically${NC}"

exit 0
