# ============================================================================
# tests/CMakeLists.txt - Test Suite Configuration (modernized)
# ============================================================================

message(STATUS "Configuring tests...")

enable_testing()

# Only wire Enhanced Analyzer integration/performance tests into the main runner
set(ENHANCED_ANALYZER_TESTS
	${CMAKE_CURRENT_SOURCE_DIR}/../test_data/integration/enhanced_analyzers/EnhancedAnalyzersIntegrationTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../test_data/integration/enhanced_analyzers/EnhancedAnalyzersPerformanceTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../test_data/integration/enhanced_analyzers/CadenceAnalyzerRealAudioTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../test_data/integration/enhanced_analyzers/EnhancedAnalyzersEnableResetTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../test_data/integration/enhanced_analyzers/MasterCallsComparisonTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/unit/test_finalize_session.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/unit/test_finalize_improvement.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/unit/test_similarity_separation.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/unit/test_finalize_fallback_flag.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/unit/test_finalize_fallback_positive.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/unit/test_finalize_fallback_negative.cpp
)

add_executable(RunEngineTests ${ENHANCED_ANALYZER_TESTS})
target_link_libraries(RunEngineTests PRIVATE UnifiedAudioEngine GTest::gtest_main GTest::gmock)
target_compile_definitions(RunEngineTests PRIVATE HUNTMASTER_TEST_HOOKS=1)
# Ensure the engine itself is compiled with test hook symbols so wrapper methods link
if (TARGET UnifiedAudioEngine)
	target_compile_definitions(UnifiedAudioEngine PUBLIC HUNTMASTER_TEST_HOOKS=1)
endif()
target_include_directories(RunEngineTests PRIVATE ${PROJECT_INCLUDE_DIR} ${PROJECT_SOURCE_DIR})
set_target_properties(RunEngineTests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
include(GoogleTest)
gtest_discover_tests(RunEngineTests)
message(STATUS "  -- Enhanced Analyzer test runner 'RunEngineTests' configured")

# Compile-only check with diagnostics disabled to validate public headers & snapshot layout
add_executable(RunEngineNoDiag tests_no_diag_compile_only.cpp)
target_link_libraries(RunEngineNoDiag PRIVATE UnifiedAudioEngine)
target_compile_definitions(RunEngineNoDiag PRIVATE HUNTMASTER_DISABLE_DIAGNOSTIC_COMPONENTS=1)
set_target_properties(RunEngineNoDiag PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Runtime assertions with diagnostics disabled
add_executable(RunEngineNoDiagTests unit/test_no_diagnostics_build.cpp)
target_link_libraries(RunEngineNoDiagTests PRIVATE UnifiedAudioEngine GTest::gtest_main)
target_compile_definitions(RunEngineNoDiagTests PRIVATE HUNTMASTER_DISABLE_DIAGNOSTIC_COMPONENTS=1)
set_target_properties(RunEngineNoDiagTests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
gtest_discover_tests(RunEngineNoDiagTests)

message(STATUS "Tests configuration complete.")
