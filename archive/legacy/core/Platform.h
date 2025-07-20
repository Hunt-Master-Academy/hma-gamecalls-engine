// File: Platform.h
#pragma once

#include "Expected.h"

// ============================================================================
// Huntmaster Engine Platform Detection
//
// This header defines a set of preprocessor macros to identify the target
// platform (OS and architecture) at compile time. This allows for
// platform-specific code and optimizations.
//
// It should be included by a central project header (e.g., IHuntmasterEngine.h).
// ============================================================================


// --- WebAssembly ---
#if defined(__EMSCRIPTEN__)
    #define HUNTMASTER_PLATFORM_WASM 1
    // For WASM, we assume a single-threaded environment unless pthreads are enabled.
    #if !defined(__EMSCRIPTEN_PTHREADS__)
        #define HUNTMASTER_SINGLE_THREADED 1
    #endif
    // WASM has no direct access to native audio devices.
    #define HUNTMASTER_NO_DIRECT_AUDIO 1

// --- Android ---
#elif defined(__ANDROID__)
    #define HUNTMASTER_PLATFORM_ANDROID 1
    #define HUNTMASTER_PLATFORM_NATIVE 1
    // Define a helper for JNI (Java Native Interface) code
    #define HUNTMASTER_JNI_INTERFACE 1

// --- Apple Platforms (iOS, macOS) ---
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #define HUNTMASTER_PLATFORM_NATIVE 1
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #define HUNTMASTER_PLATFORM_IOS 1
    #elif TARGET_OS_MAC
        #define HUNTMASTER_PLATFORM_MACOS 1
    #else
        #warning "Unknown Apple platform detected"
    #endif

// --- Windows ---
#elif defined(_WIN32) || defined(_WIN64)
    #define HUNTMASTER_PLATFORM_WINDOWS 1
    #define HUNTMASTER_PLATFORM_NATIVE 1

// --- Linux ---
#elif defined(__linux__)
    #define HUNTMASTER_PLATFORM_LINUX 1
    #define HUNTMASTER_PLATFORM_NATIVE 1

#else
    #warning "Could not detect target platform for Huntmaster Engine"

#endif

