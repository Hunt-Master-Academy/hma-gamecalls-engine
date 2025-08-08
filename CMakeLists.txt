# ==============================================================================
# CMakeLists.txt - Huntmaster Audio Engine
# Main project configuration file
# ==============================================================================

cmake_minimum_required(VERSION 3.20)
project(HuntmasterEngine VERSION 1.0.0 LANGUAGES CXX)

# ==============================================================================
# Project Configuration
# ==============================================================================

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release build if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ==============================================================================
# Global Variables and Paths
# ==============================================================================

# Define project directories
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PROJECT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
set(PROJECT_TOOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tools")
set(PROJECT_WEB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/web")
set(PROJECT_LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")

# Include path for the header files
include_directories(${PROJECT_INCLUDE_DIR})

# ==============================================================================
# Compiler Options
# ==============================================================================

# Add compiler-specific options
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# ==============================================================================
# Dependencies Management (FetchContent)
# ==============================================================================

include(FetchContent)

# Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

# Google Benchmark
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.7.1
)

# KissFFT
FetchContent_Declare(
    kissfft
    GIT_REPOSITORY https://github.com/mborgerding/kissfft.git
    GIT_TAG master
)

# Disable benchmark testing and KissFFT tools
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(KISSFFT_TOOLS OFF CACHE BOOL "" FORCE)
set(KISSFFT_TEST OFF CACHE BOOL "" FORCE)

# Make dependencies available
FetchContent_MakeAvailable(googletest benchmark kissfft)# ==============================================================================
# Platform Detection
# ==============================================================================

# Detect if we're building for WebAssembly
if(EMSCRIPTEN)
    message(STATUS "Building for WebAssembly (Emscripten)")
    set(BUILDING_FOR_WASM TRUE)
else()
    message(STATUS "Building for native platform")
    set(BUILDING_FOR_WASM FALSE)
endif()

# ==============================================================================
# Add Subdirectories
# ==============================================================================

# Add source directory (core library)
add_subdirectory(src)

# Add tests directory (if not building for WASM)
if(NOT BUILDING_FOR_WASM)
    add_subdirectory(tests)
    add_subdirectory(tools)
endif()

# KissFFT is now available via FetchContent

# ==============================================================================
# Installation (Optional)
# ==============================================================================

# Install headers
install(DIRECTORY ${PROJECT_INCLUDE_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# ==============================================================================
# Summary
# ==============================================================================

message(STATUS "=== Huntmaster Engine Configuration Summary ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Building for WASM: ${BUILDING_FOR_WASM}")
message(STATUS "Project source: ${PROJECT_SOURCE_DIR}")
message(STATUS "Project include: ${PROJECT_INCLUDE_DIR}")
message(STATUS "===============================================")
