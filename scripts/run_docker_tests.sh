#!/bin/bash
# Docker Test Runner - Comprehensive testing orchestration
# Provides easy interface for running different test configurations

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log() { echo -e "${BLUE}[$(date '+%H:%M:%S')] $1${NC}"; }
warn() { echo -e "${YELLOW}[WARNING] $1${NC}"; }
error() { echo -e "${RED}[ERROR] $1${NC}"; }
success() { echo -e "${GREEN}[SUCCESS] $1${NC}"; }

# Default configuration
COVERAGE_TARGET=90
PARALLEL_JOBS=$(nproc)

show_usage() {
    cat << EOF
Huntmaster Engine Docker Test Runner
====================================

Usage: $0 [COMMAND] [OPTIONS]

Commands:
  coverage    Run comprehensive coverage analysis (default)
  unit        Run unit tests only
  integration Run integration tests only
  security    Run security tests only
  performance Run performance tests only
  memory      Run memory analysis with valgrind
  all         Run all test suites sequentially
  clean       Clean up containers and volumes
  viz         Start coverage visualization server

Options:
  --target=N     Set coverage target percentage (default: 90)
  --timeout=N    Set test timeout in seconds (default: 180)
  --parallel=N   Set parallel jobs (default: $(nproc))
  --rebuild      Force rebuild of containers
  --help         Show this help message

Examples:
  $0 coverage --target=95 --timeout=300
  $0 unit --timeout=60
  $0 all --rebuild
  $0 viz  # Then visit http://localhost:8080
EOF
}

# Parse command line arguments
COMMAND="coverage"
REBUILD=""
TIMEOUT="180"

while [[ $# -gt 0 ]]; do
    case $1 in
        coverage|unit|integration|security|performance|memory|all|clean|viz)
            COMMAND="$1"
            shift
            ;;
        --target=*)
            COVERAGE_TARGET="${1#*=}"
            shift
            ;;
        --timeout=*)
            TIMEOUT="${1#*=}"
            shift
            ;;
        --parallel=*)
            PARALLEL_JOBS="${1#*=}"
            shift
            ;;
        --rebuild)
            REBUILD="--build"
            shift
            ;;
        --help)
            show_usage
            exit 0
            ;;
        *)
            error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Validate environment
validate_environment() {
    if ! command -v docker &> /dev/null; then
        error "Docker not found. Please install Docker."
        exit 1
    fi
    
    if ! command -v docker-compose &> /dev/null; then
        error "Docker Compose not found. Please install Docker Compose."
        exit 1
    fi
    
    if [[ ! -f "docker-compose.test.yml" ]]; then
        error "docker-compose.test.yml not found. Run from project root."
        exit 1
    fi
}

# Clean up containers and volumes
cleanup() {
    log "Cleaning up Docker containers and volumes..."
    docker-compose -f docker-compose.test.yml down --volumes --remove-orphans 2>/dev/null || true
    docker system prune -f 2>/dev/null || true
    success "Cleanup completed"
}

# Run coverage analysis
run_coverage() {
    log "Running comprehensive coverage analysis (target: $COVERAGE_TARGET%)..."
    
    COVERAGE_TARGET="$COVERAGE_TARGET" \
    TEST_TIMEOUT="$TIMEOUT" \
    docker-compose -f docker-compose.test.yml \
        --profile coverage \
        run --rm $REBUILD \
        -e COVERAGE_TARGET="$COVERAGE_TARGET" \
        -e TEST_TIMEOUT="$TIMEOUT" \
        huntmaster-test
}

# Run unit tests
run_unit_tests() {
    log "Running unit tests..."
    
    TEST_TIMEOUT="$TIMEOUT" \
    docker-compose -f docker-compose.test.yml \
        --profile unit \
        run --rm $REBUILD \
        -e TEST_TIMEOUT="$TIMEOUT" \
        huntmaster-unit-test
}

# Run integration tests
run_integration_tests() {
    log "Running integration tests..."
    
    TEST_TIMEOUT="$TIMEOUT" \
    docker-compose -f docker-compose.test.yml \
        --profile integration \
        run --rm $REBUILD \
        -e TEST_TIMEOUT="$TIMEOUT" \
        huntmaster-integration-test
}

# Run security tests
run_security_tests() {
    log "Running security tests..."
    
    TEST_TIMEOUT="$TIMEOUT" \
    docker-compose -f docker-compose.test.yml \
        --profile security \
        run --rm $REBUILD \
        -e TEST_TIMEOUT="$TIMEOUT" \
        huntmaster-security-test
}

# Run performance tests
run_performance_tests() {
    log "Running performance tests..."
    
    TEST_TIMEOUT="$TIMEOUT" \
    docker-compose -f docker-compose.test.yml \
        --profile performance \
        run --rm $REBUILD \
        -e TEST_TIMEOUT="$TIMEOUT" \
        huntmaster-performance-test
}

# Run memory analysis
run_memory_analysis() {
    log "Running memory analysis with Valgrind..."
    
    TEST_TIMEOUT="$TIMEOUT" \
    docker-compose -f docker-compose.test.yml \
        --profile memory \
        run --rm $REBUILD \
        -e TEST_TIMEOUT="$TIMEOUT" \
        huntmaster-memory-test
}

# Run all test suites
run_all_tests() {
    log "Running all test suites sequentially..."
    
    local failed_suites=()
    
    run_unit_tests || failed_suites+=("unit")
    run_integration_tests || failed_suites+=("integration")
    run_security_tests || failed_suites+=("security")
    run_performance_tests || failed_suites+=("performance")
    run_memory_analysis || failed_suites+=("memory")
    run_coverage || failed_suites+=("coverage")
    
    if [[ ${#failed_suites[@]} -eq 0 ]]; then
        success "All test suites passed!"
    else
        error "Failed test suites: ${failed_suites[*]}"
        exit 1
    fi
}

# Start visualization server
start_visualization() {
    log "Starting coverage visualization server..."
    log "Coverage reports will be available at: http://localhost:8080"
    
    docker-compose -f docker-compose.test.yml \
        --profile viz \
        up $REBUILD huntmaster-coverage-viz
}

# Ensure coverage reports directory exists
ensure_directories() {
    mkdir -p coverage_reports test_logs
    
    # Set proper permissions for Docker
    chmod 755 coverage_reports test_logs
}

# Main execution
main() {
    log "Huntmaster Engine Docker Test Runner"
    log "Command: $COMMAND, Target: $COVERAGE_TARGET%, Timeout: ${TIMEOUT}s"
    
    validate_environment
    ensure_directories
    
    case "$COMMAND" in
        coverage)
            run_coverage
            ;;
        unit)
            run_unit_tests
            ;;
        integration)
            run_integration_tests
            ;;
        security)
            run_security_tests
            ;;
        performance)
            run_performance_tests
            ;;
        memory)
            run_memory_analysis
            ;;
        all)
            run_all_tests
            ;;
        clean)
            cleanup
            ;;
        viz)
            start_visualization
            ;;
        *)
            error "Unknown command: $COMMAND"
            show_usage
            exit 1
            ;;
    esac
}

# Execute main function
main "$@"
