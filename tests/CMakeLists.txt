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
    target_link_libraries(RunEngineTests PRIVATE UnifiedAudioEngine GTest::gtest_main)
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
    target_link_libraries(${name} PRIVATE UnifiedAudioEngine)
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
    target_link_libraries(${name} PRIVATE UnifiedAudioEngine gtest gtest_main)
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

message(STATUS "  -- Specialized unit tests configured.")

# ==============================================================================
# Integration Tests - End-to-End Validation
# ==============================================================================

# End-to-end test with real audio data
add_gtest_executable(EndToEndTest "${PROJECT_TESTS_DIR}/integration/EndToEndTest.cpp")

# Real wildlife call analysis test
add_gtest_executable(RealWildlifeCallAnalysisTest "${PROJECT_TESTS_DIR}/integration/RealWildlifeCallAnalysisTest.cpp")

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
