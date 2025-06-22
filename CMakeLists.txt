# ==============================================================================
# CMakeLists.txt for Huntmaster Audio Engine (v2.3)
#
# This file defines the build system for the huntmaster-engine library,
# integrating the MFCCProcessor and KissFFT dependency.
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


# Define the project name, version, and language.
project(
    HuntmasterEngine
    VERSION 2.3
    LANGUAGES CXX C
)

# --- Fix for Mixed Compiler Flags ---
if(MINGW)
    message(STATUS "MinGW detected. Cleaning up C and CXX flags.")
    string(REPLACE "/DWIN32" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE "/D_WINDOWS" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE "/W3" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE "/D_DEBUG" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

    string(REPLACE "/DWIN32" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/D_WINDOWS" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/W3" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/D_DEBUG" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(PROJECT_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(PROJECT_TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools)


if(EXISTS ${PROJECT_LIBS_DIR}/kissfft/CMakeLists.txt)
    add_subdirectory(${PROJECT_LIBS_DIR}/kissfft)
    message(STATUS "Kiss FFT: Found and configured.")
    add_compile_definitions(HAVE_KISSFFT)
else()
    message(WARNING "Kiss FFT not found in libs/kissfft. Please run setup_kissfft.sh or clone manually.")
endif()

include(FetchContent)
cmake_policy(SET CMP0135 NEW)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


set(ENGINE_SOURCES
    "${PROJECT_SOURCE_DIR}/HuntmasterAudioEngine.cpp"
    "${PROJECT_SOURCE_DIR}/MFCCProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/DTWProcessor.cpp"
)

add_library(
    HuntmasterEngine STATIC
    ${ENGINE_SOURCES}
)

target_include_directories(
    HuntmasterEngine
    PUBLIC
        ${PROJECT_INCLUDE_DIR}
)

target_link_libraries(
    HuntmasterEngine
    PUBLIC
        kissfft
)


# --- Test Harness Executable ---
message(STATUS "Configuring Test Harness executable...")
add_executable(
    TestHarness
    "${PROJECT_TOOLS_DIR}/test_harness.cpp"
)

# --- FIX: Add include directories for the TestHarness ---
# This allows it to find "huntmaster_engine/..." and the "dr_wav.h" headers.
target_include_directories(
    TestHarness
    PRIVATE
        ${PROJECT_INCLUDE_DIR}
        ${PROJECT_LIBS_DIR}
)

target_link_libraries(
    TestHarness
    PRIVATE HuntmasterEngine
)


# --- Unit Testing Suite ---
message(STATUS "Configuring Unit Tests...")
enable_testing()

add_executable(
    RunEngineTests
    "${PROJECT_TESTS_DIR}/engine_tests.cpp"
    "${PROJECT_TESTS_DIR}/mfcc_tests.cpp"
)

target_link_libraries(
    RunEngineTests
    PRIVATE HuntmasterEngine GTest::gtest GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(RunEngineTests)



# --- Feature Generator Tool ---
add_executable(
    GenerateFeatures
    tools/generate_features.cpp
)
target_include_directories(
    GenerateFeatures
    PRIVATE
        ${PROJECT_INCLUDE_DIR}
        ${PROJECT_LIBS_DIR}
)
target_link_libraries(
    GenerateFeatures
    PRIVATE HuntmasterEngine
)
message(STATUS "Huntmaster Engine configured successfully.")
message(STATUS "Available targets: HuntmasterEngine, TestHarness, RunEngineTests")

