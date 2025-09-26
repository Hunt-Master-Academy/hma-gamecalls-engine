# ==============================================================================
# tools/CMakeLists.txt - Command-Line Tools Configuration
#
# This file is responsible for defining all command-line tool executables
# and linking them against the UnifiedAudioEngine library. These tools
# provide utilities for audio processing, analysis, and debugging.
# ==============================================================================

message(STATUS "Configuring native tools...")

# Ensure nlohmann_json is available
if(NOT TARGET nlohmann_json::nlohmann_json)
 message(FATAL_ERROR "nlohmann_json not available; ensure top-level CMake FetchContent declared it")
endif()

# ==============================================================================
# Automatic Tool Discovery and Configuration
# ==============================================================================

# Find all .cpp files in the tools directory
file(GLOB TOOL_SOURCES "${PROJECT_TOOLS_DIR}/*.cpp")

# Exclude tools that are handled by other CMake files
list(REMOVE_ITEM TOOL_SOURCES "${PROJECT_TOOLS_DIR}/IOOptimizationDemo.cpp")

# Create executable for each tool source file
foreach(tool_source_path ${TOOL_SOURCES})
 # Extract the tool name from the filename (without extension)
 get_filename_component(tool_name ${tool_source_path} NAME_WE)

 # Create executable target
 add_executable(${tool_name} ${tool_source_path})

 # Link against the core engine library and json when needed
 target_link_libraries(${tool_name} PRIVATE UnifiedAudioEngine nlohmann_json::nlohmann_json)

 # Ensure executable goes to build/bin directory
 set_target_properties(${tool_name} PROPERTIES
 RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
 )

 # Provide status feedback
 message(STATUS " -- Added tool: ${tool_name}")
endforeach()

# ==============================================================================
# Tool Descriptions (for documentation)
# ==============================================================================

# The following tools are automatically configured:
#
# analyze_recording - Analyze pre-recorded audio files
# audio_trimmer - Trim audio files to specified segments
# audio_visualization - Generate visualizations from audio data
# debug_dtw_similarity - Debug DTW similarity calculations
# detailed_analysis - Perform detailed audio analysis
# generate_features - Extract MFCC and other features from audio
# interactive_recorder - Interactive recording and analysis tool
# real_time_recording_monitor - Monitor real-time recording sessions
# simple_unified_test - Simple test using the unified engine
# test_mfcc_debugging - Debug MFCC extraction processes

# ==============================================================================
# Platform-Specific Tool Configuration
# ==============================================================================

# Add platform-specific compiler flags if needed
if(WIN32)
 # Windows-specific tool configuration
 foreach(tool_source_path ${TOOL_SOURCES})
 get_filename_component(tool_name ${tool_source_path} NAME_WE)
 # Example: Set console subsystem for Windows
 # set_target_properties(${tool_name} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
 endforeach()
elseif(UNIX)
 # Unix-specific tool configuration
 foreach(tool_source_path ${TOOL_SOURCES})
 get_filename_component(tool_name ${tool_source_path} NAME_WE)
 # Example: Add Unix-specific compilation flags
 # target_compile_definitions(${tool_name} PRIVATE _POSIX_C_SOURCE=200809L)
 endforeach()
endif()

# ==============================================================================
# Installation Configuration
# ==============================================================================

# Install tools to the binary directory
foreach(tool_source_path ${TOOL_SOURCES})
 get_filename_component(tool_name ${tool_source_path} NAME_WE)

 install(TARGETS ${tool_name}
 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
 COMPONENT Tools
 )
endforeach()

message(STATUS "Tools configuration complete.")
