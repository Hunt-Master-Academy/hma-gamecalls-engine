cmake_minimum_required(VERSION 3.15)
project(kissfft_minimal C)

# Just build the core KissFFT library
set(KISSFFT_SOURCES
 ../kissfft/kiss_fft.c
 ../kissfft/tools/kiss_fftr.c
)

add_library(kissfft STATIC ${KISSFFT_SOURCES})
target_include_directories(kissfft PUBLIC 
 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../kissfft>
 $<INSTALL_INTERFACE:include>
)
target_compile_definitions(kissfft PUBLIC kiss_fft_scalar=float)