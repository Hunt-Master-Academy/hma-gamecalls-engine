# ==============================================================================
# CMakeLists.txt for Huntmaster Audio Engine (v4.1 - Modular)
#
# This is the main CMake file that sets up project-wide configurations
# and delegates specific build tasks to subdirectories.
#
# To build for NATIVE:
#   cmake -B build
#   cmake --build build
#
# To build for WEB ASSEMBLY (with Emscripten SDK):
#   emcmake cmake -B build-wasm
#   cmake --build build-wasm
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

project(
    HuntmasterEngine
    VERSION 4.1
    LANGUAGES CXX C
)

# --- Set C++ Standard Globally ---
# Modern C++ is used across the project for both native and WASM builds.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Security Compilation Flags ---
# Add security enhancements for production builds
if(NOT EMSCRIPTEN)
    # Stack protection and format string security
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security")

    # Linker security flags for RELRO (Read-Only Relocations) and PIE (Position Independent Executable)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,relro,-z,now -pie")

    message(STATUS "Security compilation flags enabled")
endif()

# --- Project Structure Variables ---
# Define paths to key directories to keep the script clean and maintainable.
set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_SOURCE_DIR ${PROJECT_ROOT_DIR}/src)
set(PROJECT_INCLUDE_DIR ${PROJECT_ROOT_DIR}/include)
set(PROJECT_LIBS_DIR ${PROJECT_ROOT_DIR}/libs)
set(PROJECT_TESTS_DIR ${PROJECT_ROOT_DIR}/tests)
set(PROJECT_TOOLS_DIR ${PROJECT_ROOT_DIR}/tools)
set(PROJECT_WEB_DIR ${PROJECT_ROOT_DIR}/web)

# --- Build Options ---
# Provide options to control the build process.
option(HUNTMASTER_BUILD_TESTS "Enable building of unit tests and benchmarks" ON)
option(HUNTMASTER_BUILD_TOOLS "Enable building of command-line tools" ON)

# --- Output Directory Configuration ---
# Set runtime output directories to organize build artifacts
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# For multi-config generators (Visual Studio, Xcode), set per-config directories
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
endforeach()

# --- Advanced I/O Optimizer Configuration ---
# Include advanced I/O optimization features
if(NOT EMSCRIPTEN)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/AdvancedIOOptimizer.cmake")
    message(STATUS "Advanced I/O Optimizer features enabled")
endif()

# --- Platform Detection and Configuration ---
# Set preprocessor definitions based on the target platform.
if(EMSCRIPTEN)
    add_compile_definitions(HUNTMASTER_PLATFORM_WASM)
elseif(ANDROID)
    add_compile_definitions(HUNTMASTER_PLATFORM_ANDROID)
elseif(IOS)
    add_compile_definitions(HUNTMASTER_PLATFORM_IOS)
elseif(WIN32)
    add_compile_definitions(HUNTMASTER_PLATFORM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(HUNTMASTER_PLATFORM_MACOS)
elseif(UNIX AND NOT ANDROID)
    add_compile_definitions(HUNTMASTER_PLATFORM_LINUX)
endif()

# --- Third-Party Dependencies ---
include(FetchContent)

# KissFFT - Core dependency for FFT calculations
message(STATUS "Configuring KissFFT...")
FetchContent_Declare(
    kissfft
    GIT_REPOSITORY "https://github.com/mborgerding/kissfft.git"
    GIT_TAG        "v131"  # Use stable release tag
)
FetchContent_MakeAvailable(kissfft)

# FFTW3 - Optional dependency for advanced waveform analysis
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(FFTW3 fftw3)
endif()

if(NOT FFTW3_FOUND)
    find_path(FFTW3_INCLUDE_DIR fftw3.h)
    find_library(FFTW3_LIBRARY fftw3)
    if(FFTW3_INCLUDE_DIR AND FFTW3_LIBRARY)
        set(FFTW3_FOUND TRUE)
        message(STATUS "Found FFTW3: ${FFTW3_LIBRARY}")
    else()
        message(STATUS "FFTW3 not found - WaveformAnalyzer will use stub implementation")
        add_compile_definitions(FFTW_DISABLE)
    endif()
endif()

# Configure KissFFT library
if(EMSCRIPTEN)
    # For WASM, we only need the headers. We create an INTERFACE library
    # to propagate the include directory and definitions.
    message(STATUS "Configuring KissFFT as header-only for WASM")
    add_library(kissfft_huntmaster INTERFACE)
    target_include_directories(kissfft_huntmaster INTERFACE
        ${kissfft_SOURCE_DIR}
        ${kissfft_SOURCE_DIR}/tools
    )
    target_compile_definitions(kissfft_huntmaster INTERFACE kiss_fft_scalar=float)
    add_library(kissfft ALIAS kissfft_huntmaster)
else()
    # For native builds, we compile KissFFT as a static library.
    message(STATUS "Configuring KissFFT as static library for native")

    # Check which source files actually exist
    set(KISSFFT_SOURCES
        ${kissfft_SOURCE_DIR}/kiss_fft.c
        ${kissfft_SOURCE_DIR}/tools/kiss_fftr.c
    )

    # Add optional source files if they exist
    if(EXISTS ${kissfft_SOURCE_DIR}/kiss_fftnd.c)
        list(APPEND KISSFFT_SOURCES ${kissfft_SOURCE_DIR}/kiss_fftnd.c)
    endif()
    if(EXISTS ${kissfft_SOURCE_DIR}/kiss_fftndr.c)
        list(APPEND KISSFFT_SOURCES ${kissfft_SOURCE_DIR}/kiss_fftndr.c)
    endif()

    add_library(kissfft_huntmaster STATIC ${KISSFFT_SOURCES})
    target_include_directories(kissfft_huntmaster PUBLIC
        ${kissfft_SOURCE_DIR}
        ${kissfft_SOURCE_DIR}/tools
    )
    target_compile_definitions(kissfft_huntmaster PRIVATE kiss_fft_scalar=float)
    add_library(kissfft ALIAS kissfft_huntmaster)
endif()

# Google Test & Benchmark (Only for native testing)
if(HUNTMASTER_BUILD_TESTS AND NOT EMSCRIPTEN)
    message(STATUS "Configuring Google Test and Benchmark...")

    # Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY "https://github.com/google/googletest.git"
        GIT_TAG        "v1.14.0"
    )
    FetchContent_MakeAvailable(googletest)

    # Google Benchmark
    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_INSTALL OFF)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY "https://github.com/google/benchmark.git"
        GIT_TAG        "v1.8.3"
    )
    FetchContent_MakeAvailable(benchmark)
else()
    message(STATUS "Skipping Google Test and Benchmark configuration.")
endif()

# ==============================================================================
# Add Subdirectories - Delegate specific build tasks
# ==============================================================================

# Build the core engine library
add_subdirectory(src)

# Add tests (if enabled and not building for WASM)
if(HUNTMASTER_BUILD_TESTS AND NOT EMSCRIPTEN)
    add_subdirectory(tests)
endif()

# Add tools (if enabled and not building for WASM)
if(HUNTMASTER_BUILD_TOOLS AND NOT EMSCRIPTEN)
    add_subdirectory(tools)
endif()

# --- Final Configuration Message ---
message(STATUS "==================================================")
message(STATUS "Huntmaster Engine Configuration Summary:")
message(STATUS "  Version:      ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
if(EMSCRIPTEN)
    message(STATUS "  Platform:     WebAssembly (Emscripten)")
    message(STATUS "  Output:       ${PROJECT_WEB_DIR}/dist/")
else()
    message(STATUS "  Platform:     Native")
    message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
    message(STATUS "  Build Tools:  ${HUNTMASTER_BUILD_TOOLS}")
    message(STATUS "  Build Tests:  ${HUNTMASTER_BUILD_TESTS}")
endif()
message(STATUS "==================================================")
