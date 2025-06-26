# ==============================================================================
# CMakeLists.txt for Huntmaster Audio Engine (v2.4)
#
# This file defines the build system for the huntmaster-engine library,
# its dependencies, and associated testing tools.
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# --- Conda Environment Compiler Detection (Robust Version) ---
if(DEFINED ENV{CONDA_PREFIX})
    set(CONDA_ENV_PATH "$ENV{CONDA_PREFIX}")
    message(STATUS "Conda environment detected at: ${CONDA_ENV_PATH}")

    find_program(
        CONDA_CXX_COMPILER
        NAMES g++.exe g++ x86_64-w64-mingw32-g++
        HINTS "${CONDA_ENV_PATH}/Library/mingw-w64/bin"
              "${CONDA_ENV_PATH}/bin"
        NO_DEFAULT_PATH
    )
    find_program(
        CONDA_C_COMPILER
        NAMES gcc.exe gcc x86_64-w64-mingw32-gcc
        HINTS "${CONDA_ENV_PATH}/Library/mingw-w64/bin"
              "${CONDA_ENV_PATH}/bin"
        NO_DEFAULT_PATH
    )

    if(CONDA_CXX_COMPILER AND CONDA_C_COMPILER)
        message(STATUS "Found Conda CXX compiler: ${CONDA_CXX_COMPILER}")
        message(STATUS "Found Conda C compiler:  ${CONDA_C_COMPILER}")
        set(CMAKE_CXX_COMPILER "${CONDA_CXX_COMPILER}")
        set(CMAKE_C_COMPILER "${CONDA_C_COMPILER}")
    else()
        message(WARNING "Conda environment is active, but the MinGW toolchain (gcc/g++) was not found. Please install with 'conda install -c conda-forge m2w64-toolchain'.")
    endif()
else()
    message(STATUS "No active Conda environment detected. Using system default compilers.")
endif()


# --- Project Definition ---
project(
    HuntmasterEngine
    VERSION 2.4
    LANGUAGES CXX C
)

# --- Compiler and Standard Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Fix for Mixed Compiler Flags on MinGW ---
if(MINGW)
    message(STATUS "MinGW detected. Cleaning up incompatible MSVC flags.")
    foreach(flag_var CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
        string(REPLACE "/DWIN32" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/D_WINDOWS" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/W3" "" ${flag_var} "${${flag_var}}")
        string(REPLACE "/D_DEBUG" "" ${flag_var} "${${flag_var}}")
    endforeach()
endif()


# --- Project Structure Variables ---
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(PROJECT_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(PROJECT_TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools)


# --- Third-Party Dependencies ---
# 1. KissFFT
if(EXISTS ${PROJECT_LIBS_DIR}/kissfft/CMakeLists.txt)
    add_subdirectory(${PROJECT_LIBS_DIR}/kissfft)
    message(STATUS "Kiss FFT: Found and configured.") # HAVE_KISSFFT will be added to targets below
else()
    message(WARNING "Kiss FFT not found in libs/kissfft. Please clone it into the libs directory.")
endif()

# 2. Google Test
# Using FetchContent is a great modern approach for handling dependencies.
include(FetchContent)
cmake_policy(SET CMP0135 NEW)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


# --- Core Engine Library ---
set(ENGINE_SOURCES
    "${PROJECT_SOURCE_DIR}/HuntmasterAudioEngine.cpp"
    "${PROJECT_SOURCE_DIR}/MFCCProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/DTWProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/AudioRecorder.cpp"
    "${PROJECT_SOURCE_DIR}/AudioPlayer.cpp"
    # CRITICAL FIX: Add the new file containing third-party library implementations.
    # This resolves the "undefined reference" linker errors for miniaudio, dr_wav, etc.
    "${PROJECT_SOURCE_DIR}/ThirdPartyLibs.cpp"
)

add_library(HuntmasterEngine STATIC ${ENGINE_SOURCES})

target_include_directories(HuntmasterEngine PUBLIC ${PROJECT_INCLUDE_DIR})
target_compile_definitions(HuntmasterEngine PUBLIC HAVE_KISSFFT) # Modern way to add definitions
target_link_libraries(HuntmasterEngine PUBLIC kissfft)


# --- Executable Targets (Tests and Tools) ---
# Common include directories for all tools
set(TOOL_INCLUDE_DIRS
    PRIVATE
    ${PROJECT_LIBS_DIR}
)

# 1. Main Test Harness
message(STATUS "Configuring Test Harness executable...")
add_executable(TestHarness "${PROJECT_TOOLS_DIR}/test_harness.cpp")
# FIX: Link against HuntmasterEngine to inherit its include paths.
# Add libs directory for direct includes of miniaudio.h, etc.
target_include_directories(TestHarness PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(TestHarness PRIVATE HuntmasterEngine)

# 2. Recording Test Tool
message(STATUS "Configuring Recording Test executable...")
add_executable(TestRecording "${PROJECT_TOOLS_DIR}/test_recording.cpp")
target_include_directories(TestRecording PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(TestRecording PRIVATE HuntmasterEngine)

# 3. Feature Generation Tool
message(STATUS "Configuring Feature Generation tool...")
add_executable(GenerateFeatures "${PROJECT_TOOLS_DIR}/generate_features.cpp")
target_include_directories(GenerateFeatures PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(GenerateFeatures PRIVATE HuntmasterEngine)

# 4. Unit Testing Suite
message(STATUS "Configuring Unit Tests...")
enable_testing()
add_executable(
    RunEngineTests
    "${PROJECT_TESTS_DIR}/engine_tests.cpp"
    "${PROJECT_TESTS_DIR}/mfcc_tests.cpp"
)
# Test target also needs access to the libs directory for its own includes.
target_include_directories(RunEngineTests PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(RunEngineTests PRIVATE HuntmasterEngine GTest::gtest GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(RunEngineTests)

# 5. Interactive Recording Test
message(STATUS "Configuring Interactive Recorder...")
add_executable(InteractiveRecorder "${PROJECT_TOOLS_DIR}/interactive_recorder.cpp")
target_include_directories(InteractiveRecorder PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(InteractiveRecorder PRIVATE HuntmasterEngine)

# Recording Analyzer Tool
add_executable(AnalyzeRecording "${PROJECT_TOOLS_DIR}/analyze_recording.cpp")
target_include_directories(AnalyzeRecording PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(AnalyzeRecording PRIVATE HuntmasterEngine)

# Find Best Match Tool
add_executable(FindBestMatch "${PROJECT_TOOLS_DIR}/find_best_match.cpp")
target_include_directories(FindBestMatch PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(FindBestMatch PRIVATE HuntmasterEngine)

# Audio Visualization Tool
add_executable(AudioVisualization "${PROJECT_TOOLS_DIR}/audio_visualization.cpp")
target_include_directories(AudioVisualization PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(AudioVisualization PRIVATE HuntmasterEngine)

# Audio Trimming Tool
add_executable(AudioTrimmer "${PROJECT_TOOLS_DIR}/audio_trimmer.cpp")
target_include_directories(AudioTrimmer PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(AudioTrimmer PRIVATE HuntmasterEngine)

# Real-Time Recording Monitor
add_executable(RealTimeMonitor "${PROJECT_TOOLS_DIR}/real_time_recording_monitor.cpp")
target_include_directories(RealTimeMonitor PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(RealTimeMonitor PRIVATE HuntmasterEngine)

# Performance Test
message(STATUS "Configuring Performance Test...")
add_executable(TestPerformance "${PROJECT_TOOLS_DIR}/test_performance.cpp")
target_include_directories(TestPerformance PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(TestPerformance PRIVATE HuntmasterEngine)

# Link Windows-specific libraries for memory monitoring
if(WIN32)
    target_link_libraries(TestPerformance PRIVATE psapi)
endif()

# MFCC Consistency Test
message(STATUS "Configuring MFCC Consistency Test...")
add_executable(TestMFCCConsistency "${PROJECT_TOOLS_DIR}/test_mfcc_consistency.cpp")
target_include_directories(TestMFCCConsistency PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(TestMFCCConsistency PRIVATE HuntmasterEngine)

# Cross-Platform Consistency Test
message(STATUS "Configuring Cross-Platform Test...")
add_executable(TestCrossPlatform "${PROJECT_TOOLS_DIR}/test_cross_platform.cpp")
target_include_directories(TestCrossPlatform PRIVATE ${PROJECT_LIBS_DIR})
target_link_libraries(TestCrossPlatform PRIVATE HuntmasterEngine)

# --- Final Configuration Message ---
message(STATUS "Huntmaster Engine configured successfully.")
message(STATUS "Available targets: HuntmasterEngine, TestHarness, TestRecording, GenerateFeatures, RunEngineTests")
