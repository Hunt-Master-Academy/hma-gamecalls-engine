#!/bin/bash

# ==============================================================================
# Pre-WASM Validation Script for Huntmaster Audio Engine
# ==============================================================================
# This script performs final validation to ensure the engine is ready for
# WASM compilation and deployment.
#
# Usage: ./pre_wasm_validation.sh [--fix-issues] [--verbose]
# ==============================================================================

set -e

# Configuration
BUILD_DIR="build"
WASM_BUILD_DIR="build-wasm"
FIX_ISSUES=false
VERBOSE=false

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m'

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --fix-issues)
            FIX_ISSUES=true
            shift
            ;;
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [--fix-issues] [--verbose]"
            echo ""
            echo "Options:"
            echo "  --fix-issues     Attempt to automatically fix detected issues"
            echo "  --verbose, -v    Enable verbose output"
            echo "  --help, -h       Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Logging functions
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[✓]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[⚠]${NC} $1"; }
log_error() { echo -e "${RED}[✗]${NC} $1"; }
log_check() { echo -e "${PURPLE}[CHECK]${NC} $1"; }

# Validation tracking
ISSUES_FOUND=0
WARNINGS_FOUND=0

# Function to report issue
report_issue() {
    local severity="$1"
    local message="$2"
    local fix_command="$3"

    case "$severity" in
        "error")
            log_error "$message"
            ISSUES_FOUND=$((ISSUES_FOUND + 1))
            ;;
        "warning")
            log_warning "$message"
            WARNINGS_FOUND=$((WARNINGS_FOUND + 1))
            ;;
    esac

    if [[ -n "$fix_command" ]] && [[ "$FIX_ISSUES" == true ]]; then
        log_info "Attempting to fix: $fix_command"
        eval "$fix_command"
    fi
}

# Check build system and dependencies
check_build_system() {
    log_check "Validating build system..."

    # Check CMake
    if ! command -v cmake >/dev/null 2>&1; then
        report_issue "error" "CMake not found - required for WASM build"
    else
        local cmake_version=$(cmake --version | head -n1 | cut -d' ' -f3)
        log_success "CMake found: $cmake_version"
    fi

    # Check Emscripten
    if ! command -v emcc >/dev/null 2>&1; then
        report_issue "warning" "Emscripten not found - required for WASM compilation" \
            "# Install Emscripten: git clone https://github.com/emscripten-core/emsdk.git && cd emsdk && ./emsdk install latest && ./emsdk activate latest"
    else
        local emcc_version=$(emcc --version | head -n1)
        log_success "Emscripten found: $emcc_version"
    fi

    # Check native build
    if [[ ! -d "$BUILD_DIR" ]]; then
        report_issue "error" "Native build directory not found" \
            "mkdir -p $BUILD_DIR && cd $BUILD_DIR && cmake .. && make -j4"
    else
        log_success "Native build directory exists"
    fi
}

# Check source code for WASM compatibility
check_wasm_compatibility() {
    log_check "Checking WASM compatibility..."

    # Check for platform-specific code that might not work in WASM
    local platform_specific_includes=(
        "#include <windows.h>"
        "#include <unistd.h>"
        "#include <sys/mman.h>"
        "#include <pthread.h>"
    )

    for include in "${platform_specific_includes[@]}"; do
        if grep -r "$include" src/ include/ 2>/dev/null | grep -v "// WASM_COMPATIBLE" >/dev/null; then
            report_issue "warning" "Platform-specific include found: $include (check WASM compatibility)"
        fi
    done

    # Check for file system operations
    local fs_operations=(
        "std::filesystem::"
        "fopen("
        "FILE\*"
    )

    for op in "${fs_operations[@]}"; do
        if grep -r "$op" src/ include/ 2>/dev/null | grep -v "// WASM_COMPATIBLE" >/dev/null; then
            report_issue "warning" "File system operation found: $op (may need WASM adaptation)"
        fi
    done

    # Check for thread usage
    if grep -r "std::thread" src/ include/ 2>/dev/null | grep -v "// WASM_COMPATIBLE" >/dev/null; then
        report_issue "warning" "std::thread usage found (WASM has limited threading support)"
    fi

    log_success "WASM compatibility check completed"
}

# Check audio dependencies
check_audio_dependencies() {
    log_check "Validating audio dependencies..."

    # Check for WASM-compatible audio libraries
    local audio_files=(
        "libs/dr_wav.h"
        "libs/miniaudio.h"
    )

    for file in "${audio_files[@]}"; do
        if [[ -f "$file" ]]; then
            log_success "Audio library found: $file"
        else
            report_issue "warning" "Audio library not found: $file"
        fi
    done

    # Check for problematic audio dependencies
    if grep -r "alsa" src/ include/ 2>/dev/null >/dev/null; then
        report_issue "warning" "ALSA dependency found (not available in WASM)"
    fi

    if grep -r "pulse" src/ include/ 2>/dev/null >/dev/null; then
        report_issue "warning" "PulseAudio dependency found (not available in WASM)"
    fi
}

# Check memory usage patterns
check_memory_patterns() {
    log_check "Analyzing memory usage patterns..."

    # Check for large static allocations
    if grep -r "static.*\[.*[0-9][0-9][0-9][0-9].*\]" src/ include/ 2>/dev/null >/dev/null; then
        report_issue "warning" "Large static arrays found (consider dynamic allocation for WASM)"
    fi

    # Check for malloc/free usage
    if grep -r "malloc\|free" src/ include/ 2>/dev/null | grep -v "// WASM_COMPATIBLE" >/dev/null; then
        report_issue "warning" "Direct malloc/free usage found (prefer smart pointers for WASM)"
    fi

    # Check for memory-mapped files
    if grep -r "mmap" src/ include/ 2>/dev/null >/dev/null; then
        report_issue "error" "Memory-mapped files found (not supported in WASM)"
    fi

    log_success "Memory pattern analysis completed"
}

# Test core functionality
test_core_functionality() {
    log_check "Testing core functionality before WASM build..."

    # Run essential tests
    local core_tests=(
        "UnifiedEngineTest:Engine core functionality"
        "MFCCConsistencyUnifiedTest:MFCC processing"
        "DTWUnifiedTest:DTW algorithm"
    )

    local passed=0
    local total=${#core_tests[@]}

    for test_info in "${core_tests[@]}"; do
        IFS=':' read -r test_name description <<< "$test_info"

        if [[ -f "$BUILD_DIR/bin/$test_name" ]]; then
            log_info "Running $description..."

            if [[ "$VERBOSE" == true ]]; then
                "$BUILD_DIR/bin/$test_name"
            else
                "$BUILD_DIR/bin/$test_name" >/dev/null 2>&1
            fi

            if [[ $? -eq 0 ]]; then
                log_success "$description passed"
                passed=$((passed + 1))
            else
                report_issue "error" "$description failed"
            fi
        else
            report_issue "error" "Test executable not found: $test_name"
        fi
    done

    if [[ $passed -eq $total ]]; then
        log_success "All core functionality tests passed"
    else
        report_issue "error" "Core functionality tests failed ($passed/$total passed)"
    fi
}

# Check WASM build configuration
check_wasm_build_config() {
    log_check "Validating WASM build configuration..."

    # Check for WASM-specific CMake files
    if [[ -f "CMakeLists.txt" ]]; then
        if grep -q "EMSCRIPTEN" CMakeLists.txt; then
            log_success "WASM/Emscripten configuration found in CMakeLists.txt"
        else
            report_issue "warning" "No WASM/Emscripten configuration found in CMakeLists.txt"
        fi
    fi

    # Check for WASM build script
    if [[ -f "scripts/build_wasm.sh" ]]; then
        log_success "WASM build script found"
    else
        report_issue "warning" "WASM build script not found at scripts/build_wasm.sh"
    fi

    # Check WASM bindings
    if [[ -d "bindings/wasm" ]]; then
        log_success "WASM bindings directory found"

        local wasm_files=(
            "bindings/wasm/WASMInterface.h"
            "bindings/wasm/WASMInterface.cpp"
        )

        for file in "${wasm_files[@]}"; do
            if [[ -f "$file" ]]; then
                log_success "WASM binding file found: $file"
            else
                report_issue "warning" "WASM binding file not found: $file"
            fi
        done
    else
        report_issue "warning" "WASM bindings directory not found"
    fi
}

# Check web assets and deployment files
check_web_assets() {
    log_check "Validating web assets and deployment files..."

    if [[ -d "web" ]]; then
        log_success "Web directory found"

        local web_files=(
            "web/index.html"
            "web/test.html"
            "web/js/huntmaster.js"
        )

        for file in "${web_files[@]}"; do
            if [[ -f "$file" ]]; then
                log_success "Web asset found: $file"
            else
                report_issue "warning" "Web asset not found: $file"
            fi
        done
    else
        report_issue "warning" "Web directory not found"
    fi
}

# Performance and size analysis
analyze_performance() {
    log_check "Analyzing performance characteristics..."

    # Check binary sizes
    if [[ -f "$BUILD_DIR/lib/libUnifiedAudioEngine.a" ]]; then
        local lib_size=$(du -h "$BUILD_DIR/lib/libUnifiedAudioEngine.a" | cut -f1)
        log_info "Native library size: $lib_size"

        # Warn if library is very large
        local size_bytes=$(du -b "$BUILD_DIR/lib/libUnifiedAudioEngine.a" | cut -f1)
        if [[ $size_bytes -gt 10485760 ]]; then  # 10MB
            report_issue "warning" "Library size is large ($lib_size) - may impact WASM load time"
        fi
    fi

    # Run performance test if available
    if [[ -f "$BUILD_DIR/bin/PerformanceTest" ]]; then
        log_info "Running performance analysis..."

        if [[ "$VERBOSE" == true ]]; then
            "$BUILD_DIR/bin/PerformanceTest"
        else
            "$BUILD_DIR/bin/PerformanceTest" >/dev/null 2>&1
        fi

        if [[ $? -eq 0 ]]; then
            log_success "Performance test completed"
        else
            report_issue "warning" "Performance test failed or had issues"
        fi
    fi
}

# Generate pre-WASM report
generate_report() {
    local report_file="pre_wasm_validation_$(date +%Y%m%d_%H%M%S).md"

    log_info "Generating pre-WASM validation report: $report_file"

    {
        echo "# Pre-WASM Validation Report"
        echo ""
        echo "**Generated:** $(date)"
        echo "**Engine Version:** Huntmaster Audio Engine v4.1"
        echo ""
        echo "## Summary"
        echo ""
        echo "- **Issues Found:** $ISSUES_FOUND"
        echo "- **Warnings:** $WARNINGS_FOUND"
        echo ""

        if [[ $ISSUES_FOUND -eq 0 ]]; then
            echo "✅ **Status:** Ready for WASM compilation"
        else
            echo "❌ **Status:** Issues must be resolved before WASM compilation"
        fi

        echo ""
        echo "## Validation Checklist"
        echo ""
        echo "- [x] Build system validation"
        echo "- [x] WASM compatibility check"
        echo "- [x] Audio dependencies verification"
        echo "- [x] Memory patterns analysis"
        echo "- [x] Core functionality testing"
        echo "- [x] WASM build configuration"
        echo "- [x] Web assets verification"
        echo "- [x] Performance analysis"
        echo ""
        echo "## Next Steps"
        echo ""

        if [[ $ISSUES_FOUND -eq 0 ]]; then
            echo "1. Proceed with WASM compilation using \`scripts/build_wasm.sh\`"
            echo "2. Test WASM bindings with web interface"
            echo "3. Perform end-to-end testing in browser environment"
            echo "4. Optimize for web deployment"
        else
            echo "1. Review and fix the $ISSUES_FOUND issues identified above"
            echo "2. Re-run validation with \`--fix-issues\` flag if applicable"
            echo "3. Ensure all core tests pass"
            echo "4. Re-run this validation script"
        fi

        echo ""
        echo "## Recommendations"
        echo ""
        echo "- Monitor WASM binary size and loading performance"
        echo "- Test audio processing in various browser environments"
        echo "- Implement proper error handling for web context"
        echo "- Consider Progressive Web App (PWA) features"
        echo ""
    } > "$report_file"

    log_success "Report saved to: $report_file"
}

# Main execution
main() {
    echo "╔══════════════════════════════════════════════════════════════════════════════════════════════════╗"
    echo "║                          Pre-WASM Validation - Huntmaster Audio Engine                         ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════════════════════════╝"
    echo ""

    log_info "Starting pre-WASM validation..."
    log_info "Fix issues automatically: $FIX_ISSUES"
    log_info "Verbose output: $VERBOSE"
    echo ""

    # Run all validation checks
    check_build_system
    check_wasm_compatibility
    check_audio_dependencies
    check_memory_patterns
    test_core_functionality
    check_wasm_build_config
    check_web_assets
    analyze_performance

    # Generate report
    generate_report

    # Final summary
    echo ""
    echo "╔══════════════════════════════════════════════════════════════════════════════════════════════════╗"
    echo "║                                    VALIDATION SUMMARY                                           ║"
    echo "╚══════════════════════════════════════════════════════════════════════════════════════════════════╝"

    if [[ $ISSUES_FOUND -eq 0 ]]; then
        log_success "🎉 Validation PASSED! Engine is ready for WASM compilation."
        echo ""
        echo "Next steps:"
        echo "  1. Run: ./scripts/build_wasm.sh"
        echo "  2. Test WASM build in browser"
        echo "  3. Deploy to web environment"

        if [[ $WARNINGS_FOUND -gt 0 ]]; then
            echo ""
            log_warning "Note: $WARNINGS_FOUND warnings were found. Review them for optimization opportunities."
        fi

        exit 0
    else
        log_error "❌ Validation FAILED! $ISSUES_FOUND issues must be fixed."
        echo ""
        echo "To fix issues automatically, run:"
        echo "  $0 --fix-issues"
        echo ""
        echo "Or review the validation report for manual fixes."
        exit 1
    fi
}

# Execute main function
main "$@"
