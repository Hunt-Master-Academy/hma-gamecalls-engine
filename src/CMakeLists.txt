# ==============================================================================
# src/CMakeLists.txt - Core Engine Library
#
# This file is responsible for defining and building the UnifiedAudioEngine
# static library and WASM executable. It contains all platform-agnostic
# engine code shared between native and WASM builds.
# ==============================================================================

message(STATUS "Configuring core UnifiedAudioEngine library...")

# ==============================================================================
# Define Core Engine Sources
# ==============================================================================

# Define the core sources for the modern UnifiedAudioEngine architecture
set(UNIFIED_ENGINE_CORE_SOURCES
    "${PROJECT_SOURCE_DIR}/core/UnifiedAudioEngine.cpp"
    "${PROJECT_SOURCE_DIR}/core/AudioBufferPool.cpp"
    "${PROJECT_SOURCE_DIR}/core/VoiceActivityDetector.cpp"
    "${PROJECT_SOURCE_DIR}/core/MFCCProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/core/DTWComparator.cpp"
    "${PROJECT_SOURCE_DIR}/core/RealTimeAudioProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/core/WaveformGenerator.cpp"
    "${PROJECT_SOURCE_DIR}/core/AudioPlayer.cpp"
    "${PROJECT_SOURCE_DIR}/core/AudioRecorder.cpp"
    "${PROJECT_SOURCE_DIR}/core/DTWProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/core/ThirdPartyLibs.cpp"
    "${PROJECT_SOURCE_DIR}/core/AudioLevelProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/core/SpectrogramProcessor.cpp"
    "${PROJECT_SOURCE_DIR}/core/DebugLogger.cpp"
    "${PROJECT_SOURCE_DIR}/core/RealtimeScorer.cpp"
    "${PROJECT_SOURCE_DIR}/core/ErrorLogger.cpp"
    "${PROJECT_SOURCE_DIR}/core/ComponentErrorHandler.cpp"
    "${PROJECT_SOURCE_DIR}/core/ErrorMonitor.cpp"
    "${PROJECT_SOURCE_DIR}/core/PerformanceProfiler.cpp"
    # Phase 1: Enhanced Analysis Features (template compatibility fixes needed)
    # "${PROJECT_SOURCE_DIR}/core/PitchTracker.cpp"
    # "${PROJECT_SOURCE_DIR}/core/HarmonicAnalyzer.cpp"
    # "${PROJECT_SOURCE_DIR}/core/CadenceAnalyzer.cpp"
)

# Add performance profiling sources
set(PROFILING_SOURCES
    "${PROJECT_SOURCE_DIR}/profiling/PerformanceProfiler.cpp"
)

# Add security framework sources
set(SECURITY_SOURCES
    "${PROJECT_SOURCE_DIR}/security/access-controller.cpp"
    "${PROJECT_SOURCE_DIR}/security/audit-logger.cpp"
    "${PROJECT_SOURCE_DIR}/security/crypto-manager.cpp"
    "${PROJECT_SOURCE_DIR}/security/input-validator.cpp"
    "${PROJECT_SOURCE_DIR}/security/memory-guard.cpp"
    "${PROJECT_SOURCE_DIR}/security/memory-protection.cpp"
)

# Add visualization sources
set(VISUALIZATION_SOURCES
    "${PROJECT_SOURCE_DIR}/visualization/WaveformAnalyzer.cpp"
)

# ==============================================================================
# Create Core Engine Library
# ==============================================================================

# Create the unified engine library
add_library(UnifiedAudioEngine STATIC ${UNIFIED_ENGINE_CORE_SOURCES} ${PROFILING_SOURCES} ${SECURITY_SOURCES} ${VISUALIZATION_SOURCES})

# Set public include directories. Any target linking to UnifiedAudioEngine will inherit these.
target_include_directories(UnifiedAudioEngine PUBLIC
    ${PROJECT_INCLUDE_DIR}
    # Add libs dir for header-only libraries like dr_wav.h and miniaudio.h
    ${PROJECT_LIBS_DIR}/dr_wav
    ${PROJECT_LIBS_DIR}/miniaudio
    ${PROJECT_TESTS_DIR}/lib/googletest
    ${PROJECT_LIBS_DIR}
)

# Include KissFFT headers from FetchContent
target_include_directories(UnifiedAudioEngine PRIVATE
    ${kissfft_SOURCE_DIR}
)

# Link core dependencies - KissFFT is now available via FetchContent
target_link_libraries(UnifiedAudioEngine PUBLIC kissfft)
target_compile_definitions(UnifiedAudioEngine PUBLIC HAVE_KISSFFT)

# Link FFTW3 if available
if(FFTW3_FOUND)
    if(FFTW3_LIBRARIES)
        target_link_libraries(UnifiedAudioEngine PUBLIC ${FFTW3_LIBRARIES})
    else()
        target_link_libraries(UnifiedAudioEngine PUBLIC ${FFTW3_LIBRARY})
    endif()

    if(FFTW3_INCLUDE_DIRS)
        target_include_directories(UnifiedAudioEngine PUBLIC ${FFTW3_INCLUDE_DIRS})
    else()
        target_include_directories(UnifiedAudioEngine PUBLIC ${FFTW3_INCLUDE_DIR})
    endif()

    target_compile_definitions(UnifiedAudioEngine PUBLIC HAVE_FFTW3)
    message(STATUS "FFTW3 support enabled for WaveformAnalyzer")
else()
    target_compile_definitions(UnifiedAudioEngine PUBLIC FFTW_DISABLE)
    message(STATUS "FFTW3 support disabled - using stub implementation")
endif()

# ==============================================================================
# Platform-Specific Configuration
# ==============================================================================

if(EMSCRIPTEN)
    # --------------------------------------------------------------------------
    # WebAssembly (Emscripten) Build
    # --------------------------------------------------------------------------
    message(STATUS "Configuring for WebAssembly (Emscripten) build.")

    # Define the sources needed specifically for the WASM module
    # This includes the Enhanced WASM Interface with comprehensive functionality
    set(WASM_INTERFACE_SOURCES
        "${PROJECT_SOURCE_DIR}/platform/wasm/EnhancedWASMInterface.cpp"
    )

    # Create the final WASM executable (which produces .js and .wasm files)
    add_executable(huntmaster_wasm ${WASM_INTERFACE_SOURCES})

    # Link the WASM executable against the unified engine library
    target_link_libraries(huntmaster_wasm PRIVATE UnifiedAudioEngine)

    # Add Emscripten-specific compiler options
    target_compile_options(huntmaster_wasm PRIVATE
        -O3                 # Optimization level
        -fno-exceptions     # Disable exceptions for smaller size
        # -fno-rtti was correctly removed, as embind requires it
        -Wall
        -Wextra
    )

    # Add Emscripten-specific linker options
    target_link_options(huntmaster_wasm PRIVATE
        -O3
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=16777216      # 16MB initial memory
        -sMAXIMUM_MEMORY=268435456     # 256MB max memory
        -sMODULARIZE=1
        -sEXPORT_NAME=HuntmasterEngine
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue']"
        -sENVIRONMENT=web
        -sSINGLE_FILE=0                # Separate .wasm file is good practice
        --bind                         # Enable embind
    )

    # Set the final output name and directory for the WASM files
    set_target_properties(huntmaster_wasm PROPERTIES
        OUTPUT_NAME "huntmaster_engine"
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_WEB_DIR}/dist"
    )

else()
    # --------------------------------------------------------------------------
    # Native Build (Windows, Linux, macOS)
    # --------------------------------------------------------------------------
    message(STATUS "Configuring for Native build.")

    # Link platform-specific libraries for native audio, threading, etc.
    if(WIN32)
        target_link_libraries(UnifiedAudioEngine PUBLIC winmm)
    elseif(APPLE)
        find_library(COREAUDIO_LIBRARY CoreAudio REQUIRED)
        find_library(AUDIOUNIT_LIBRARY AudioUnit REQUIRED)
        find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
        target_link_libraries(UnifiedAudioEngine PUBLIC
            ${COREAUDIO_LIBRARY}
            ${AUDIOUNIT_LIBRARY}
            ${COREFOUNDATION_LIBRARY}
        )
    elseif(UNIX AND NOT ANDROID)
        find_package(Threads REQUIRED)
        target_link_libraries(UnifiedAudioEngine PUBLIC Threads::Threads)
        # For audio, you might need to find and link ALSA or PulseAudio here
        # find_package(ALSA)
        # if(ALSA_FOUND)
        #   target_link_libraries(UnifiedAudioEngine PUBLIC ${ALSA_LIBRARIES})
        # endif()
    endif()

    # --- Installation Rules ---
    include(GNUInstallDirs)
    install(TARGETS UnifiedAudioEngine
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(DIRECTORY ${PROJECT_INCLUDE_DIR}/huntmaster
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

endif()

message(STATUS "UnifiedAudioEngine library configuration complete.")
