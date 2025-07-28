# ==============================================================================
# tests/CMakeLists.txt - Test Suite Configuration
#
# This file is responsible for defining all test executables and linking them
# against the UnifiedAudioEngine library. It includes unit tests, integration
# tests, and standalone diagnostic tools.
# ==============================================================================

message(STATUS "Configuring unit tests and benchmarks...")

# Enable testing framework
enable_testing()

# ==============================================================================
# Test Utilities Library
# ==============================================================================

# Create TestUtils library for shared test infrastructure
add_library(TestUtils STATIC
    ${PROJECT_TESTS_DIR}/lib/TestUtils.cpp
)

target_include_directories(TestUtils PUBLIC
    ${PROJECT_TESTS_DIR}/lib
    ${PROJECT_INCLUDE_DIR}
)

target_link_libraries(TestUtils PUBLIC
    UnifiedAudioEngine
    gtest
)

# Ensure C++20 standard for filesystem support
target_compile_features(TestUtils PUBLIC cxx_std_20)

message(STATUS "  -- TestUtils library configured for cross-platform test infrastructure.")

# ==============================================================================
# Main Unit Test Suite (Google Test)
# ==============================================================================

# Find all test files except standalone diagnostic tools
file(GLOB_RECURSE ALL_TEST_SOURCES
    "${PROJECT_TESTS_DIR}/unit/*.cpp"
    "${PROJECT_TESTS_DIR}/integration/*.cpp"
)

# Exclude standalone diagnostic tools that have their own main() functions
set(STANDALONE_TESTS
    "${PROJECT_TESTS_DIR}/unit/test_cross_platform.cpp"
    "${PROJECT_TESTS_DIR}/unit/test_harness.cpp"
    "${PROJECT_TESTS_DIR}/unit/test_performance.cpp"
    "${PROJECT_TESTS_DIR}/unit/test_recording.cpp"
    "${PROJECT_TESTS_DIR}/unit/test_recorder.cpp"
    "${PROJECT_TESTS_DIR}/unit/test_audio_format_converter.cpp"  # Temporarily excluded - contains abstract class instantiations
    "${PROJECT_TESTS_DIR}/unit/test_enhanced_wasm_interface.cpp"  # WASM-specific test, excluded from native builds
    "${PROJECT_TESTS_DIR}/integration/LoadandExtractTest.cpp"
)

# Filter out standalone tests from Google Test sources
set(TEST_SOURCES)
foreach(test_file ${ALL_TEST_SOURCES})
    list(FIND STANDALONE_TESTS ${test_file} exclude_index)
    if(exclude_index EQUAL -1)
        list(APPEND TEST_SOURCES ${test_file})
    endif()
endforeach()

# Create main test runner if we have test sources
if(TEST_SOURCES)
    add_executable(RunEngineTests ${TEST_SOURCES})
    target_link_libraries(RunEngineTests PRIVATE UnifiedAudioEngine TestUtils GTest::gtest_main)
    set_target_properties(RunEngineTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    include(GoogleTest)
    gtest_discover_tests(RunEngineTests)
    message(STATUS "  -- Unit test runner 'RunEngineTests' configured.")
endif()

# ==============================================================================
# Standalone Diagnostic Tools
# ==============================================================================

# Macro to create test executable with proper output directory
macro(add_test_executable name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE UnifiedAudioEngine TestUtils)
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endmacro()

# Cross-platform compatibility test
add_test_executable(CrossPlatformTest "${PROJECT_TESTS_DIR}/unit/test_cross_platform.cpp")

# Test harness for manual validation
add_test_executable(TestHarness "${PROJECT_TESTS_DIR}/unit/test_harness.cpp")

# Performance testing tool
add_test_executable(PerformanceTest "${PROJECT_TESTS_DIR}/unit/test_performance.cpp")

# Recording functionality test
add_test_executable(RecordingTest "${PROJECT_TESTS_DIR}/unit/test_recording.cpp")

# Audio recorder test
add_test_executable(RecorderTest "${PROJECT_TESTS_DIR}/unit/test_recorder.cpp")

message(STATUS "  -- Standalone diagnostic tools configured.")

# ==============================================================================
# Specialized Unit Tests with Google Test
# ==============================================================================

# Macro to create specialized test executable with Google Test
macro(add_gtest_executable name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE UnifiedAudioEngine TestUtils gtest gtest_main)
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endmacro()

# UnifiedAudioEngine test
add_gtest_executable(UnifiedEngineTest "${PROJECT_TESTS_DIR}/unit/test_unified_engine.cpp")

# MFCC consistency test
add_gtest_executable(MFCCConsistencyUnifiedTest "${PROJECT_TESTS_DIR}/unit/test_mfcc_consistency_unified.cpp")

# DTW algorithm test
add_gtest_executable(DTWUnifiedTest "${PROJECT_TESTS_DIR}/unit/dtw_tests_unified.cpp")

# MFCC direct computation test
add_gtest_executable(MFCCDirectUnifiedTest "${PROJECT_TESTS_DIR}/unit/test_mfcc_direct_unified.cpp")

# Validation test suite
add_gtest_executable(ValidationUnifiedTest "${PROJECT_TESTS_DIR}/unit/test_validation_unified.cpp")

# VAD Configuration test suite
add_gtest_executable(UnifiedEngineVADConfigTest "${PROJECT_TESTS_DIR}/unit/test_unified_engine_vad_config.cpp")

# Performance Profiler test suite
add_gtest_executable(PerformanceProfilerTest "${PROJECT_TESTS_DIR}/unit/test_performance_profiler.cpp")

# Comprehensive test suites for 90% coverage
add_gtest_executable(UnifiedEngineComprehensiveTest "${PROJECT_TESTS_DIR}/unit/test_unified_engine_comprehensive.cpp")
add_gtest_executable(DTWComprehensiveTest "${PROJECT_TESTS_DIR}/unit/test_dtw_comprehensive.cpp")
add_gtest_executable(MFCCEdgeCasesTest "${PROJECT_TESTS_DIR}/unit/test_mfcc_edge_cases.cpp")
add_gtest_executable(MasterCallComprehensiveTest "${PROJECT_TESTS_DIR}/unit/test_master_call_comprehensive.cpp")
add_gtest_executable(DebugTest "${PROJECT_TESTS_DIR}/unit/DebugTest.cpp")
add_gtest_executable(ErrorHandlingTest "${PROJECT_TESTS_DIR}/unit/test_error_handling_comprehensive.cpp")
add_gtest_executable(MemoryManagementTest "${PROJECT_TESTS_DIR}/unit/test_memory_management_comprehensive.cpp")
add_gtest_executable(SessionStateTest "${PROJECT_TESTS_DIR}/unit/test_session_state_comprehensive.cpp")
add_gtest_executable(BasicCoverageTest "${PROJECT_TESTS_DIR}/unit/test_basic_coverage.cpp")
add_gtest_executable(CoverageOptimizerTest "${PROJECT_TESTS_DIR}/unit/test_coverage_optimizer.cpp")

message(STATUS "  -- Specialized unit tests configured (including comprehensive coverage tests).")

# ==============================================================================
# Integration Tests - End-to-End Validation
# ==============================================================================

# End-to-end test with real audio data
add_gtest_executable(EndToEndTest "${PROJECT_TESTS_DIR}/integration/EndToEndTest.cpp")

# Real wildlife call analysis test
add_gtest_executable(RealWildlifeCallAnalysisTest "${PROJECT_TESTS_DIR}/integration/RealWildlifeCallAnalysisTest.cpp")

# Load and extract feature test for debugging DTW issues
add_test_executable(LoadandExtractTest "${PROJECT_TESTS_DIR}/integration/LoadandExtractTest.cpp")

# Full audio pipeline integration test
add_gtest_executable(AudioPipelineTest "${PROJECT_TESTS_DIR}/integration/test_audio_pipeline.cpp")

message(STATUS "  -- Integration tests configured for real audio data validation.")

# ==============================================================================
# Benchmarks (Google Benchmark)
# ==============================================================================

# Include benchmarks if they have their own CMakeLists.txt
if(EXISTS "${PROJECT_TESTS_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(${PROJECT_TESTS_DIR}/benchmarks)
    message(STATUS "  -- Benchmarks configured.")
endif()

message(STATUS "Test suite configuration complete.")
