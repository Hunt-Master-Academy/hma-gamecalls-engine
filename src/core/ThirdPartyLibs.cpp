// ThirdPartyLibs.cpp - Implementation of third-party libraries

#include <cstdlib>  // for malloc/free

#ifdef __EMSCRIPTEN__
// For WASM builds, include KissFFT implementation directly
#define KISS_FFT_MALLOC(size) malloc(size)
#define KISS_FFT_FREE(ptr) free(ptr)
#define kiss_fft_scalar float

// Include the C files directly for WASM
extern "C" {
#include "kiss_fft.c"
#include "kiss_fftr.c"
}
#endif

// Define these before including to ensure we get the implementations
#define DR_WAV_IMPLEMENTATION
#define MINIAUDIO_IMPLEMENTATION

// Platform-specific defines for miniaudio
#ifdef __EMSCRIPTEN__
#define MA_NO_PTHREAD_IN_HEADER
#define MA_NO_GENERATION
#define MA_NO_DEVICE_IO  // WASM doesn't have direct device access
#endif

// Include the single-header libraries
#include "dr_wav.h"
#include "miniaudio.h"

// Optional: Include any other single-header libraries you're using
// #define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"

// Note: This file intentionally has no other content.
// Its purpose is solely to provide the implementations of the
// single-header libraries to avoid multiple definition errors.