# ==============================================================================
# CMakeLists.txt for Huntmaster Audio Engine (v3.2 - Restructured)
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

project(
    HuntmasterEngine
    VERSION 3.2
    LANGUAGES CXX C
)

# --- Project Structure Variables ---
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(PROJECT_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(PROJECT_TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools)
set(PROJECT_BINDINGS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bindings)

# --- Third-Party Dependencies ---
# 1. KissFFT (Vendored)
add_subdirectory(${PROJECT_LIBS_DIR}/kissfft)
message(STATUS "KissFFT configured.")

# 2. Google Test (Fetched at configure time)
include(FetchContent)
cmake_policy(SET CMP0135 NEW)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "Use shared CRT for GoogleTest (required for MSVC projects)" FORCE)
FetchContent_MakeAvailable(googletest)
message(STATUS "Google Test configured.")

# --- Core Engine Library ---
set(CORE_SOURCES
    "${PROJECT_SOURCE_DIR}/core/HuntmasterEngine.cpp"
    "${PROJECT_SOURCE_DIR}/core/AudioBufferPool.cpp"
    "${PROJECT_SOURCE_DIR}/core/VoiceActivityDetector.cpp"
    "${PROJECT_SOURCE_DIR}/core/MFCCProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/core/DTWComparator.cpp"
    "${PROJECT_SOURCE_DIR}/core/RealtimeAudioProcessor.cpp"
    # Legacy files (if they still exist and are needed)
    "${PROJECT_SOURCE_DIR}/HuntmasterAudioEngine.cpp"
    "${PROJECT_SOURCE_DIR}/DTWProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/AudioRecorder.cpp"
    "${PROJECT_SOURCE_DIR}/AudioPlayer.cpp"
    "${PROJECT_SOURCE_DIR}/ThirdPartyLibs.cpp"
)

# Platform-specific sources
set(PLATFORM_SOURCES "")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    # Only append WASMInterface.cpp for Emscripten builds
    list(APPEND PLATFORM_SOURCES "${PROJECT_SOURCE_DIR}/platform/wasm/WASMInterface.cpp")

    # Create WASM-specific executable
    add_executable(huntmaster_wasm
        ${PROJECT_SOURCE_DIR}/platform/wasm/WASMInterface.cpp
        ${CORE_SOURCES}
    )

    target_link_libraries(huntmaster_wasm PRIVATE HuntmasterEngine)

    # Set output directory
    set_target_properties(huntmaster_wasm PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web/dist"
        OUTPUT_NAME "huntmaster_engine"
    )
else()
    # Add native platform sources here when implemented
    # list(APPEND PLATFORM_SOURCES "${PROJECT_SOURCE_DIR}/platform/native/NativeInterface.cpp")
endif()
# Create the main library
# Create the main library
add_library(HuntmasterEngine STATIC ${CORE_SOURCES} ${PLATFORM_SOURCES})

# Set include directories
target_include_directories(HuntmasterEngine PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>"
    # Add the library directory to the include path so that third-party headers (e.g., "kissfft/kiss_fftr.h") are found.
    # This also allows for easy inclusion of additional third-party libraries in the future.
    "$<BUILD_INTERFACE:${PROJECT_LIBS_DIR}>"
    "$<INSTALL_INTERFACE:include>"
)

# Use C++20 for modern features
target_compile_features(HuntmasterEngine PUBLIC cxx_std_20)

# Link dependencies
target_link_libraries(HuntmasterEngine PUBLIC kissfft)

# Compile definitions
target_compile_definitions(HuntmasterEngine PUBLIC HAVE_KISSFFT)

# Platform-specific configurations (consolidated)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    target_compile_definitions(HuntmasterEngine PUBLIC __EMSCRIPTEN__)
    set_target_properties(HuntmasterEngine PROPERTIES
        COMPILE_FLAGS "-s USE_PTHREADS=0 -s ALLOW_MEMORY_GROWTH=1"
        LINK_FLAGS "-s WASM=1 -s MODULARIZE=1 -s EXPORT_NAME='HuntmasterModule' -lembind"
    )
    target_compile_options(HuntmasterEngine PRIVATE
        -O3
        -s ALLOW_MEMORY_GROWTH=1
        -s INITIAL_MEMORY=16MB
        -s MAXIMUM_MEMORY=128MB
    )
    target_link_options(HuntmasterEngine PUBLIC
        -s MODULARIZE=1
        -s EXPORT_NAME='HuntmasterEngine'
        -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue','HEAP8','HEAP16','HEAP32','HEAPU8','HEAPU16','HEAPU32','HEAPF32','HEAPF64']
        -s ENVIRONMENT='web,worker'
        -s SINGLE_FILE=0  # Keep WASM file separate for better caching
    )
endif()
# --- Helper Function for Creating Test Tools ---
function(add_huntmaster_tool tool_name file_name)
if(EMSCRIPTEN)
    target_compile_definitions(HuntmasterEngine PUBLIC __EMSCRIPTEN__)
endif()
message(STATUS "Configuring tools and executables...")
add_huntmaster_tool(TestHarness test_harness.cpp)
add_huntmaster_tool(TestRecording test_recording.cpp)
add_huntmaster_tool(GenerateFeatures generate_features.cpp)
add_huntmaster_tool(InteractiveRecorder interactive_recorder.cpp)
add_huntmaster_tool(AnalyzeRecording analyze_recording.cpp)
add_huntmaster_tool(FindBestMatch find_best_match.cpp)
add_huntmaster_tool(AudioVisualization audio_visualization.cpp)
add_huntmaster_tool(AudioTrimmer audio_trimmer.cpp)
add_huntmaster_tool(RealTimeMonitor real_time_recording_monitor.cpp)
add_huntmaster_tool(TestPerformance test_performance.cpp)
add_huntmaster_tool(TestMFCCConsistency test_mfcc_consistency.cpp)
add_huntmaster_tool(TestCrossPlatform test_cross_platform.cpp)
add_huntmaster_tool(DetailedAnalysis detailed_analysis.cpp)
add_huntmaster_tool(TestRecorder test_recorder.cpp)
add_huntmaster_tool(TestValidation test_validation.cpp)

# Link Windows-specific libraries only for the relevant target
if(WIN32)
    target_link_libraries(TestPerformance PRIVATE psapi)
endif()

# --- Unit Testing Suite ---
message(STATUS "Configuring unit tests...")
enable_testing()

# Core unit tests
set(TEST_SOURCES
    "${PROJECT_TESTS_DIR}/engine_tests.cpp"
    "${PROJECT_TESTS_DIR}/mfcc_tests.cpp"
    "${PROJECT_TESTS_DIR}/dtw_tests.cpp"
    "${PROJECT_TESTS_DIR}/binary_compatibility_tests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_kissfft.cpp"
)

# Add new unit tests if they exist
if(EXISTS "${PROJECT_TESTS_DIR}/unit/AudioBufferPoolTest.cpp")
    list(APPEND TEST_SOURCES "${PROJECT_TESTS_DIR}/unit/AudioBufferPoolTest.cpp")
endif()
if(EXISTS "${PROJECT_TESTS_DIR}/unit/VoiceActivityDetectorTest.cpp")
    list(APPEND TEST_SOURCES "${PROJECT_TESTS_DIR}/unit/VoiceActivityDetectorTest.cpp")
endif()
if(EXISTS "${PROJECT_TESTS_DIR}/unit/MFCCProcessorTest.cpp")
    list(APPEND TEST_SOURCES "${PROJECT_TESTS_DIR}/unit/MFCCProcessorTest.cpp")
endif()
if(EXISTS "${PROJECT_TESTS_DIR}/unit/DTWComparatorTest.cpp")
    list(APPEND TEST_SOURCES "${PROJECT_TESTS_DIR}/unit/DTWComparatorTest.cpp")
endif()

# Add WASM tests only if building for WASM
# Add WASM tests only if building for WASM
if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_wasm_minimal.cpp")
    list(APPEND TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_wasm_minimal.cpp")
endif()
add_executable(RunEngineTests ${TEST_SOURCES})
target_link_libraries(RunEngineTests PRIVATE HuntmasterEngine GTest::gtest GTest::gtest_main)
target_include_directories(RunEngineTests PRIVATE ${PROJECT_LIBS_DIR})

include(GoogleTest)
gtest_discover_tests(RunEngineTests)

# --- Installation Rules (Optional) ---
install(TARGETS HuntmasterEngine
    EXPORT HuntmasterEngineTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY ${PROJECT_INCLUDE_DIR}/huntmaster
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Install TypeScript definitions for WASM
# Install TypeScript definitions for WASM
if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    install(FILES ${PROJECT_BINDINGS_DIR}/wasm/huntmaster-engine.d.ts
        DESTINATION share/huntmaster/bindings
    )
endif()
# --- Export Configuration ---
install(EXPORT HuntmasterEngineTargets
    FILE HuntmasterEngineTargets.cmake
    NAMESPACE Huntmaster::
    DESTINATION lib/cmake/HuntmasterEngine
)

# --- Final Configuration Message ---
message(STATUS "=====================================")
message(STATUS "Huntmaster Engine Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: C++20")
if(EMSCRIPTEN)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    message(STATUS "  Platform: WebAssembly (Emscripten)")
else()
    message(STATUS "  Platform: Native")
endif()
message(STATUS "Huntmaster Engine build configured successfully.")