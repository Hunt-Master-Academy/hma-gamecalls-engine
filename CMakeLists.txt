# ==============================================================================
# CMakeLists.txt for Huntmaster Audio Engine (v4.0 - Modernized & Unified)
#
# This script handles both NATIVE builds (for testing tools) and
# WEB ASSEMBLY builds (for the web app) using modern CMake practices.
#
# To build for NATIVE:
#   cmake -B build
#   cmake --build build
#
# To build for WEB ASSEMBLY:
#   emcmake cmake -B build-wasm
#   cmake --build build-wasm
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

project(
    HuntmasterEngine
    VERSION 4.0
    LANGUAGES CXX C
)

# --- Project Structure Variables ---
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(PROJECT_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(PROJECT_TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# --- Third-Party Dependencies ---
# 1. KissFFT (Vendored)
add_subdirectory(${PROJECT_LIBS_DIR}/kissfft)
message(STATUS "KissFFT configured.")

# 2. Google Test (Fetched at configure time)
# Note: Google Test will only be built for native targets.
if(NOT EMSCRIPTEN) # Native build only
    # Using a Git submodule for Google Test is more robust than FetchContent,
    # especially in Docker environments, as it avoids filesystem permission issues.
    add_subdirectory(tests/lib/googletest)
    message(STATUS "Google Test configured.")
endif()


# ==============================================================================
# Build Logic: Switch between Native and WebAssembly
# ==============================================================================

if(EMSCRIPTEN)
    # --------------------------------------------------------------------------
    # --- WEB ASSEMBLY (EMSCRIPTEN) BUILD ---
    # --------------------------------------------------------------------------
    message(STATUS "Configuring for WebAssembly (Emscripten) build.")

    # Define the sources needed for the lightweight WASM module.
    # It does NOT include the full HuntmasterEngine.cpp or native audio components.
    set(WASM_SOURCES
        "${PROJECT_SOURCE_DIR}/platform/wasm/WASMInterface.cpp"
        "${PROJECT_SOURCE_DIR}/core/MFCCProcessor.cpp"
        "${PROJECT_SOURCE_DIR}/core/DTWComparator.cpp"
        "${PROJECT_SOURCE_DIR}/ThirdPartyLibs.cpp"
    )

    # Create the final WASM executable (which produces .js and .wasm files)
    add_executable(huntmaster_wasm ${WASM_SOURCES})

    # Set include directories for the WASM target
    target_include_directories(huntmaster_wasm PRIVATE
        ${PROJECT_INCLUDE_DIR}
        ${PROJECT_LIBS_DIR}
    )

    # Set C++ standard
    target_compile_features(huntmaster_wasm PRIVATE cxx_std_23)

    # Link against KissFFT
    target_link_libraries(huntmaster_wasm PRIVATE kissfft)
    target_compile_definitions(huntmaster_wasm PRIVATE HAVE_KISSFFT)

    # Add Emscripten-specific compiler and linker options
    target_compile_options(huntmaster_wasm PRIVATE
        -O3 # Optimization level
        -s ALLOW_MEMORY_GROWTH=1
        -s INITIAL_MEMORY=16MB
    )
    target_link_options(huntmaster_wasm PRIVATE
        -s MODULARIZE=1
        -s EXPORT_NAME='HuntmasterEngine'
        -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue']
        # EXPORTED_FUNCTIONS is not needed due to EMSCRIPTEN_KEEPALIVE
    )

    # Set the final output name and directory
    set_target_properties(huntmaster_wasm PROPERTIES
        OUTPUT_NAME "huntmaster_engine"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web/dist"
    )

else()
    # --------------------------------------------------------------------------
    # --- NATIVE BUILD (Windows, Linux, macOS) ---
    # --------------------------------------------------------------------------
    message(STATUS "Configuring for Native build.")

    # Define the core library with all platform-agnostic C++ code.
    add_library(
        HuntmasterEngine STATIC
        "${PROJECT_SOURCE_DIR}/core/HuntmasterEngine.cpp"
        "${PROJECT_SOURCE_DIR}/core/AudioBufferPool.cpp"
        "${PROJECT_SOURCE_DIR}/core/VoiceActivityDetector.cpp"
        "${PROJECT_SOURCE_DIR}/core/MFCCProcessor.cpp"
        "${PROJECT_SOURCE_DIR}/core/DTWComparator.cpp"
        "${PROJECT_SOURCE_DIR}/core/RealtimeAudioProcessor.cpp"
        "${PROJECT_SOURCE_DIR}/ThirdPartyLibs.cpp"
    )
    # Set public include directories for the library and any targets that link to it.
    target_include_directories(HuntmasterEngine PUBLIC
        ${PROJECT_INCLUDE_DIR}
        ${PROJECT_LIBS_DIR}
    )
    target_compile_features(HuntmasterEngine PUBLIC cxx_std_23)
    target_link_libraries(HuntmasterEngine PUBLIC kissfft)
    target_compile_definitions(HuntmasterEngine PUBLIC HAVE_KISSFFT)

    # Note: The following is for IDE organization and does not affect the build.
    source_group("Source Files/Core" FILES
        "${PROJECT_SOURCE_DIR}/core/HuntmasterEngine.cpp"
        "${PROJECT_SOURCE_DIR}/core/AudioBufferPool.cpp"
        "${PROJECT_SOURCE_DIR}/core/VoiceActivityDetector.cpp"
        "${PROJECT_SOURCE_DIR}/core/MFCCProcessor.cpp"
        "${PROJECT_SOURCE_DIR}/core/DTWComparator.cpp"
        "${PROJECT_SOURCE_DIR}/core/RealtimeAudioProcessor.cpp"
    )
    source_group("Source Files/Common" FILES "${PROJECT_SOURCE_DIR}/ThirdPartyLibs.cpp")

    # Helper function to create all our command-line test tools
    function(add_huntmaster_tool tool_name file_name)
        add_executable(${tool_name} "${PROJECT_TOOLS_DIR}/${file_name}")
        target_link_libraries(${tool_name} PRIVATE HuntmasterEngine)
    endfunction()
    
    # --- Executable Targets (Tools) ---
    message(STATUS "Configuring native tools and executables...")
    # add_huntmaster_tool(TestHarness test_harness.cpp) # Disabled: Source file not found
    add_huntmaster_tool(GenerateFeatures generate_features.cpp)
    add_huntmaster_tool(AnalyzeRecording analyze_recording.cpp)
    add_huntmaster_tool(AudioTrimmer audio_trimmer.cpp)
    add_huntmaster_tool(InteractiveRecorder interactive_recorder.cpp)
    add_huntmaster_tool(FindBestMatch find_best_match.cpp)
    add_huntmaster_tool(AudioVisualization audio_visualization.cpp)
    add_huntmaster_tool(RealTimeMonitor real_time_recording_monitor.cpp)
    # add_huntmaster_tool(TestPerformance test_performance.cpp) # Disabled: Source file not found
    # add_huntmaster_tool(TestRecorder test_recorder.cpp) # Disabled: Source file not found
    # add_huntmaster_tool(TestValidation test_validation.cpp) # Disabled: Source file not found
    # add_huntmaster_tool(TestMFCCConsistency test_mfcc_consistency.cpp) # Disabled: Source file not found
    # add_huntmaster_tool(TestCrossPlatform test_cross_platform.cpp) # Disabled: Source file not found
    add_huntmaster_tool(DetailedAnalysis detailed_analysis.cpp)


    # --- Unit Testing Suite ---
    message(STATUS "Configuring unit tests...")
    enable_testing()
    add_executable(
        RunEngineTests
        "${PROJECT_TESTS_DIR}/unit/AudioBufferPoolTest.cpp"
        "${PROJECT_TESTS_DIR}/unit/DTWComparatorTest.cpp"
        "${PROJECT_TESTS_DIR}/unit/RealTimeAudioProcessorTest.cpp"
        "${PROJECT_TESTS_DIR}/unit/VoiceActivityDetectorTest.cpp"
        "${PROJECT_TESTS_DIR}/integration/EndToEndTest.cpp"
    )
    # Note: The following is for IDE organization and does not affect the build.
    source_group("Test Files/Unit" FILES
        "${PROJECT_TESTS_DIR}/unit/AudioBufferPoolTest.cpp"
        "${PROJECT_TESTS_DIR}/unit/DTWComparatorTest.cpp"
        "${PROJECT_TESTS_DIR}/unit/RealTimeAudioProcessorTest.cpp"
        "${PROJECT_TESTS_DIR}/unit/VoiceActivityDetectorTest.cpp"
    )
    source_group("Test Files/Integration" FILES "${PROJECT_TESTS_DIR}/integration/EndToEndTest.cpp")
    target_link_libraries(RunEngineTests PRIVATE HuntmasterEngine GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(RunEngineTests)

    # --- Benchmarking Suite ---
    message(STATUS "Configuring benchmarks...")
    add_subdirectory(${PROJECT_TESTS_DIR}/benchmarks)

endif()

# --- Final Configuration Message ---
message(STATUS "=====================================")
message(STATUS "Huntmaster Engine Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
if(EMSCRIPTEN)
    message(STATUS "  Platform: WebAssembly (Emscripten)")
else()
    message(STATUS "  Platform: Native")
endif()
message(STATUS "=====================================")
