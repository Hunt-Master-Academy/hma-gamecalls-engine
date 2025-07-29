#!/bin/bash

# Performance Profiling and Bottleneck Resolution Test Script
# This script builds and runs comprehensive performance analysis

set -e  # Exit on any error

echo "========================================================================="
echo "Huntmaster Engine Performance Profiling and Bottleneck Resolution"
echo "========================================================================="
echo ""

# Configuration
BUILD_DIR="build"
CMAKE_FLAGS="-DHUNTMASTER_BUILD_TOOLS=ON -DHUNTMASTER_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release"
PROFILING_DEMO="performance_profiling_demo"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."

    if ! command_exists cmake; then
        print_error "CMake not found. Please install CMake 3.16 or later."
        exit 1
    fi

    if ! command_exists make; then
        print_error "Make not found. Please install build tools."
        exit 1
    fi

    CMAKE_VERSION=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
    print_success "CMake version $CMAKE_VERSION found"

    # Check for compiler
    if ! command_exists g++ && ! command_exists clang++; then
        print_error "No C++ compiler found. Please install g++ or clang++."
        exit 1
    fi

    print_success "Prerequisites check passed"
}

# Build the project
build_project() {
    print_status "Building Huntmaster Engine with Performance Profiling tools..."

    # Create build directory
    if [ -d "$BUILD_DIR" ]; then
        print_status "Cleaning existing build directory..."
        rm -rf "$BUILD_DIR"
    fi

    mkdir -p "$BUILD_DIR"

    # Configure with CMake
    print_status "Configuring build with CMake..."
    cd "$BUILD_DIR"
    cmake .. $CMAKE_FLAGS

    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed"
        exit 1
    fi

    # Build the project
    print_status "Building project (this may take a few minutes)..."
    make -j$(nproc) 2>/dev/null || make -j4 2>/dev/null || make

    if [ $? -ne 0 ]; then
        print_error "Build failed"
        exit 1
    fi

    cd ..
    print_success "Build completed successfully"
}

# Check if profiling demo was built
check_profiling_demo() {
    DEMO_PATH="$BUILD_DIR/tools/$PROFILING_DEMO"

    if [ -f "$DEMO_PATH" ]; then
        print_success "Performance profiling demo found: $DEMO_PATH"
        return 0
    else
        print_error "Performance profiling demo not found at: $DEMO_PATH"
        print_status "Available tools in $BUILD_DIR/tools/:"
        ls -la "$BUILD_DIR/tools/" 2>/dev/null || print_warning "Tools directory not found"
        return 1
    fi
}

# Run performance profiling demo
run_profiling_demo() {
    print_status "Running performance profiling demonstration..."

    DEMO_PATH="$BUILD_DIR/tools/$PROFILING_DEMO"

    if [ ! -f "$DEMO_PATH" ]; then
        print_error "Performance profiling demo not found"
        return 1
    fi

    # Create output directory for results
    OUTPUT_DIR="performance_results_$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$OUTPUT_DIR"
    cd "$OUTPUT_DIR"

    print_status "Running comprehensive performance analysis..."
    print_status "Output will be saved to: $(pwd)"
    echo ""

    # Run the demo
    "../$DEMO_PATH" 2>&1 | tee performance_analysis.log

    if [ $? -eq 0 ]; then
        print_success "Performance profiling demo completed successfully"
        print_status "Results saved in: $(pwd)"

        # List generated files
        print_status "Generated files:"
        ls -la *.json *.log 2>/dev/null | while read line; do
            echo "  $line"
        done

    else
        print_error "Performance profiling demo failed"
        return 1
    fi

    cd ..
}

# Run basic performance tests if available
run_basic_tests() {
    print_status "Running basic performance tests..."

    PERF_TEST="$BUILD_DIR/tests/unit/test_performance"

    if [ -f "$PERF_TEST" ]; then
        print_status "Running test_performance..."
        "$PERF_TEST"

        if [ $? -eq 0 ]; then
            print_success "Basic performance tests passed"
        else
            print_warning "Some performance tests failed (this may be expected on slower systems)"
        fi
    else
        print_warning "Basic performance tests not found at: $PERF_TEST"
    fi
}

# Generate performance report summary
generate_summary() {
    print_status "Generating performance analysis summary..."

    LATEST_RESULTS=$(ls -td performance_results_* 2>/dev/null | head -1)

    if [ -n "$LATEST_RESULTS" ] && [ -d "$LATEST_RESULTS" ]; then
        echo ""
        echo "========================================================================="
        echo "PERFORMANCE ANALYSIS SUMMARY"
        echo "========================================================================="
        echo "Results Directory: $LATEST_RESULTS"
        echo ""

        # Check for key result files
        if [ -f "$LATEST_RESULTS/performance_report.json" ]; then
            print_success "✓ Session performance report generated"
        fi

        if [ -f "$LATEST_RESULTS/comprehensive_benchmark_results.json" ]; then
            print_success "✓ Comprehensive benchmark results generated"
        fi

        if [ -f "$LATEST_RESULTS/performance_analysis.log" ]; then
            print_success "✓ Detailed analysis log generated"

            # Extract key metrics from log if possible
            if grep -q "Real-time ratio:" "$LATEST_RESULTS/performance_analysis.log"; then
                echo ""
                print_status "Key Performance Metrics:"
                grep -E "(Real-time ratio|Average Processing Time|Peak Memory|Performance Category)" "$LATEST_RESULTS/performance_analysis.log" | head -10 | while read line; do
                    echo "  $line"
                done
            fi
        fi

        echo ""
        print_status "Next Steps:"
        echo "  1. Review detailed results in: $LATEST_RESULTS/"
        echo "  2. Import JSON files into analysis tools"
        echo "  3. Apply suggested optimizations"
        echo "  4. Re-run benchmarks to validate improvements"

    else
        print_warning "No performance results found"
    fi
}

# Display usage information
show_usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --build-only       Only build the project, don't run tests"
    echo "  --test-only        Only run tests (assumes already built)"
    echo "  --skip-basic-tests Skip basic performance tests"
    echo "  --help             Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                 # Full build and test"
    echo "  $0 --build-only    # Just build"
    echo "  $0 --test-only     # Just run performance analysis"
}

# Main execution flow
main() {
    local build_only=false
    local test_only=false
    local skip_basic_tests=false

    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --build-only)
                build_only=true
                shift
                ;;
            --test-only)
                test_only=true
                shift
                ;;
            --skip-basic-tests)
                skip_basic_tests=true
                shift
                ;;
            --help)
                show_usage
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done

    # Execute based on options
    if [ "$test_only" = false ]; then
        check_prerequisites
        build_project
    fi

    if [ "$build_only" = false ]; then
        if check_profiling_demo; then
            run_profiling_demo

            if [ "$skip_basic_tests" = false ]; then
                run_basic_tests
            fi

            generate_summary
        else
            print_error "Cannot run performance analysis - demo not available"
            exit 1
        fi
    fi

    echo ""
    print_success "Performance profiling and bottleneck resolution analysis complete!"
}

# Trap to clean up on exit
cleanup() {
    if [ $? -ne 0 ]; then
        print_error "Script failed - check output above for details"
    fi
}
trap cleanup EXIT

# Run main function with all arguments
main "$@"
