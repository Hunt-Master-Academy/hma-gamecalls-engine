# ==============================================================================
# CMakeLists.txt for Huntmaster Audio Engine (v2.0)
#
# This file defines the build system for the huntmaster-engine library,
# integrating the MFCCProcessor and KissFFT dependency.
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

# --- Conda Environment Compiler Detection (Robust Version) ---
# This block makes the build robust by automatically finding the compiler
# from the active Conda environment. This should be defined before project().
if(DEFINED ENV{CONDA_PREFIX})
    set(CONDA_ENV_PATH "$ENV{CONDA_PREFIX}")
    message(STATUS "Conda environment detected at: ${CONDA_ENV_PATH}")

    # Use find_program to search for the compiler within the Conda environment.
    find_program(
        CONDA_CXX_COMPILER
        NAMES g++.exe g++ x86_64-w64-mingw32-g++
        HINTS "${CONDA_ENV_PATH}/Library/mingw-w64/bin"
              "${CONDA_ENV_PATH}/bin"
              "${CONDA_ENV_PATH}/Scripts"
        NO_DEFAULT_PATH
    )
    if(CONDA_CXX_COMPILER)
        message(STATUS "Found Conda CXX compiler: ${CONDA_CXX_COMPILER}")
        set(CMAKE_CXX_COMPILER "${CONDA_CXX_COMPILER}")
    else()
        message(WARNING "Conda environment is active, but g++.exe was not found. Please install with 'conda install -c conda-forge m2w64-toolchain'.")
    endif()
else()
    message(STATUS "No active Conda environment detected. Using system default compilers.")
endif()


# Define the project name, version, and language.
project(
    HuntmasterEngine
    VERSION 2.0
    LANGUAGES CXX
)

# Set the C++ standard to C++17 and enforce it.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# --- Project Structure Variables ---
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(PROJECT_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(PROJECT_TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools)


# --- Third-Party Dependencies ---

# 1. KissFFT
# Add the kissfft subdirectory. This will process its own CMakeLists.txt
# and make the 'kissfft' library target available to our project.
if(EXISTS ${PROJECT_LIBS_DIR}/kissfft/CMakeLists.txt)
    add_subdirectory(${PROJECT_LIBS_DIR}/kissfft)
    message(STATUS "Kiss FFT: Found and configured.")
    # Add a compile definition so our code knows KissFFT is available.
    add_compile_definitions(HAVE_KISSFFT)
else()
    message(WARNING "Kiss FFT not found in libs/kissfft. Please run setup_kissfft.sh or clone manually.")
endif()

# 2. Google Test
include(FetchContent)
cmake_policy(SET CMP0135 NEW)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/v1.14.0.zip
)
# For Windows, ensure we build gtest as a static library
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


# --- Core Engine Library ---
set(ENGINE_SOURCES
    "${PROJECT_SOURCE_DIR}/HuntmasterAudioEngine.cpp"
    "${PROJECT_SOURCE_DIR}/MFCCProcessor.cpp" # <-- Add new MFCCProcessor source file
)

add_library(
    HuntmasterEngine STATIC
    ${ENGINE_SOURCES}
)

# Define the library's public include directories.
target_include_directories(
    HuntmasterEngine
    PUBLIC
        ${PROJECT_INCLUDE_DIR}
)

# Link the engine library against its dependencies (KissFFT).
# Any executable that links to HuntmasterEngine will now also get KissFFT.
target_link_libraries(
    HuntmasterEngine
    PUBLIC
        kissfft # <-- Link against the KissFFT library
)


# --- Test Harness Executable ---
message(STATUS "Configuring Test Harness executable...")
add_executable(
    TestHarness
    "${PROJECT_TOOLS_DIR}/test_harness.cpp"
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
    "${PROJECT_TESTS_DIR}/mfcc_tests.cpp" # <-- Add new MFCC test file
)

target_link_libraries(
    RunEngineTests
    PRIVATE HuntmasterEngine GTest::gtest GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(RunEngineTests)


# --- Build Output ---
message(STATUS "Huntmaster Engine configured successfully.")
message(STATUS "Available targets: HuntmasterEngine, TestHarness, RunEngineTests")

