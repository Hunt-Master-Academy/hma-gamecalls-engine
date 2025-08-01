#!/bin/bash

# Huntmaster Engine Development Container Manager
# For WSL Ubuntu 24.04 development

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Container names
CPP_CONTAINER="huntmaster-cpp-dev"
WASM_CONTAINER="huntmaster-wasm-builder"

# Functions
print_status() {
    echo -e "${GREEN}[STATUS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Check if running in WSL
check_wsl() {
    if ! grep -q Microsoft /proc/version; then
        print_warning "Not running in WSL. Some features may not work correctly."
    fi
}

# Setup audio for WSL
setup_audio() {
    print_status "Setting up audio for WSL..."
    export PULSE_SERVER=tcp:$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):4713
    echo "PULSE_SERVER=$PULSE_SERVER"
}

# Build development container
build_container() {
    print_status "Building development container..."
    docker-compose -f docker-compose.dev.yml build cpp-dev
}

# Start development environment
start_dev() {
    print_status "Starting development environment..."
    docker-compose -f docker-compose.dev.yml up -d cpp-dev
    
    # Wait for container to be ready
    sleep 2
    
    # Setup audio
    docker-compose -f docker-compose.dev.yml exec cpp-dev setup-audio
    
    print_status "Development container started!"
    print_status "To enter the container, run: ./dev-container.sh shell"
}

# Enter development shell
enter_shell() {
    print_status "Entering development shell..."
    docker-compose -f docker-compose.dev.yml exec \
        -e PULSE_SERVER=tcp:$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):4713 \
        cpp-dev /bin/bash
}

# Build the project
build_project() {
    local BUILD_TYPE=${1:-Debug}
    print_status "Building project in $BUILD_TYPE mode..."
    docker-compose -f docker-compose.dev.yml exec cpp-dev \
        bash -c "cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=$BUILD_TYPE && cmake --build build --parallel"
}

# Run tests
run_tests() {
    print_status "Running tests..."
    docker-compose -f docker-compose.dev.yml exec cpp-dev \
        bash -c "./build/bin/RunEngineTests"
}

# Build WebAssembly
build_wasm() {
    print_status "Building WebAssembly..."
    docker-compose -f docker-compose.dev.yml run --rm wasm-builder \
        bash -c "./scripts/build_wasm.sh"
}

# Stop all containers
stop_all() {
    print_status "Stopping all containers..."
    docker-compose -f docker-compose.dev.yml down
}

# Clean everything
clean_all() {
    print_status "Cleaning all containers and volumes..."
    docker-compose -f docker-compose.dev.yml down -v
    rm -rf build build-* .ccache
}

# Show logs
show_logs() {
    docker-compose -f docker-compose.dev.yml logs -f cpp-dev
}

# Main script logic
case "$1" in
    build)
        check_wsl
        build_container
        ;;
    start)
        check_wsl
        setup_audio
        start_dev
        ;;
    shell)
        enter_shell
        ;;
    compile)
        build_project "${2:-Debug}"
        ;;
    test)
        run_tests
        ;;
    wasm)
        build_wasm
        ;;
    stop)
        stop_all
        ;;
    clean)
        clean_all
        ;;
    logs)
        show_logs
        ;;
    *)
        echo "Usage: $0 {build|start|shell|compile|test|wasm|stop|clean|logs}"
        echo ""
        echo "Commands:"
        echo "  build    - Build the development container"
        echo "  start    - Start the development environment"
        echo "  shell    - Enter the development shell"
        echo "  compile  - Build the project (optional: Debug/Release)"
        echo "  test     - Run the test suite"
        echo "  wasm     - Build WebAssembly version"
        echo "  stop     - Stop all containers"
        echo "  clean    - Clean all containers and build artifacts"
        echo "  logs     - Show container logs"
        exit 1
        ;;
esac