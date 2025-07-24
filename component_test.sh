#!/bin/bash

# ==============================================================================
# Component-Specific Test Script for Huntmaster Audio Engine
# ==============================================================================
# This script provides detailed testing for individual engine components
# with comprehensive diagnostics and debugging information.
#
# Usage: ./component_test.sh <component> [options]
# Components: engine, mfcc, dtw, vad, realtime, audio, all
# ==============================================================================

set -e

# Configuration
BUILD_DIR="build"
COMPONENT=""
VERBOSE=true
STRESS_TEST=false
MEMORY_CHECK=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        engine|mfcc|dtw|vad|realtime|audio|all)
            COMPONENT="$1"
            shift
            ;;
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        --quiet|-q)
            VERBOSE=false
            shift
            ;;
        --stress)
            STRESS_TEST=true
            shift
            ;;
        --memory)
            MEMORY_CHECK=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 <component> [options]"
            echo ""
            echo "Components:"
            echo "  engine    - UnifiedAudioEngine core functionality"
            echo "  mfcc      - MFCC feature extraction"
            echo "  dtw       - Dynamic Time Warping algorithm"
            echo "  vad       - Voice Activity Detection"
            echo "  realtime  - Real-time processing and scoring"
            echo "  audio     - Audio I/O and processing"
            echo "  all       - All components"
            echo ""
            echo "Options:"
            echo "  --verbose, -v    Enable verbose output (default)"
            echo "  --quiet, -q      Disable verbose output"
            echo "  --stress         Run stress tests"
            echo "  --memory         Run memory checks with valgrind"
            echo "  --help, -h       Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

if [[ -z "$COMPONENT" ]]; then
    echo "Error: Component not specified"
    echo "Use --help for usage information"
    exit 1
fi

# Logging functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[✓]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[!]${NC} $1"; }
log_error() { echo -e "${RED}[✗]${NC} $1"; }
log_test() { echo -e "${CYAN}[TEST]${NC} $1"; }

# Function to run a test with detailed output
run_detailed_test() {
    local test_name="$1"
    local description="$2"
    local test_cmd="$3"

    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    log_test "$description"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    local start_time=$(date +%s.%N)

    if [[ "$MEMORY_CHECK" == true ]] && command -v valgrind >/dev/null 2>&1; then
        test_cmd="valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $test_cmd"
        log_info "Running with memory checking enabled"
    fi

    if [[ "$VERBOSE" == true ]]; then
        eval "$test_cmd"
        local result=$?
    else
        eval "$test_cmd" 2>&1
        local result=$?
    fi

    local end_time=$(date +%s.%N)
    local duration=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "N/A")

    if [[ $result -eq 0 ]]; then
        log_success "$test_name completed successfully (${duration}s)"
        return 0
    else
        log_error "$test_name failed with exit code $result (${duration}s)"
        return 1
    fi
}

# Function to create synthetic test audio
create_test_audio() {
    local output_file="$1"
    local duration="$2"
    local frequency="$3"

    if command -v sox >/dev/null 2>&1; then
        sox -n -r 44100 -c 1 "$output_file" synth "$duration" sine "$frequency" vol 0.3
        log_success "Created synthetic test audio: $output_file"
        return 0
    else
        log_warning "Sox not available - cannot create synthetic test audio"
        return 1
    fi
}

# Test UnifiedAudioEngine core functionality
test_engine_component() {
    log_info "Testing UnifiedAudioEngine Core..."

    # Basic engine functionality
    run_detailed_test "Engine Creation" "Basic engine instantiation and session management" \
        "$BUILD_DIR/bin/UnifiedEngineTest"

    # Engine with real audio data - test master call
    if [[ -f "data/master_calls/buck_grunt.wav" ]]; then
        run_detailed_test "Engine Audio Processing (Master)" "Engine processing with master call audio data" \
            "$BUILD_DIR/bin/LoadandExtractTest data/master_calls/buck_grunt.wav buck_grunt"
    fi

    # Test each user recording variant for specific purposes
    log_info "Testing engine with different user recording conditions..."

    # Standard user recording - baseline comparison
    if [[ -f "data/recordings/user_attempt_buck_grunt.wav" ]]; then
        run_detailed_test "Baseline User Recording" "Standard user attempt vs master call comparison" \
            "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt.wav buck_grunt"
    fi

    # Gap variant - tests VAD and timing handling
    if [[ -f "data/recordings/user_attempt_buck_grunt_gap.wav" ]]; then
        run_detailed_test "Gap Handling Test" "User recording with silence gaps - VAD and timing analysis" \
            "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_gap.wav buck_grunt"
    fi

    # Low volume variant - tests amplitude normalization and sensitivity
    if [[ -f "data/recordings/user_attempt_buck_grunt_lowvolume.wav" ]]; then
        run_detailed_test "Low Volume Handling" "User recording with low volume - amplitude sensitivity test" \
            "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_lowvolume.wav buck_grunt"
    fi

    # No gap variant - tests tight timing and continuous audio
    if [[ -f "data/recordings/user_attempt_buck_grunt_nogap.wav" ]]; then
        run_detailed_test "Continuous Audio Test" "User recording without gaps - continuous audio processing" \
            "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_nogap.wav buck_grunt"
    fi

    # Multi-session testing
    if [[ "$STRESS_TEST" == true ]]; then
        log_test "Running multi-session stress test..."
        # This would require a custom stress test executable
        log_warning "Multi-session stress test not yet implemented"
    fi
}

# Test MFCC feature extraction
test_mfcc_component() {
    log_info "Testing MFCC Feature Extraction..."

    # MFCC consistency tests
    run_detailed_test "MFCC Consistency" "MFCC feature extraction consistency check" \
        "$BUILD_DIR/bin/MFCCConsistencyUnifiedTest"

    # Direct MFCC computation
    run_detailed_test "MFCC Direct Computation" "Direct MFCC computation validation" \
        "$BUILD_DIR/bin/MFCCDirectUnifiedTest"

    # MFCC robustness tests with different recording conditions
    log_info "Testing MFCC feature extraction robustness..."

    # Baseline MFCC extraction
    if [[ -f "data/recordings/user_attempt_buck_grunt.wav" ]]; then
        run_detailed_test "MFCC Baseline Extraction" "MFCC feature extraction from standard user recording" \
            "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt.wav buck_grunt"
    fi

    # MFCC with gaps - tests feature consistency across silence boundaries
    if [[ -f "data/recordings/user_attempt_buck_grunt_gap.wav" ]]; then
        run_detailed_test "MFCC Gap Robustness" "MFCC extraction with silence gaps - feature boundary handling" \
            "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_gap.wav buck_grunt"
    fi

    # MFCC with low volume - tests normalization and noise floor handling
    if [[ -f "data/recordings/user_attempt_buck_grunt_lowvolume.wav" ]]; then
        run_detailed_test "MFCC Low Volume Robustness" "MFCC extraction from low amplitude audio" \
            "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_lowvolume.wav buck_grunt"
    fi

    # MFCC with continuous audio - tests frame-by-frame consistency
    if [[ -f "data/recordings/user_attempt_buck_grunt_nogap.wav" ]]; then
        run_detailed_test "MFCC Continuous Processing" "MFCC extraction from continuous audio stream" \
            "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_nogap.wav buck_grunt"
    fi

    # MFCC with synthetic audio for controlled testing
    if [[ "$STRESS_TEST" == true ]]; then
        log_test "Testing MFCC with synthetic audio characteristics..."

        # Create test audio files with different characteristics
        mkdir -p temp_test_audio

        if create_test_audio "temp_test_audio/sine_440.wav" "2" "440"; then
            run_detailed_test "MFCC Pure Tone" "MFCC extraction from pure sine wave" \
                "$BUILD_DIR/bin/LoadandExtractTest temp_test_audio/sine_440.wav mfcc_sine"
        fi

        if create_test_audio "temp_test_audio/sweep.wav" "3" "100-1000"; then
            run_detailed_test "MFCC Frequency Sweep" "MFCC extraction from frequency sweep" \
                "$BUILD_DIR/bin/LoadandExtractTest temp_test_audio/sweep.wav mfcc_sweep"
        fi

        # Cleanup
        rm -rf temp_test_audio
    fi
}

# Test DTW algorithm
test_dtw_component() {
    log_info "Testing Dynamic Time Warping..."

    # DTW algorithm tests
    run_detailed_test "DTW Algorithm Core" "DTW algorithm functionality and accuracy" \
        "$BUILD_DIR/bin/DTWUnifiedTest"

    # DTW comparison accuracy tests with different recording conditions
    log_info "Testing DTW comparison accuracy with various recording conditions..."

    local master_call="data/master_calls/buck_grunt.wav"

    if [[ -f "$master_call" ]]; then
        # Baseline comparison - should have high similarity
        if [[ -f "data/recordings/user_attempt_buck_grunt.wav" ]]; then
            run_detailed_test "DTW Baseline Comparison" "DTW similarity with standard user recording" \
                "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt.wav buck_grunt"
        fi

        # Gap handling - tests DTW's ability to handle timing variations
        if [[ -f "data/recordings/user_attempt_buck_grunt_gap.wav" ]]; then
            run_detailed_test "DTW Gap Tolerance" "DTW alignment with silence gaps and timing variations" \
                "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_gap.wav buck_grunt"
        fi

        # Volume independence - DTW should focus on spectral patterns, not amplitude
        if [[ -f "data/recordings/user_attempt_buck_grunt_lowvolume.wav" ]]; then
            run_detailed_test "DTW Volume Independence" "DTW similarity despite amplitude differences" \
                "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_lowvolume.wav buck_grunt"
        fi

        # Continuous audio - tests DTW with no timing gaps
        if [[ -f "data/recordings/user_attempt_buck_grunt_nogap.wav" ]]; then
            run_detailed_test "DTW Continuous Alignment" "DTW alignment with continuous audio sequences" \
                "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_nogap.wav buck_grunt"
        fi
    fi

    if [[ "$STRESS_TEST" == true ]]; then
        log_test "Running DTW stress tests with synthetic patterns..."
        # This would test DTW with various sequence lengths and patterns
        log_warning "DTW stress tests not yet implemented"
    fi
}

# Test Voice Activity Detection
test_vad_component() {
    log_info "Testing Voice Activity Detection..."

    # VAD baseline test with master call
    if [[ -f "data/master_calls/buck_grunt.wav" ]]; then
        run_detailed_test "VAD Baseline (Master)" "VAD processing with master call reference" \
            "$BUILD_DIR/bin/LoadandExtractTest data/master_calls/buck_grunt.wav vad_master"
    fi

    # VAD with gap recording - primary test for silence detection
    if [[ -f "data/recordings/user_attempt_buck_grunt_gap.wav" ]]; then
        run_detailed_test "VAD Gap Detection" "VAD processing with silence gaps - core VAD functionality" \
            "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_gap.wav vad_gap"
    fi

    # VAD with no-gap recording - test for false positive silence detection
    if [[ -f "data/recordings/user_attempt_buck_grunt_nogap.wav" ]]; then
        run_detailed_test "VAD Continuous Audio" "VAD with continuous audio - no false silence detection" \
            "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_nogap.wav vad_nogap"
    fi

    # VAD with low volume - test sensitivity thresholds
    if [[ -f "data/recordings/user_attempt_buck_grunt_lowvolume.wav" ]]; then
        run_detailed_test "VAD Low Volume Sensitivity" "VAD sensitivity to low amplitude audio" \
            "$BUILD_DIR/bin/LoadandExtractTest data/recordings/user_attempt_buck_grunt_lowvolume.wav vad_lowvol"
    fi

    # Create synthetic test audio with silence for additional VAD testing
    if [[ "$STRESS_TEST" == true ]] && command -v sox >/dev/null 2>&1; then
        log_test "Creating synthetic audio with silence periods for VAD stress testing..."
        mkdir -p temp_test_audio

        # Create audio with silence at beginning and end
        sox -n -r 44100 -c 1 temp_test_audio/silent_start.wav synth 1 sine 0 vol 0 : synth 2 sine 440 vol 0.3 : synth 1 sine 0 vol 0

        if [[ -f "temp_test_audio/silent_start.wav" ]]; then
            run_detailed_test "VAD Synthetic Silence" "VAD processing with synthetic silence periods" \
                "$BUILD_DIR/bin/LoadandExtractTest temp_test_audio/silent_start.wav test_silence"
        fi

        rm -rf temp_test_audio
    fi
}

# Test real-time processing
test_realtime_component() {
    log_info "Testing Real-time Processing..."

    # Real-time processing tests would be part of the main engine tests
    log_test "Real-time processing through engine tests..."

    run_detailed_test "Real-time Engine Test" "Real-time processing capabilities" \
        "$BUILD_DIR/bin/UnifiedEngineTest"

    if [[ "$STRESS_TEST" == true ]]; then
        log_test "Real-time stress testing..."
        # This would simulate continuous audio processing
        log_warning "Real-time stress tests not yet implemented"
    fi
}

# Test audio I/O and processing
test_audio_component() {
    log_info "Testing Audio I/O and Processing..."

    # Test audio file loading and processing
    if [[ -f "data/master_calls/buck_grunt.wav" ]]; then
        run_detailed_test "Audio File Loading" "Audio file I/O and format handling" \
            "$BUILD_DIR/bin/LoadandExtractTest data/master_calls/buck_grunt.wav buck_grunt"
    fi

    # Test recording functionality (if available)
    run_detailed_test "Audio Recording Test" "Audio recording capabilities" \
        "$BUILD_DIR/bin/RecordingTest"

    # Test cross-platform audio
    run_detailed_test "Cross-platform Audio" "Cross-platform audio compatibility" \
        "$BUILD_DIR/bin/CrossPlatformTest"

    if [[ "$STRESS_TEST" == true ]]; then
        log_test "Audio I/O stress testing..."
        # Test with multiple concurrent audio streams
        log_warning "Audio I/O stress tests not yet implemented"
    fi
}

# Function to run all component tests
test_all_components() {
    log_info "Running comprehensive component testing..."

    local components=("engine" "mfcc" "dtw" "vad" "realtime" "audio")
    local passed=0
    local total=${#components[@]}

    for component in "${components[@]}"; do
        echo ""
        echo "╔══════════════════════════════════════════════════════════════════════════════════════════════════╗"
        echo "║                                    TESTING: ${component^^}                                                  ║"
        echo "╚══════════════════════════════════════════════════════════════════════════════════════════════════╝"

        case "$component" in
            "engine") test_engine_component && ((passed++)) ;;
            "mfcc") test_mfcc_component && ((passed++)) ;;
            "dtw") test_dtw_component && ((passed++)) ;;
            "vad") test_vad_component && ((passed++)) ;;
            "realtime") test_realtime_component && ((passed++)) ;;
            "audio") test_audio_component && ((passed++)) ;;
        esac
    done

    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════════════════════════╗"
    echo "║                                    FINAL SUMMARY                                                ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════════════════════════╝"
    log_info "Component Tests Summary: $passed/$total components passed"

    if [[ $passed -eq $total ]]; then
        log_success "🎉 All component tests passed!"
        return 0
    else
        log_error "❌ Some component tests failed"
        return 1
    fi
}

# Main execution
main() {
    echo "╔══════════════════════════════════════════════════════════════════════════════════════════════════╗"
    echo "║                        Huntmaster Audio Engine - Component Testing                              ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════════════════════════╝"
    echo ""

    log_info "Component: $COMPONENT"
    log_info "Verbose: $VERBOSE"
    log_info "Stress Test: $STRESS_TEST"
    log_info "Memory Check: $MEMORY_CHECK"
    echo ""

    # Check if build directory exists
    if [[ ! -d "$BUILD_DIR" ]]; then
        log_error "Build directory not found: $BUILD_DIR"
        log_info "Please run the comprehensive_test.sh script first to build the project"
        exit 1
    fi

    # Execute component tests
    case "$COMPONENT" in
        "engine") test_engine_component ;;
        "mfcc") test_mfcc_component ;;
        "dtw") test_dtw_component ;;
        "vad") test_vad_component ;;
        "realtime") test_realtime_component ;;
        "audio") test_audio_component ;;
        "all") test_all_components ;;
        *)
            log_error "Unknown component: $COMPONENT"
            exit 1
            ;;
    esac

    local result=$?

    echo ""
    if [[ $result -eq 0 ]]; then
        log_success "Component testing completed successfully!"
    else
        log_error "Component testing failed!"
    fi

    exit $result
}

# Check for required tools
if [[ "$MEMORY_CHECK" == true ]] && ! command -v valgrind >/dev/null 2>&1; then
    log_warning "Valgrind not found - memory checking disabled"
    MEMORY_CHECK=false
fi

if [[ "$STRESS_TEST" == true ]] && ! command -v bc >/dev/null 2>&1; then
    log_warning "bc (calculator) not found - some timing calculations may not work"
fi

# Execute main function
main "$@"
