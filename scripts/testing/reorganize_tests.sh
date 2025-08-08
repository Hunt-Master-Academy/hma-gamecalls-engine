#!/bin/bash

# Test File Reorganization Script
# This script moves test files from the current structure to a cleaner, categorized structure

set -e

echo "=== Huntmaster Engine Test Reorganization ==="
echo "Moving test files to categorized directory structure..."

# Source and destination base directories
SRC_BASE="/workspaces/huntmaster-engine/tests"
DEST_BASE="/workspaces/huntmaster-engine/tests_new"

# Create all necessary directories
echo "Creating directory structure..."

# Unit test categories
mkdir -p "$DEST_BASE/unit/core"           # Core engine functionality
mkdir -p "$DEST_BASE/unit/audio"          # Audio processing components
mkdir -p "$DEST_BASE/unit/analysis"       # Analysis algorithms (MFCC, DTW, etc.)
mkdir -p "$DEST_BASE/unit/security"       # Security and access control
mkdir -p "$DEST_BASE/unit/utils"          # Utilities and helpers
mkdir -p "$DEST_BASE/unit/vad"            # Voice Activity Detection

# Integration test categories  
mkdir -p "$DEST_BASE/integration/audio_pipeline"      # Full audio pipeline tests
mkdir -p "$DEST_BASE/integration/enhanced_analyzers"  # Enhanced analyzer integration
mkdir -p "$DEST_BASE/integration/real_world"          # Real-world wildlife call tests
mkdir -p "$DEST_BASE/integration/web"                 # Web interface integration

# Performance test categories
mkdir -p "$DEST_BASE/performance/benchmarks"   # Performance benchmarks
mkdir -p "$DEST_BASE/performance/profiling"    # Performance profiling tests

# Tools and utilities
mkdir -p "$DEST_BASE/tools"        # Testing tools and debugging utilities
mkdir -p "$DEST_BASE/lib"          # Test libraries and utilities

# Function to move file and log the action
move_file() {
    local src="$1"
    local dest="$2"
    
    if [[ -f "$src" ]]; then
        echo "Moving: $(basename "$src") -> $dest"
        mv "$src" "$dest"
    else
        echo "Warning: File not found: $src"
    fi
}

echo ""
echo "=== Moving Core Engine Tests ==="
# Core engine functionality tests
move_file "$SRC_BASE/core/test_audio_processing.cpp" "$DEST_BASE/unit/core/"
move_file "$SRC_BASE/core/test_session_management.cpp" "$DEST_BASE/unit/core/"
move_file "$SRC_BASE/core/test_master_call_management.cpp" "$DEST_BASE/unit/core/"
move_file "$SRC_BASE/core/test_recording_system.cpp" "$DEST_BASE/unit/core/"

echo ""
echo "=== Moving Audio Processing Tests ==="
# Audio processing and buffer management
move_file "$SRC_BASE/unit/AudioBufferPoolTest.cpp" "$DEST_BASE/unit/audio/"
move_file "$SRC_BASE/unit/RealTimeAudioProcessorTest.cpp" "$DEST_BASE/unit/audio/"
move_file "$SRC_BASE/unit/test_audio_player.cpp" "$DEST_BASE/unit/audio/"
move_file "$SRC_BASE/unit/test_recorder.cpp" "$DEST_BASE/unit/audio/"
move_file "$SRC_BASE/unit/test_recording.cpp" "$DEST_BASE/unit/audio/"
move_file "$SRC_BASE/unit/test_audio_level_processor.cpp" "$DEST_BASE/unit/audio/"
move_file "$SRC_BASE/unit/test_circular_audio_buffer.cpp" "$DEST_BASE/unit/audio/"
move_file "$SRC_BASE/unit/test_streaming_audio_processor.cpp" "$DEST_BASE/unit/audio/"
move_file "$SRC_BASE/unit/test_audio_format_converter.cpp" "$DEST_BASE/unit/audio/"
move_file "$SRC_BASE/unit/test_waveform_generator.cpp" "$DEST_BASE/unit/audio/"
move_file "$SRC_BASE/unit/test_spectrogram_processor.cpp" "$DEST_BASE/unit/audio/"

echo ""
echo "=== Moving Analysis Algorithm Tests ==="
# Analysis algorithms (MFCC, DTW, Pitch, Harmonic, Cadence)
move_file "$SRC_BASE/unit/DTWComparatorTest.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/dtw_tests.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/dtw_tests_unified.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_dtw_comprehensive.cpp" "$DEST_BASE/unit/analysis/"

# MFCC tests
move_file "$SRC_BASE/unit/test_mfcc_direct.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_mfcc_direct_unified.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_mfcc_direct_validation.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_mfcc_edge_cases.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_mfcc_consistency_unified.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_mfcc_coverage_boost_targeted.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_mfcc_error_paths.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_mfcc_error_paths_fixed.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/core/test_mfcc_minimal.cpp" "$DEST_BASE/unit/analysis/"

# Pitch, Harmonic, and Cadence analyzers
move_file "$SRC_BASE/unit/test_pitch_tracker_comprehensive.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_harmonic_analyzer_comprehensive.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_cadence_analyzer_comprehensive.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_cadence_analyzer_comprehensive_fixed.cpp" "$DEST_BASE/unit/analysis/"
move_file "$SRC_BASE/unit/test_waveform_analyzer_comprehensive.cpp" "$DEST_BASE/unit/analysis/"

# Realtime scorer
move_file "$SRC_BASE/unit/test_realtime_scorer.cpp" "$DEST_BASE/unit/analysis/"

echo ""
echo "=== Moving Voice Activity Detection Tests ==="
# VAD tests
move_file "$SRC_BASE/unit/VoiceActivityDetectorTest.cpp" "$DEST_BASE/unit/vad/"
move_file "$SRC_BASE/unit/SimpleVADTest.cpp" "$DEST_BASE/unit/vad/"
move_file "$SRC_BASE/unit/VADActiveTransitionTest.cpp" "$DEST_BASE/unit/vad/"
move_file "$SRC_BASE/unit/VADCandidateTransitionTest.cpp" "$DEST_BASE/unit/vad/"
move_file "$SRC_BASE/unit/VADDiagnosticTest.cpp" "$DEST_BASE/unit/vad/"
move_file "$SRC_BASE/unit/VADStateTransitionTest.cpp" "$DEST_BASE/unit/vad/"
move_file "$SRC_BASE/unit/VADThresholdTest.cpp" "$DEST_BASE/unit/vad/"
move_file "$SRC_BASE/unit/StateMachineTest.cpp" "$DEST_BASE/unit/vad/"
move_file "$SRC_BASE/unit/EnergyComparisonTest.cpp" "$DEST_BASE/unit/vad/"
move_file "$SRC_BASE/unit/core/test_vad_manual.cpp" "$DEST_BASE/unit/vad/"

echo ""
echo "=== Moving Security Tests ==="
# Security and access control
move_file "$SRC_BASE/unit/test_access_controller.cpp" "$DEST_BASE/unit/security/"
move_file "$SRC_BASE/unit/test_access_controller_comprehensive.cpp" "$DEST_BASE/unit/security/"
move_file "$SRC_BASE/unit/test_crypto_manager.cpp" "$DEST_BASE/unit/security/"
move_file "$SRC_BASE/unit/test_crypto_manager_comprehensive.cpp" "$DEST_BASE/unit/security/"
move_file "$SRC_BASE/unit/test_memory_guard.cpp" "$DEST_BASE/unit/security/"
move_file "$SRC_BASE/unit/test_memory_protection_comprehensive.cpp" "$DEST_BASE/unit/security/"
move_file "$SRC_BASE/unit/test_memory_management_comprehensive.cpp" "$DEST_BASE/unit/security/"
move_file "$SRC_BASE/unit/test_input_validator.cpp" "$DEST_BASE/unit/security/"
move_file "$SRC_BASE/unit/test_input_validator_comprehensive.cpp" "$DEST_BASE/unit/security/"
move_file "$SRC_BASE/unit/security/MemoryProtectionComprehensiveTest.cpp" "$DEST_BASE/unit/security/"

echo ""
echo "=== Moving Utility and Debug Tests ==="
# Utilities, debugging, and validation
move_file "$SRC_BASE/unit/test_debug_logger.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/DebugTest.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_errormonitor.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_error_handling_comprehensive.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_error_logging_system.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_audit_logger.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_audit_logger_comprehensive.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_validation.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_validation_unified.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_basic_coverage.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_branch_coverage_boost.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_branch_coverage_boost_corrected.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_coverage_optimizer.cpp" "$DEST_BASE/unit/utils/"
move_file "$SRC_BASE/unit/test_coverage_validator.cpp" "$DEST_BASE/unit/utils/"

echo ""
echo "=== Moving Unified Engine Tests ==="
# Unified engine tests
move_file "$SRC_BASE/unit/test_unified_engine.cpp" "$DEST_BASE/unit/core/"
move_file "$SRC_BASE/unit/test_unified_engine_advanced.cpp" "$DEST_BASE/unit/core/"
move_file "$SRC_BASE/unit/test_unified_engine_comprehensive.cpp" "$DEST_BASE/unit/core/"
move_file "$SRC_BASE/unit/test_unified_engine_vad_config.cpp" "$DEST_BASE/unit/core/"

echo ""
echo "=== Moving Master Call Tests ==="
# Master call management
move_file "$SRC_BASE/unit/test_master_call_comprehensive.cpp" "$DEST_BASE/unit/core/"

echo ""
echo "=== Moving Integration Tests ==="
# Integration tests
move_file "$SRC_BASE/integration/test_audio_pipeline.cpp" "$DEST_BASE/integration/audio_pipeline/"
move_file "$SRC_BASE/integration/EnhancedAnalyzersIntegrationTest.cpp" "$DEST_BASE/integration/enhanced_analyzers/"
move_file "$SRC_BASE/integration/EnhancedAnalyzersPerformanceTest.cpp" "$DEST_BASE/integration/enhanced_analyzers/"
move_file "$SRC_BASE/integration/test_EnhancedAnalysisProcessorIntegration.cpp" "$DEST_BASE/integration/enhanced_analyzers/"
move_file "$SRC_BASE/enhanced/test_EnhancedAnalysisProcessor.cpp" "$DEST_BASE/integration/enhanced_analyzers/"
move_file "$SRC_BASE/integration/RealWildlifeCallAnalysisTest.cpp" "$DEST_BASE/integration/real_world/"
move_file "$SRC_BASE/integration/test_wasm_integration.cpp" "$DEST_BASE/integration/web/"

echo ""
echo "=== Moving Performance Tests ==="
# Performance tests
move_file "$SRC_BASE/unit/test_performance.cpp" "$DEST_BASE/performance/benchmarks/"
move_file "$SRC_BASE/unit/test_performance_profiler.cpp" "$DEST_BASE/performance/profiling/"
move_file "$SRC_BASE/unit/test_performance_profiler_comprehensive.cpp" "$DEST_BASE/performance/profiling/"

echo ""
echo "=== Moving Tools and Utilities ==="
# Tools and debugging utilities
move_file "$SRC_BASE/tools/run_advanced_test.cpp" "$DEST_BASE/tools/"
move_file "$SRC_BASE/tools/test_mfcc_debugging.cpp" "$DEST_BASE/tools/"
move_file "$SRC_BASE/tools/test_alpha_deployment_ready.cpp" "$DEST_BASE/tools/"

# Test libraries
move_file "$SRC_BASE/lib/TestUtils.cpp" "$DEST_BASE/lib/"

echo ""
echo "=== Moving Remaining Tests ==="
# Catch any remaining tests that don't fit the above categories
for file in $(find "$SRC_BASE" -name "*.cpp" -type f); do
    if [[ -f "$file" ]]; then
        filename=$(basename "$file")
        echo "Moving remaining file: $filename -> unit/utils/"
        mv "$file" "$DEST_BASE/unit/utils/"
    fi
done

echo ""
echo "=== Moving Supporting Files ==="
# Move CMakeLists.txt and other supporting files
if [[ -f "$SRC_BASE/CMakeLists.txt" ]]; then
    echo "Moving CMakeLists.txt"
    cp "$SRC_BASE/CMakeLists.txt" "$DEST_BASE/"
fi

# Move any header files in lib
if [[ -d "$SRC_BASE/lib" ]]; then
    find "$SRC_BASE/lib" -name "*.h" -o -name "*.hpp" | while read -r file; do
        echo "Moving header: $(basename "$file")"
        cp "$file" "$DEST_BASE/lib/"
    done
fi

echo ""
echo "=== Reorganization Complete ==="
echo "All test files have been moved to the new categorized structure."
echo "Directory structure:"
echo "tests_new/"
echo "├── unit/"
echo "│   ├── core/           # Core engine tests"
echo "│   ├── audio/          # Audio processing tests" 
echo "│   ├── analysis/       # Analysis algorithm tests"
echo "│   ├── vad/           # Voice Activity Detection tests"
echo "│   ├── security/       # Security and access control tests"
echo "│   └── utils/          # Utility and debug tests"
echo "├── integration/"
echo "│   ├── audio_pipeline/ # Full pipeline integration tests"
echo "│   ├── enhanced_analyzers/ # Enhanced analyzer integration"
echo "│   ├── real_world/     # Real-world audio tests"
echo "│   └── web/           # Web interface integration"
echo "├── performance/"
echo "│   ├── benchmarks/     # Performance benchmarks"
echo "│   └── profiling/      # Performance profiling"
echo "├── tools/             # Testing tools and utilities"
echo "└── lib/               # Test libraries and headers"
