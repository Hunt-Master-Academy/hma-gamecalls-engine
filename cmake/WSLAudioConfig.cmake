# WSL Audio Configuration for CMake builds
# Handles ALSA issues and provides mock audio device support

# Detect WSL environment
if(EXISTS "/proc/version")
    file(READ "/proc/version" PROC_VERSION)
    if(PROC_VERSION MATCHES "Microsoft|WSL")
        set(WSL_ENVIRONMENT TRUE)
        message(STATUS "WSL environment detected - configuring audio fallbacks")
    endif()
endif()

# Configure audio backends for WSL
if(WSL_ENVIRONMENT)
    # Use mock audio devices
    add_compile_definitions(WSL_AUDIO_MOCK=1)
    add_compile_definitions(ALSA_FALLBACK_NULL=1)
    
    # Disable hardware-dependent audio features
    add_compile_definitions(DISABLE_HARDWARE_AUDIO=1)
    
    # Set test-friendly audio paths
    add_compile_definitions(WSL_TEST_AUDIO_PATH="/tmp/wsl_audio_test/")
    
    message(STATUS "WSL audio configuration applied")
endif()
