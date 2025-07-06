# File: tests/benchmarks/CMakeLists.txt

# Add the Google Benchmark library from the Git submodule.
# This is more robust than FetchContent in containerized environments.
set(BENCHMARK_ENABLE_TESTING OFF)
set(BENCHMARK_ENABLE_INSTALL OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/benchmark)

# Define the benchmark executable
add_executable(
    RunBenchmarks
    RealtimeAudioProcessorBenchmark.cpp
)

# Link the benchmark executable against our engine and the benchmark library.
target_link_libraries(
    RunBenchmarks
    PRIVATE
    HuntmasterEngine
    benchmark::benchmark
)