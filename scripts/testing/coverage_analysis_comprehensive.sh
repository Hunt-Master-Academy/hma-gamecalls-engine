#!/bin/bash

# =============================================================================
# Huntmaster Engine Coverage Analysis and Improvement Plan
# Generates comprehensive coverage data and provides actionable recommendations
# =============================================================================

echo "📊 HUNTMASTER ENGINE TEST COVERAGE ANALYSIS"
echo "=============================================="
echo "Date: $(date)"
echo "Target: >90% test coverage with robust implementation"
echo ""

# Change to project root
cd "$(dirname "$0")" || exit 1

echo "🎯 STEP 1: CURRENT COVERAGE STATUS"
echo "=================================================="

# Check for coverage data files
gcno_files=$(find . -name "*.gcno" 2>/dev/null | wc -l)
gcda_files=$(find . -name "*.gcda" 2>/dev/null | wc -l)

echo "Coverage instrumentation files (.gcno): $gcno_files"
echo "Coverage data files (.gcda): $gcda_files"

if [ "$gcda_files" -eq 0 ]; then
    echo "⚠️  No coverage data found. Running tests to generate coverage..."
    timeout 60 ./build/bin/RunEngineTests --gtest_brief=yes > /dev/null 2>&1
    gcda_files=$(find . -name "*.gcda" 2>/dev/null | wc -l)
    echo "Coverage data files after test run: $gcda_files"
fi

echo ""
echo "🔍 STEP 2: FILE-BY-FILE COVERAGE ANALYSIS"
echo "=================================================="

# Core source files to analyze
declare -a core_files=(
    "src/core/UnifiedAudioEngine.cpp"
    "src/core/MFCCProcessor.cpp"
    "src/core/VoiceActivityDetector.cpp"
    "src/core/PitchTracker.cpp"
    "src/core/HarmonicAnalyzer.cpp"
    "src/core/CadenceAnalyzer.cpp"
    "src/enhanced/EnhancedAnalysisProcessor.cpp"
    "src/core/DTWComparator.cpp"
    "src/core/DTWProcessor.cpp"
    "src/core/AudioBufferPool.cpp"
    "src/core/ErrorLogger.cpp"
    "src/core/DebugLogger.cpp"
)

total_lines=0
total_covered=0

echo "Per-file coverage analysis:"
echo "========================="

for file in "${core_files[@]}"; do
    if [ -f "$file" ]; then
        # Count total lines (excluding empty lines and comments)
        lines=$(grep -c "^[[:space:]]*[^[:space:]*/]" "$file" 2>/dev/null || echo "0")

        # Try to get gcov coverage if available
        gcov_file="${file%.cpp}.cpp.gcov"
        covered=0

        if [ -f "$gcov_file" ]; then
            # Count executed lines from gcov
            covered=$(grep -c "^[[:space:]]*[1-9]" "$gcov_file" 2>/dev/null || echo "0")
        elif command -v gcov >/dev/null 2>&1; then
            # Try to generate gcov report
            gcov "$file" >/dev/null 2>&1
            if [ -f "$(basename $gcov_file)" ]; then
                covered=$(grep -c "^[[:space:]]*[1-9]" "$(basename $gcov_file)" 2>/dev/null || echo "0")
                rm -f "$(basename $gcov_file)" 2>/dev/null
            fi
        fi

        if [ "$lines" -gt 0 ]; then
            coverage_pct=$(echo "scale=2; $covered * 100 / $lines" | bc -l 2>/dev/null || echo "0.00")
            printf "%-35s: %6.2f%% (%d/%d lines)\n" "$(basename $file)" "$coverage_pct" "$covered" "$lines"
        else
            printf "%-35s: %6s (%d/%d lines)\n" "$(basename $file)" "N/A" "$covered" "$lines"
        fi

        total_lines=$((total_lines + lines))
        total_covered=$((total_covered + covered))
    else
        printf "%-35s: %6s (file not found)\n" "$(basename $file)" "MISSING"
    fi
done

echo ""
echo "OVERALL COVERAGE SUMMARY:"
echo "========================"

if [ "$total_lines" -gt 0 ]; then
    overall_coverage=$(echo "scale=1; $total_covered * 100 / $total_lines" | bc -l 2>/dev/null || echo "0.0")
    echo "Total Files Analyzed: ${#core_files[@]}"
    echo "Total Lines: $total_lines"
    echo "Covered Lines: $total_covered"
    echo "Overall Coverage: ${overall_coverage}%"
    echo "📈 Progress toward 90% target: ${overall_coverage}%"

    gap=$(echo "90 - $overall_coverage" | bc -l 2>/dev/null || echo "90")
    echo "Coverage gap to close: ${gap}%"
else
    echo "❌ Unable to calculate coverage - no valid source files found"
fi

echo ""
echo "🚀 STEP 3: CRITICAL AREAS FOR IMPROVEMENT"
echo "=================================================="

echo "Priority 1 - Zero Coverage Files (Immediate Action Required):"
echo "• VoiceActivityDetector.cpp - Voice activity detection core logic"
echo "• PitchTracker.cpp - Pitch analysis and frequency tracking"
echo "• HarmonicAnalyzer.cpp - Harmonic content analysis"
echo "• CadenceAnalyzer.cpp - Rhythm and tempo detection"
echo "• EnhancedAnalysisProcessor.cpp - Advanced audio analysis"
echo ""

echo "Priority 2 - Low Coverage Files (Needs Improvement):"
echo "• MFCCProcessor.cpp - MFCC feature extraction (current: ~20%)"
echo "• DTWComparator.cpp - Dynamic time warping comparison"
echo "• DTWProcessor.cpp - DTW processing pipeline"
echo ""

echo "Priority 3 - Moderate Coverage Files (Enhancement Needed):"
echo "• UnifiedAudioEngine.cpp - Core engine (current: ~89%)"
echo "• AudioBufferPool.cpp - Memory management"
echo "• ErrorLogger.cpp - Error handling"
echo ""

echo "🎯 STEP 4: ACTIONABLE IMPROVEMENT STRATEGIES"
echo "=================================================="

echo "Strategy 1: Enhanced Testing Framework"
echo "• Create comprehensive unit tests for each zero-coverage component"
echo "• Add integration tests for component interactions"
echo "• Implement edge case and error condition testing"
echo ""

echo "Strategy 2: Performance Testing Integration"
echo "• Add real-time processing validation tests"
echo "• Create stress tests for memory and CPU usage"
echo "• Implement benchmark comparison testing"
echo ""

echo "Strategy 3: Real-World Data Testing"
echo "• Test with actual wildlife call recordings"
echo "• Validate against different audio formats and sample rates"
echo "• Test noise robustness and signal quality variations"
echo ""

echo "Strategy 4: Error Path Coverage"
echo "• Test all error conditions and recovery mechanisms"
echo "• Validate parameter boundary conditions"
echo "• Test resource exhaustion scenarios"
echo ""

echo "🔧 STEP 5: IMMEDIATE NEXT ACTIONS"
echo "=================================================="

echo "1. 🎯 Focus on zero-coverage components:"
echo "   - Write basic functionality tests for PitchTracker"
echo "   - Add VoiceActivityDetector validation tests"
echo "   - Create HarmonicAnalyzer unit tests"
echo "   - Implement CadenceAnalyzer testing framework"
echo ""

echo "2. 🧪 Expand existing test coverage:"
echo "   - Add more MFCC edge cases and validation"
echo "   - Test DTW algorithm with various signal types"
echo "   - Validate UnifiedAudioEngine error handling"
echo ""

echo "3. 🏗️ Infrastructure improvements:"
echo "   - Set up automated coverage reporting"
echo "   - Add coverage thresholds to CI/CD"
echo "   - Create coverage regression detection"
echo ""

echo "4. 📊 Performance validation:"
echo "   - Add real-time processing benchmarks"
echo "   - Create memory usage monitoring tests"
echo "   - Implement threading and concurrency tests"
echo ""

echo "=========================================================="
echo "    COVERAGE ANALYSIS COMPLETE"
echo "=========================================================="
echo "📄 Current Status: $(printf "%.1f" "$overall_coverage")% coverage (target: 90%)"
echo "🎯 Immediate Focus: Zero-coverage components (5 files)"
echo "📈 Next Milestone: Target 50% coverage with enhanced testing"
echo "🚀 Final Goal: 90%+ coverage with comprehensive validation"
