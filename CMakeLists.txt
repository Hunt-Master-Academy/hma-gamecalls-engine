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
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
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
if(EMSCRIPTEN)
    list(APPEND PLATFORM_SOURCES "${PROJECT_SOURCE_DIR}/platform/wasm/WASMInterface.cpp")
else()
    # Add native platform sources here when implemented
    # list(APPEND PLATFORM_SOURCES "${PROJECT_SOURCE_DIR}/platform/native/NativeInterface.cpp")
endif()

# Create the main library
add_library(HuntmasterEngine STATIC ${CORE_SOURCES} ${PLATFORM_SOURCES})

# Set include directories
target_include_directories(HuntmasterEngine PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>"
    # This ensures that files like "kissfft/kiss_fftr.h" can be found
    "$<BUILD_INTERFACE:${PROJECT_LIBS_DIR}>"
    "$<INSTALL_INTERFACE:include>"
)

# Use C++20 for modern features
target_compile_features(HuntmasterEngine PUBLIC cxx_std_20)

# Link dependencies
target_link_libraries(HuntmasterEngine PUBLIC kissfft)

# Compile definitions
target_compile_definitions(HuntmasterEngine PUBLIC HAVE_KISSFFT)

# Platform-specific configurations
if(EMSCRIPTEN)
    target_compile_definitions(HuntmasterEngine PUBLIC __EMSCRIPTEN__)
    set_target_properties(HuntmasterEngine PROPERTIES
        COMPILE_FLAGS "-s USE_PTHREADS=0 -s ALLOW_MEMORY_GROWTH=1"
        LINK_FLAGS "-s WASM=1 -s MODULARIZE=1 -s EXPORT_NAME='HuntmasterModule' -lembind"
    )
endif()

# --- Helper Function for Creating Test Tools ---
function(add_huntmaster_tool tool_name file_name)
    add_executable(${tool_name} "${PROJECT_TOOLS_DIR}/${file_name}")
    target_link_libraries(${tool_name} PRIVATE HuntmasterEngine)
    # The tools might need to include third-party headers directly (e.g., miniaudio.h)
    target_include_directories(${tool_name} PRIVATE ${PROJECT_LIBS_DIR})
endfunction()

# --- Executable Targets (Tests and Tools) ---
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
if(EMSCRIPTEN AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_wasm_minimal.cpp")
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
if(EMSCRIPTEN)
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
    message(STATUS "  Platform: WebAssembly (Emscripten)")
else()
    message(STATUS "  Platform: Native")
endif()
message(STATUS "=====================================")
message(STATUS "Huntmaster Engine build configured successfully.")