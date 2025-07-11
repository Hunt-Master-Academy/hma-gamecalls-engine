# ==============================================================================
# CMakeLists.txt for Huntmaster Audio Engine (v4.0 - Modernized & Unified)
#
# This script handles both NATIVE builds (for testing tools) and
# WEB ASSEMBLY builds (for the web app) using modern CMake practices.
#
# To build for NATIVE:
#   cmake -B build
#   cmake --build build
#
# To build for WEB ASSEMBLY:
#   emcmake cmake -B build-wasm
#   cmake --build build-wasm
# ==============================================================================

cmake_minimum_required(VERSION 3.15)

project(
    HuntmasterEngine
    VERSION 4.0
    LANGUAGES CXX C
)

# --- Project Structure Variables ---
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(PROJECT_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(PROJECT_TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# --- Platform Detection and Configuration ---
if(EMSCRIPTEN)
    add_compile_definitions(HUNTMASTER_SINGLE_THREADED)
    add_compile_definitions(HUNTMASTER_NO_DIRECT_AUDIO)
    add_compile_definitions(HUNTMASTER_PLATFORM_WASM)
elseif(ANDROID)
    add_compile_definitions(HUNTMASTER_PLATFORM_ANDROID)
elseif(IOS)
    add_compile_definitions(HUNTMASTER_PLATFORM_IOS)
elseif(WIN32)
    add_compile_definitions(HUNTMASTER_PLATFORM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(HUNTMASTER_PLATFORM_MACOS)
elseif(UNIX)
    add_compile_definitions(HUNTMASTER_PLATFORM_LINUX)
endif()

# --- Third-Party Dependencies ---
# 1. KissFFT (Vendored)
if(EMSCRIPTEN)
    # For WASM, use KissFFT as header-only
    add_library(kissfft INTERFACE)
    target_include_directories(kissfft INTERFACE ${PROJECT_LIBS_DIR}/kissfft)
    target_compile_definitions(kissfft INTERFACE kiss_fft_scalar=float)
    message(STATUS "KissFFT configured as header-only for WASM")
else()
    # Use full KissFFT for native builds
    add_subdirectory(${PROJECT_LIBS_DIR}/kissfft)
    message(STATUS "KissFFT configured as library for native build")
endif()

# 2. Google Test (Fetched at configure time)
# Note: Google Test will only be built for native targets.
if(NOT EMSCRIPTEN) # Native build only
    # Using a Git submodule for Google Test is more robust than FetchContent,
    # especially in Docker environments, as it avoids filesystem permission issues.
    if(EXISTS ${PROJECT_TESTS_DIR}/lib/googletest/CMakeLists.txt)
        add_subdirectory(${PROJECT_TESTS_DIR}/lib/googletest)
        message(STATUS "Google Test configured.")
    else()
        message(WARNING "Google Test not found. Unit tests will not be built.")
    endif()
endif()

# ==============================================================================
# Build Logic: Switch between Native and WebAssembly
# ==============================================================================

if(EMSCRIPTEN)
    # --------------------------------------------------------------------------
    # --- WEB ASSEMBLY (EMSCRIPTEN) BUILD ---
    # --------------------------------------------------------------------------
    message(STATUS "Configuring for WebAssembly (Emscripten) build.")

    # Define the sources needed for the WASM module.
    set(WASM_SOURCES
        "${PROJECT_SOURCE_DIR}/platform/wasm/WASMInterface.cpp"
        "${PROJECT_SOURCE_DIR}/core/MFCCProcessor.cpp"
        "${PROJECT_SOURCE_DIR}/core/DTWComparator.cpp"
        "${PROJECT_SOURCE_DIR}/core/RealtimeAudioProcessor.cpp"
        "${PROJECT_SOURCE_DIR}/core/VoiceActivityDetector.cpp"
        "${PROJECT_SOURCE_DIR}/core/AudioBufferPool.cpp"
        "${PROJECT_SOURCE_DIR}/core/HuntmasterEngine.cpp"
        "${PROJECT_SOURCE_DIR}/ThirdPartyLibs.cpp"
    )

    # Create the final WASM executable (which produces .js and .wasm files)
    add_executable(huntmaster_wasm ${WASM_SOURCES})

    # Set include directories for the WASM target
    target_include_directories(huntmaster_wasm PRIVATE
        ${PROJECT_INCLUDE_DIR}
        ${PROJECT_LIBS_DIR}
    )

    # Set C++ standard - use C++20 for better compatibility with Emscripten
    target_compile_features(huntmaster_wasm PRIVATE cxx_std_20)

    # Link against KissFFT
    target_link_libraries(huntmaster_wasm PRIVATE kissfft)
    target_compile_definitions(huntmaster_wasm PRIVATE HAVE_KISSFFT)

    # Add Emscripten-specific compiler options
    target_compile_options(huntmaster_wasm PRIVATE
    -O3                     # Optimization level
    -fno-exceptions         # Disable exceptions for smaller size
    # -fno-rtti removed - embind requires RTTI
    -Wall                  # Enable warnings
    -Wextra                # Enable extra warnings
    )

    # Add Emscripten-specific linker options
    target_link_options(huntmaster_wasm PRIVATE
        -O3
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=16777216              # 16MB initial memory
        -sMAXIMUM_MEMORY=268435456             # 256MB max memory
        -sMODULARIZE=1
        -sEXPORT_NAME=HuntmasterEngine
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue']"
        -sENVIRONMENT=web
        -sSINGLE_FILE=0                        # Separate .wasm file
        --bind                                 # Enable embind
    )

    # Create output directory if it doesn't exist
    add_custom_command(TARGET huntmaster_wasm PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/web/dist
    )

    # Set the final output name and directory
    set_target_properties(huntmaster_wasm PROPERTIES
        OUTPUT_NAME "huntmaster_engine"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web/dist"
    )

else()
    # --------------------------------------------------------------------------
    # --- NATIVE BUILD (Windows, Linux, macOS) ---
    # --------------------------------------------------------------------------
    message(STATUS "Configuring for Native build.")

    # Define the core library with all platform-agnostic C++ code.
    add_library(
        HuntmasterEngine STATIC
        "${PROJECT_SOURCE_DIR}/core/HuntmasterEngine.cpp"
        "${PROJECT_SOURCE_DIR}/core/AudioBufferPool.cpp"
        "${PROJECT_SOURCE_DIR}/core/VoiceActivityDetector.cpp"
        "${PROJECT_SOURCE_DIR}/core/MFCCProcessor.cpp"
        "${PROJECT_SOURCE_DIR}/core/DTWComparator.cpp"
        "${PROJECT_SOURCE_DIR}/core/RealtimeAudioProcessor.cpp"
        "${PROJECT_SOURCE_DIR}/ThirdPartyLibs.cpp"
    )

    # Set public include directories for the library and any targets that link to it.
    target_include_directories(HuntmasterEngine PUBLIC
        ${PROJECT_INCLUDE_DIR}
        ${PROJECT_LIBS_DIR}
        ${CMAKE_SOURCE_DIR}/include/huntmaster/core
        ${CMAKE_SOURCE_DIR}/include/huntmaster/platform/wasm
    )

    # Use C++20 for better compatibility across platforms
    target_compile_features(HuntmasterEngine PUBLIC cxx_std_20)
    
    # Link libraries
    target_link_libraries(HuntmasterEngine PUBLIC kissfft)
    target_compile_definitions(HuntmasterEngine PUBLIC HAVE_KISSFFT)

    # Platform-specific libraries
    if(WIN32)
        target_link_libraries(HuntmasterEngine PUBLIC winmm)  # For Windows audio
    elseif(APPLE)
        find_library(COREAUDIO_LIBRARY CoreAudio)
        find_library(AUDIOUNIT_LIBRARY AudioUnit)
        find_library(COREFOUNDATION_LIBRARY CoreFoundation)
        target_link_libraries(HuntmasterEngine PUBLIC 
            ${COREAUDIO_LIBRARY} 
            ${AUDIOUNIT_LIBRARY} 
            ${COREFOUNDATION_LIBRARY}
        )
    elseif(UNIX)
        find_package(Threads REQUIRED)
        target_link_libraries(HuntmasterEngine PUBLIC Threads::Threads)
        # Add ALSA or PulseAudio if needed
    endif()

    # Note: The following is for IDE organization and does not affect the build.
    source_group("Source Files/Core" FILES
        "${PROJECT_SOURCE_DIR}/core/HuntmasterEngine.cpp"
        "${PROJECT_SOURCE_DIR}/core/AudioBufferPool.cpp"
        "${PROJECT_SOURCE_DIR}/core/VoiceActivityDetector.cpp"
        "${PROJECT_SOURCE_DIR}/core/MFCCProcessor.cpp"
        "${PROJECT_SOURCE_DIR}/core/DTWComparator.cpp"
        "${PROJECT_SOURCE_DIR}/core/RealtimeAudioProcessor.cpp"
    )
    source_group("Source Files/Common" FILES "${PROJECT_SOURCE_DIR}/ThirdPartyLibs.cpp")

    # Helper function to create all our command-line test tools
    function(add_huntmaster_tool tool_name file_name)
        if(EXISTS "${PROJECT_TOOLS_DIR}/${file_name}")
            add_executable(${tool_name} "${PROJECT_TOOLS_DIR}/${file_name}")
            target_link_libraries(${tool_name} PRIVATE HuntmasterEngine)
        else()
            message(WARNING "Tool source not found: ${file_name}")
        endif()
    endfunction()
    
    # --- Executable Targets (Tools) ---
    message(STATUS "Configuring native tools and executables...")
    add_huntmaster_tool(GenerateFeatures generate_features.cpp)
    add_huntmaster_tool(AnalyzeRecording analyze_recording.cpp)
    add_huntmaster_tool(AudioTrimmer audio_trimmer.cpp)
    add_huntmaster_tool(InteractiveRecorder interactive_recorder.cpp)
    add_huntmaster_tool(FindBestMatch find_best_match.cpp)
    add_huntmaster_tool(AudioVisualization audio_visualization.cpp)
    add_huntmaster_tool(RealTimeMonitor real_time_recording_monitor.cpp)
    add_huntmaster_tool(DetailedAnalysis detailed_analysis.cpp)

    # --- Unit Testing Suite ---
    if(TARGET GTest::gtest_main)
        message(STATUS "Configuring unit tests...")
        enable_testing()
        
        set(TEST_SOURCES
            "${PROJECT_TESTS_DIR}/unit/AudioBufferPoolTest.cpp"
            "${PROJECT_TESTS_DIR}/unit/DTWComparatorTest.cpp"
            "${PROJECT_TESTS_DIR}/unit/RealTimeAudioProcessorTest.cpp"
            "${PROJECT_TESTS_DIR}/unit/VoiceActivityDetectorTest.cpp"
            "${PROJECT_TESTS_DIR}/integration/EndToEndTest.cpp"
        )
        
        # Only add test files that exist
        set(EXISTING_TEST_SOURCES)
        foreach(test_file ${TEST_SOURCES})
            if(EXISTS ${test_file})
                list(APPEND EXISTING_TEST_SOURCES ${test_file})
            else()
                message(WARNING "Test file not found: ${test_file}")
            endif()
        endforeach()
        
        if(EXISTING_TEST_SOURCES)
            add_executable(RunEngineTests ${EXISTING_TEST_SOURCES})
            target_link_libraries(RunEngineTests PRIVATE HuntmasterEngine GTest::gtest_main)
            include(GoogleTest)
            gtest_discover_tests(RunEngineTests)
        endif()
    endif()

    # --- Benchmarking Suite ---
    if(EXISTS ${PROJECT_TESTS_DIR}/benchmarks/CMakeLists.txt)
        message(STATUS "Configuring benchmarks...")
        add_subdirectory(${PROJECT_TESTS_DIR}/benchmarks)
    endif()

endif()

# --- Installation Rules (Optional) ---
if(NOT EMSCRIPTEN)
    include(GNUInstallDirs)
    
    # Install the library
    install(TARGETS HuntmasterEngine
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    
    # Install headers
    install(DIRECTORY ${PROJECT_INCLUDE_DIR}/huntmaster
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

# --- Final Configuration Message ---
message(STATUS "=====================================")
message(STATUS "Huntmaster Engine Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: C++20")
if(EMSCRIPTEN)
    message(STATUS "  Platform: WebAssembly (Emscripten)")
    message(STATUS "  Output: web/dist/")
else()
    message(STATUS "  Platform: Native")
    message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
endif()
message(STATUS "=====================================")