# File: tests/benchmarks/CMakeLists.txt

# This file defines the executables for your benchmarks.
# It assumes the Google Benchmark library (target: benchmark::benchmark) has
# already been made available by FetchContent in the root CMakeLists.txt.

add_executable(
    RunBenchmarks
    RealtimeAudioProcessorBenchmark.cpp
)

# Link the benchmark executable against our engine and the Google Benchmark library.
target_link_libraries(
    RunBenchmarks
    PRIVATE
    HuntmasterEngine
    benchmark::benchmark
)