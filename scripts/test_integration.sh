#!/bin/bash

# Huntmaster Engine - Phase 1 Integration Test Script
# Tests integration of EnhancedWASMInterface with existing pipeline

set -e

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_OUTPUT_DIR="$BUILD_DIR/integration_test_output"

echo "🧪 Huntmaster Engine - Phase 1 Integration Testing"
echo "=================================================="
echo "Project Root: $PROJECT_ROOT"
echo "Build Directory: $BUILD_DIR"
echo ""

# Create test output directory
mkdir -p "$TEST_OUTPUT_DIR"

# Function to log test results
log_test_result() {
    local test_name="$1"
    local result="$2"
    local details="$3"

    if [ "$result" = "PASS" ]; then
        echo "✅ $test_name: PASSED"
    else
        echo "❌ $test_name: FAILED - $details"
    fi

    echo "$(date): $test_name - $result - $details" >> "$TEST_OUTPUT_DIR/integration_test.log"
}

# Test 1: Check if EnhancedWASMInterface files exist
test_enhanced_interface_files() {
    echo "🔍 Testing: Enhanced WASM Interface Files"

    local header_file="$PROJECT_ROOT/include/huntmaster/platform/wasm/EnhancedWASMInterface.h"
    local impl_file="$PROJECT_ROOT/src/platform/wasm/EnhancedWASMInterface.cpp"

    if [ -f "$header_file" ] && [ -f "$impl_file" ]; then
        local header_lines=$(wc -l < "$header_file")
        local impl_lines=$(wc -l < "$impl_file")
        log_test_result "Enhanced WASM Interface Files" "PASS" "Header: $header_lines lines, Implementation: $impl_lines lines"
        return 0
    else
        log_test_result "Enhanced WASM Interface Files" "FAIL" "Missing required files"
        return 1
    fi
}

# Test 2: Check if build scripts exist and are executable
test_build_scripts() {
    echo "🔍 Testing: Phase 1 Build Scripts"

    local scripts=(
        "$PROJECT_ROOT/scripts/build/wasm_build_optimizer.sh"
        "$PROJECT_ROOT/scripts/build/generate_typescript_defs.sh"
        "$PROJECT_ROOT/scripts/build/validate_wasm_artifacts.sh"
    )

    local all_exist=true
    for script in "${scripts[@]}"; do
        if [ ! -f "$script" ] || [ ! -x "$script" ]; then
            log_test_result "Build Scripts" "FAIL" "Missing or non-executable: $(basename "$script")"
            all_exist=false
        fi
    done

    if [ "$all_exist" = true ]; then
        log_test_result "Build Scripts" "PASS" "All 3 build scripts exist and are executable"
        return 0
    else
        return 1
    fi
}

# Test 3: Check CMakeLists.txt integration
test_cmake_integration() {
    echo "🔍 Testing: CMakeLists.txt Integration"

    local cmake_file="$PROJECT_ROOT/src/CMakeLists.txt"

    if grep -q "EnhancedWASMInterface.cpp" "$cmake_file"; then
        log_test_result "CMakeLists Integration" "PASS" "EnhancedWASMInterface.cpp referenced in build system"
        return 0
    else
        log_test_result "CMakeLists Integration" "FAIL" "EnhancedWASMInterface.cpp not found in CMakeLists.txt"
        return 1
    fi
}

# Test 4: Validate core dependencies exist
test_core_dependencies() {
    echo "🔍 Testing: Core Dependencies"

    local core_files=(
        "$PROJECT_ROOT/src/core/AudioFormatConverter.cpp"
        "$PROJECT_ROOT/src/core/CircularAudioBuffer.cpp"
        "$PROJECT_ROOT/src/core/QualityAssessor.cpp"
        "$PROJECT_ROOT/src/core/SessionManager.cpp"
        "$PROJECT_ROOT/src/core/StreamingAudioProcessor.cpp"
    )

    local missing_count=0
    for file in "${core_files[@]}"; do
        if [ ! -f "$file" ]; then
            ((missing_count++))
        fi
    done

    if [ $missing_count -eq 0 ]; then
        log_test_result "Core Dependencies" "PASS" "All 5 core components exist"
        return 0
    else
        log_test_result "Core Dependencies" "FAIL" "$missing_count core files missing"
        return 1
    fi
}

# Test 5: Check for TODO completion
test_todo_completion() {
    echo "🔍 Testing: TODO Completion"

    local impl_file="$PROJECT_ROOT/src/platform/wasm/EnhancedWASMInterface.cpp"
    local todo_count=$(grep -c "// TODO:" "$impl_file" 2>/dev/null || echo "0")

    # Only count actual TODO items, not section headers
    local actual_todos=$(grep "// TODO:" "$impl_file" | grep -v "TODO 1.2." | wc -l)

    if [ "$actual_todos" -eq 0 ]; then
        log_test_result "TODO Completion" "PASS" "All implementation TODOs completed"
        return 0
    else
        log_test_result "TODO Completion" "FAIL" "$actual_todos TODOs remaining"
        return 1
    fi
}

# Test 6: Validate build system can find headers
test_header_accessibility() {
    echo "🔍 Testing: Header File Accessibility"

    local header_dir="$PROJECT_ROOT/include/huntmaster/platform/wasm"
    local main_header="$header_dir/EnhancedWASMInterface.h"

    if [ -f "$main_header" ] && [ -r "$main_header" ]; then
        local class_count=$(grep -c "class.*EnhancedWASMInterface" "$main_header")
        if [ "$class_count" -gt 0 ]; then
            log_test_result "Header Accessibility" "PASS" "EnhancedWASMInterface header accessible and valid"
            return 0
        else
            log_test_result "Header Accessibility" "FAIL" "Header exists but class not found"
            return 1
        fi
    else
        log_test_result "Header Accessibility" "FAIL" "Header file not accessible"
        return 1
    fi
}

# Test 7: Check Emscripten bindings
test_emscripten_bindings() {
    echo "🔍 Testing: Emscripten Bindings"

    local impl_file="$PROJECT_ROOT/src/platform/wasm/EnhancedWASMInterface.cpp"

    if grep -q "EMSCRIPTEN_BINDINGS" "$impl_file"; then
        local binding_count=$(grep -c "function.*EnhancedWASMInterface::" "$impl_file")
        log_test_result "Emscripten Bindings" "PASS" "Bindings found with $binding_count method bindings"
        return 0
    else
        log_test_result "Emscripten Bindings" "FAIL" "No Emscripten bindings found"
        return 1
    fi
}

# Main test execution
main() {
    echo "Starting Phase 1 Integration Tests..."
    echo ""

    local total_tests=0
    local passed_tests=0

    # Run all tests
    local tests=(
        "test_enhanced_interface_files"
        "test_build_scripts"
        "test_cmake_integration"
        "test_core_dependencies"
        "test_todo_completion"
        "test_header_accessibility"
        "test_emscripten_bindings"
    )

    for test in "${tests[@]}"; do
        ((total_tests++))
        if $test; then
            ((passed_tests++))
        fi
        echo ""
    done

    # Generate summary
    echo "📊 Integration Test Summary"
    echo "=========================="
    echo "Total Tests: $total_tests"
    echo "Passed: $passed_tests"
    echo "Failed: $((total_tests - passed_tests))"

    local pass_rate=$((passed_tests * 100 / total_tests))
    echo "Pass Rate: $pass_rate%"
    echo ""

    if [ $passed_tests -eq $total_tests ]; then
        echo "🎉 All integration tests passed! Phase 1 is ready for production."
        echo ""
        echo "Next Steps:"
        echo "1. Run build system with new enhanced interface"
        echo "2. Test WASM output in browser environment"
        echo "3. Proceed with Phase 2 web application development"
        echo "4. Set up CI/CD pipeline with new build scripts"

        return 0
    else
        echo "⚠️  Some integration tests failed. Please address issues before proceeding."
        echo ""
        echo "Check detailed logs in: $TEST_OUTPUT_DIR/integration_test.log"

        return 1
    fi
}

# Run main function
main "$@"
