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

# KissFFT (Vendored Submodule) - Core dependency for FFT calculations
if(EMSCRIPTEN)
    # For WASM, we only need the headers. We create an INTERFACE library
    # to propagate the include directory and definitions.
    message(STATUS "Configuring KissFFT as header-only for WASM")
    add_library(kissfft INTERFACE)
    target_include_directories(kissfft INTERFACE
        ${PROJECT_LIBS_DIR}/kissfft
        ${PROJECT_LIBS_DIR}/kissfft/tools
    )
    target_compile_definitions(kissfft INTERFACE kiss_fft_scalar=float)
else()
    # For native builds, we compile KissFFT as a static library.
    message(STATUS "Configuring KissFFT as static library for native")
    add_library(kissfft STATIC
        ${PROJECT_LIBS_DIR}/kissfft/kiss_fft.c
        ${PROJECT_LIBS_DIR}/kissfft/tools/kiss_fftr.c
        ${PROJECT_LIBS_DIR}/kissfft/kiss_fftnd.c
        ${PROJECT_LIBS_DIR}/kissfft/kiss_fftndr.c
    )
    target_include_directories(kissfft PUBLIC
        ${PROJECT_LIBS_DIR}/kissfft
        ${PROJECT_LIBS_DIR}/kissfft/tools
    )
    target_compile_definitions(kissfft PRIVATE kiss_fft_scalar=float)
endif()

# Google Test & Benchmark (Only for native testing)
if(HUNTMASTER_BUILD_TESTS AND NOT EMSCRIPTEN)
    message(STATUS "Configuring Google Test and Benchmark...")

    # Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY "https://github.com/google/googletest.git"
        GIT_TAG        "v1.14.0"
        SOURCE_DIR     "${PROJECT_TESTS_DIR}/lib/googletest"
    )
    FetchContent_MakeAvailable(googletest)

    # Google Benchmark
    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_INSTALL OFF)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY "https://github.com/google/benchmark.git"
        GIT_TAG        "v1.8.3"
        SOURCE_DIR     "${PROJECT_TESTS_DIR}/lib/benchmark"
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
