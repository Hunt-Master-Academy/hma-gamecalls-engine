# ==============================================================================
# CMakeLists.txt for Huntmaster Audio Engine (v4.1 - Unified)
#
# This script handles both NATIVE builds (for testing tools) and
# WEB ASSEMBLY builds (for the web app) from a single, unified file.
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
# Provide options to controoption(HUNTMASTER_BUILD_TESTS "Enable building of unit tests and benchmarks" ON)
option(HUNTMASTER_BUILD_TOOLS "Enable building of command-line tools" ON)

# This is useful for faster configuration and builds when tools or tests are not needed.
option(HUNTMASTER_BUILD_TESTS "Enable building of unit tests and benchmarks" ON)
option(HUNTMASTER_BUILD_TOOLS "Enable building of command-line tools" ON)


# --- Platform Detection and Configuration ---
# Set preprocessor definitions based on the target platform.
if(EMSCRIPTEN)
    add_compile_definitions(HUNTMASTER_PLATFORM_WASM)
    # REPLACED: These definitions are now handled by specific configurations if needed.
    # add_compile_definitions(HUNTMASTER_SINGLE_THREADED)
    # add_compile_definitions(HUNTMASTER_NO_DIRECT_AUDIO)
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

# 1. KissFFT (Vendored Submodule)
# KissFFT is a core dependency for FFT calculations.
if(EMSCRIPTEN)
    # For WASM, we only need the headers. We create an INTERFACE library
    # to propagate the include directory and definitions.
    message(STATUS "Configuring KissFFT as header-only for WASM")
    add_library(kissfft INTERFACE)
    target_include_directories(kissfft INTERFACE ${PROJECT_LIBS_DIR}/kissfft)
    target_compile_definitions(kissfft INTERFACE kiss_fft_scalar=float)
else()
    # For native builds, we compile KissFFT as a static library.
    # We disable its tools and tests to avoid pulling in extra dependencies.
    add_library(kissfft STATIC
        ${PROJECT_LIBS_DIR}/kissfft/kiss_fft.c
        ${PROJECT_LIBS_DIR}/kissfft/tools/kiss_fftr.c
        ${PROJECT_LIBS_DIR}/kissfft/kiss_fftnd.c
        ${PROJECT_LIBS_DIR}/kissfft/kiss_fftndr.c
    )
    # The library needs its own include directory to find its headers.
    # This is made PUBLIC so that targets linking to kissfft also get the path.
    target_include_directories(kissfft PUBLIC
        ${PROJECT_LIBS_DIR}/kissfft
        ${PROJECT_LIBS_DIR}/kissfft/tools
    )
    # Set the data type for the FFT calculations. This only affects the compilation of kissfft.
    target_compile_definitions(kissfft PRIVATE kiss_fft_scalar=float)
endif()
# 2. Google Test & Benchmark (Fetched at configure time for native builds)
# These are only needed for native testing, not for the final WASM product.
if(HUNTMASTER_BUILD_TESTS AND NOT EMSCRIPTEN)
    message(STATUS "Configuring Google Test and Benchmark...")

    # Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY "https://github.com/google/googletest.git"
        GIT_TAG        "v1.14.0" # Using a specific, stable tag
        SOURCE_DIR     "${PROJECT_TESTS_DIR}/lib/googletest"
    )
    FetchContent_MakeAvailable(googletest)

    # Google Benchmark
    set(BENCHMARK_ENABLE_TESTING OFF) # Disable benchmark's own tests
    set(BENCHMARK_ENABLE_INSTALL OFF) # Disable benchmark's install step
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
# Core Engine Library
# This target contains all platform-agnostic engine code and is shared
# between the native and WASM builds.
# ==============================================================================
message(STATUS "Configuring core HuntmasterEngine library...")

# Define the list of core source files once to avoid duplication.
# set(HUNTMASTER_CORE_SOURCES
#     "${PROJECT_SOURCE_DIR}/core/HuntmasterEngine.cpp"
#     "${PROJECT_SOURCE_DIR}/core/AudioBufferPool.cpp"
#     "${PROJECT_SOURCE_DIR}/core/VoiceActivityDetector.cpp"
#     "${PROJECT_SOURCE_DIR}/core/MFCCProcessor.cpp"
#     "${PROJECT_SOURCE_DIR}/core/DTWComparator.cpp"
#     "${PROJECT_SOURCE_DIR}/core/RealtimeAudioProcessor.cpp"
#     # This file seems to handle third-party library inclusion details.
#     "${PROJECT_SOURCE_DIR}/core/ThirdPartyLibs.cpp"
#     # NOTE: The files AudioPlayer.cpp, AudioRecorder.cpp, and DTWProcessor.cpp
#     # are in the top-level src dir. Decide if they belong in the core library
#     # or are part of the native tools/platform implementation.
#     # For now, adding them to the core library.
#     "${PROJECT_SOURCE_DIR}/core/AudioPlayer.cpp"
#     "${PROJECT_SOURCE_DIR}/core/AudioRecorder.cpp"
#     "${PROJECT_SOURCE_DIR}/core/DTWProcessor.cpp"
# )

# This will find all .cpp files in the src/core directory and add them to the variable.
file(GLOB HUNTMASTER_CORE_SOURCES
    "${PROJECT_SOURCE_DIR}/core/*.cpp"
)


# Create the static library for the engine core.
add_library(HuntmasterEngine STATIC ${HUNTMASTER_CORE_SOURCES})

# Set public include directories. Any target linking to HuntmasterEngine will inherit these.
target_include_directories(HuntmasterEngine PUBLIC
    ${PROJECT_INCLUDE_DIR}
    # Add libs dir for header-only libraries like dr_wav.h and miniaudio.h
    ${PROJECT_LIBS_DIR}/kissfft
    ${PROJECT_LIBS_DIR}/dr_wav
    ${PROJECT_LIBS_DIR}/miniaudio
    ${PROJECT_TESTS_DIR}/lib/googletest
    ${PROJECT_LIBS_DIR}
)

# Link core dependencies.
target_link_libraries(HuntmasterEngine PUBLIC kissfft)
target_compile_definitions(HuntmasterEngine PUBLIC HAVE_KISSFFT)


# ==============================================================================
# Build Logic: Switch between Native and WebAssembly
# ==============================================================================

if(EMSCRIPTEN)
    # --------------------------------------------------------------------------
    # --- WEB ASSEMBLY (EMSCRIPTEN) BUILD ---
    # --------------------------------------------------------------------------
    message(STATUS "Configuring for WebAssembly (Emscripten) build.")

    # Define the sources needed specifically for the WASM module.
    # This primarily includes the Embind interface file.
    set(WASM_INTERFACE_SOURCES
        "${PROJECT_SOURCE_DIR}/platform/wasm/WASMInterface.cpp" # Assuming this is the correct path
    )

    # Create the final WASM executable (which produces .js and .wasm files)
    add_executable(huntmaster_wasm ${WASM_INTERFACE_SOURCES})

    # Link the WASM executable against the core engine library.
    # This pulls in all the core source files and their dependencies.
    target_link_libraries(huntmaster_wasm PRIVATE HuntmasterEngine)

    # Add Emscripten-specific compiler options
    target_compile_options(huntmaster_wasm PRIVATE
        -O3                 # Optimization level
        -fno-exceptions     # Disable exceptions for smaller size
        # -fno-rtti was correctly removed, as embind requires it.
        -Wall
        -Wextra
    )

    # Add Emscripten-specific linker options
    target_link_options(huntmaster_wasm PRIVATE
        -O3
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=16777216      # 16MB initial memory
        -sMAXIMUM_MEMORY=268435456     # 256MB max memory
        -sMODULARIZE=1
        -sEXPORT_NAME=HuntmasterEngine
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue']"
        -sENVIRONMENT=web
        -sSINGLE_FILE=0                # Separate .wasm file is good practice
        --bind                         # Enable embind
    )

    # Set the final output name and directory for the WASM files.
    set_target_properties(huntmaster_wasm PROPERTIES
        OUTPUT_NAME "huntmaster_engine"
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_WEB_DIR}/dist"
    )

else()
    # --------------------------------------------------------------------------
    # --- NATIVE BUILD (Windows, Linux, macOS) ---
    # --------------------------------------------------------------------------
    message(STATUS "Configuring for Native build.")

    # Link platform-specific libraries for native audio, threading, etc.
    if(WIN32)
        target_link_libraries(HuntmasterEngine PUBLIC winmm)
    elseif(APPLE)
        find_library(COREAUDIO_LIBRARY CoreAudio REQUIRED)
        find_library(AUDIOUNIT_LIBRARY AudioUnit REQUIRED)
        find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
        target_link_libraries(HuntmasterEngine PUBLIC
            ${COREAUDIO_LIBRARY}
            ${AUDIOUNIT_LIBRARY}
            ${COREFOUNDATION_LIBRARY}
        )
    elseif(UNIX AND NOT ANDROID)
        find_package(Threads REQUIRED)
        target_link_libraries(HuntmasterEngine PUBLIC Threads::Threads)
        # For audio, you might need to find and link ALSA or PulseAudio here.
        # find_package(ALSA)
        # if(ALSA_FOUND)
        #   target_link_libraries(HuntmasterEngine PUBLIC ${ALSA_LIBRARIES})
        # endif()
    endif()

    # --- Executable Targets (Tools) ---
    if(HUNTMASTER_BUILD_TOOLS)
        message(STATUS "Configuring native tools...")
        # Loop over a list of tool source files to create executables.
        file(GLOB TOOL_SOURCES "${PROJECT_TOOLS_DIR}/*.cpp")
        foreach(tool_source_path ${TOOL_SOURCES})
            get_filename_component(tool_name ${tool_source_path} NAME_WE)
            add_executable(${tool_name} ${tool_source_path})
            target_link_libraries(${tool_name} PRIVATE HuntmasterEngine)
            message(STATUS "  -- Added tool: ${tool_name}")
        endforeach()
    endif()

    # --- Unit Testing and Benchmarking Suite ---
    if(HUNTMASTER_BUILD_TESTS)
        message(STATUS "Configuring unit tests and benchmarks...")
        enable_testing()

        # Unit Tests (gtest)
        # Find all test files and create a single test runner executable.
        file(GLOB_RECURSE TEST_SOURCES
            "${PROJECT_TESTS_DIR}/unit/*.cpp"
            "${PROJECT_TESTS_DIR}/integration/*.cpp"
        )

        if(TEST_SOURCES)
            add_executable(RunEngineTests ${TEST_SOURCES})
            target_link_libraries(RunEngineTests PRIVATE HuntmasterEngine gtest gtest_main)
            include(GoogleTest)
            gtest_discover_tests(RunEngineTests)
            message(STATUS "  -- Unit test runner 'RunEngineTests' configured.")
        endif()

        # Benchmarks (gbenchmark)
        # The benchmarks have their own CMakeLists.txt, so we just add the directory.
        if(EXISTS "${PROJECT_TESTS_DIR}/benchmarks/CMakeLists.txt")
            add_subdirectory(${PROJECT_TESTS_DIR}/benchmarks)
            message(STATUS "  -- Benchmarks configured.")
        endif()
    endif()

    # --- Installation Rules ---
    # REPLACED: This whole section is now more robust.
    # The old install rules were okay, but using GNUInstallDirs is standard practice.
    include(GNUInstallDirs)
    install(TARGETS HuntmasterEngine
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(DIRECTORY ${PROJECT_INCLUDE_DIR}/huntmaster
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

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
