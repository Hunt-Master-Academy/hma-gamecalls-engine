# Advanced I/O Optimization Components for Huntmaster Audio Engine

# Check for optional dependencies
find_package(PkgConfig QUIET)

# Check for NUMA support on Linux
set(HAVE_NUMA FALSE)
if(UNIX AND NOT APPLE)
    find_library(NUMA_LIBRARY numa)
    find_path(NUMA_INCLUDE_DIR numa.h)
    if(NUMA_LIBRARY AND NUMA_INCLUDE_DIR)
        set(HAVE_NUMA TRUE)
        message(STATUS "NUMA support found")
    else()
        message(STATUS "NUMA support not found - will use fallback")
    endif()
endif()

# Check for io_uring support on Linux
set(HAVE_IO_URING FALSE)
if(UNIX AND NOT APPLE)
    find_library(URING_LIBRARY uring)
    find_path(URING_INCLUDE_DIR liburing.h)
    if(URING_LIBRARY AND URING_INCLUDE_DIR)
        set(HAVE_IO_URING TRUE)
        message(STATUS "io_uring support found")
    else()
        message(STATUS "io_uring support not found - will use thread pool fallback")
    endif()
endif()

# Check for compression libraries
find_library(LZ4_LIBRARY lz4)
find_path(LZ4_INCLUDE_DIR lz4.h)
set(HAVE_LZ4 FALSE)
if(LZ4_LIBRARY AND LZ4_INCLUDE_DIR)
    set(HAVE_LZ4 TRUE)
    message(STATUS "LZ4 compression support found")
endif()

find_library(ZSTD_LIBRARY zstd)
find_path(ZSTD_INCLUDE_DIR zstd.h)
set(HAVE_ZSTD FALSE)
if(ZSTD_LIBRARY AND ZSTD_INCLUDE_DIR)
    set(HAVE_ZSTD TRUE)
    message(STATUS "Zstandard compression support found")
endif()

# Create the Advanced I/O Optimization library
add_library(AdvancedIOOptimizer STATIC
    src/core/AdvancedIOOptimizer.cpp
)

target_include_directories(AdvancedIOOptimizer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(AdvancedIOOptimizer PUBLIC
    OptimizedAudioIO
)

# Add conditional compilation flags
target_compile_definitions(AdvancedIOOptimizer PRIVATE
    $<$<BOOL:${HAVE_NUMA}>:HAVE_NUMA>
    $<$<BOOL:${HAVE_IO_URING}>:HAVE_IO_URING>
    $<$<BOOL:${HAVE_LZ4}>:HAVE_LZ4>
    $<$<BOOL:${HAVE_ZSTD}>:HAVE_ZSTD>
)

# Link optional libraries
if(HAVE_NUMA)
    target_link_libraries(AdvancedIOOptimizer PRIVATE ${NUMA_LIBRARY})
    target_include_directories(AdvancedIOOptimizer PRIVATE ${NUMA_INCLUDE_DIR})
endif()

if(HAVE_IO_URING)
    target_link_libraries(AdvancedIOOptimizer PRIVATE ${URING_LIBRARY})
    target_include_directories(AdvancedIOOptimizer PRIVATE ${URING_INCLUDE_DIR})
endif()

if(HAVE_LZ4)
    target_link_libraries(AdvancedIOOptimizer PRIVATE ${LZ4_LIBRARY})
    target_include_directories(AdvancedIOOptimizer PRIVATE ${LZ4_INCLUDE_DIR})
endif()

if(HAVE_ZSTD)
    target_link_libraries(AdvancedIOOptimizer PRIVATE ${ZSTD_LIBRARY})
    target_include_directories(AdvancedIOOptimizer PRIVATE ${ZSTD_INCLUDE_DIR})
endif()

# Set C++17 requirement for modern features
target_compile_features(AdvancedIOOptimizer PUBLIC cxx_std_17)

# Add compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(AdvancedIOOptimizer PRIVATE
        -O3                # Maximum optimization
        -march=native      # Use native CPU instructions
        -mtune=native      # Tune for native CPU
        -ffast-math        # Aggressive floating-point optimizations
        -funroll-loops     # Unroll loops for better performance
        -finline-functions # Inline functions aggressively
        -fomit-frame-pointer # Omit frame pointer for better register usage
    )

    # Add SIMD support detection
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-msse4.2" HAVE_SSE42)
    check_cxx_compiler_flag("-mavx2" HAVE_AVX2)
    check_cxx_compiler_flag("-mavx512f" HAVE_AVX512)

    if(HAVE_AVX512)
        target_compile_options(AdvancedIOOptimizer PRIVATE -mavx512f)
        target_compile_definitions(AdvancedIOOptimizer PRIVATE HAVE_AVX512)
        message(STATUS "AVX-512 support enabled")
    elseif(HAVE_AVX2)
        target_compile_options(AdvancedIOOptimizer PRIVATE -mavx2)
        target_compile_definitions(AdvancedIOOptimizer PRIVATE HAVE_AVX2)
        message(STATUS "AVX2 support enabled")
    elseif(HAVE_SSE42)
        target_compile_options(AdvancedIOOptimizer PRIVATE -msse4.2)
        target_compile_definitions(AdvancedIOOptimizer PRIVATE HAVE_SSE42)
        message(STATUS "SSE 4.2 support enabled")
    endif()
endif()

# Create the I/O Optimization Demo tool
add_executable(IOOptimizationDemo
    tools/IOOptimizationDemo.cpp
)

target_link_libraries(IOOptimizationDemo PRIVATE
    AdvancedIOOptimizer
    OptimizedAudioIO
    UnifiedAudioEngine
)

# Add platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(IOOptimizationDemo PRIVATE pthread rt)
endif()

if(WIN32)
    target_link_libraries(IOOptimizationDemo PRIVATE ws2_32 winmm)
endif()

# Install targets
install(TARGETS AdvancedIOOptimizer
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(TARGETS IOOptimizationDemo
    RUNTIME DESTINATION bin
)

install(FILES
    include/huntmaster/core/AdvancedIOOptimizer.h
    DESTINATION include/huntmaster/core
)

# Create a configuration summary
message(STATUS "Advanced I/O Optimization Configuration:")
message(STATUS "  NUMA support: ${HAVE_NUMA}")
message(STATUS "  io_uring support: ${HAVE_IO_URING}")
message(STATUS "  LZ4 compression: ${HAVE_LZ4}")
message(STATUS "  Zstandard compression: ${HAVE_ZSTD}")
message(STATUS "  SIMD optimizations: ${HAVE_AVX512}${HAVE_AVX2}${HAVE_SSE42}")

# Add test target if we're building tests
if(BUILD_TESTING)
    add_executable(AdvancedIOOptimizerTests
        tests/unit/test_advanced_io_optimizer.cpp
    )

    target_link_libraries(AdvancedIOOptimizerTests PRIVATE
        AdvancedIOOptimizer
        gtest_main
    )

    add_test(NAME AdvancedIOOptimizerTests COMMAND AdvancedIOOptimizerTests)
endif()

# Add performance benchmark target if the source file exists
if(EXISTS "${CMAKE_SOURCE_DIR}/tools/IOPerformanceBenchmark.cpp")
    add_executable(IOPerformanceBenchmark
        tools/IOPerformanceBenchmark.cpp
    )

    target_link_libraries(IOPerformanceBenchmark PRIVATE
        AdvancedIOOptimizer
        OptimizedAudioIO
    )
endif()

# Documentation generation
if(BUILD_DOCUMENTATION)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_XML YES)
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")

    doxygen_add_docs(
        AdvancedIOOptimizerDocs
        ${CMAKE_CURRENT_SOURCE_DIR}/include/huntmaster/core/AdvancedIOOptimizer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/AdvancedIOOptimizer.cpp
        COMMENT "Generating Advanced I/O Optimizer documentation"
    )
endif()
