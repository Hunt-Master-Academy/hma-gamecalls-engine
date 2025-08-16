#!/bin/bash

# =============================================================================
# Unified Environment Setup Script - Huntmaster Audio Engine
# =============================================================================
# Consolidated environment validation and setup tool combining functionality from:
# - validate_container_environment.sh
# - dev_environment_check.sh
# - wsl_audio_fix.sh
# =============================================================================

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$PROJECT_ROOT/environment_setup_$TIMESTAMP.log"

# Platform detection
PLATFORM=""
IN_CONTAINER=false
IN_WSL=false
HAS_AUDIO=false

# Validation flags
CHECK_COMPILERS=true
CHECK_CMAKE=true
CHECK_AUDIO=true
CHECK_TOOLS=true
AUTO_FIX=false
VERBOSE=false

print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}================================================${NC}"
}

print_section() {
    echo -e "\n${PURPLE}=== $1 ===${NC}"
}

print_status() {
    echo -e "${CYAN}[INFO]${NC} $1"
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

show_usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Environment Setup Modes:"
    echo "  --validate     Validate current environment (default)"
    echo "  --setup        Set up development environment"
    echo "  --fix-audio    Fix audio issues (WSL/Container specific)"
    echo ""
    echo "Platform Options:"
    echo "  --platform=X   Force platform detection (linux, wsl, docker, macos)"
    echo "  --auto-fix     Automatically fix detected issues"
    echo ""
    echo "Validation Options:"
    echo "  --skip-audio   Skip audio system validation"
    echo "  --skip-tools   Skip development tools validation"
    echo "  --verbose      Enable verbose output"
    echo "  --help         Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                          # Basic environment validation"
    echo "  $0 --setup --auto-fix       # Set up and fix environment"
    echo "  $0 --fix-audio --platform=wsl # Fix WSL audio issues"
}

parse_arguments() {
    local mode="validate"

    while [[ $# -gt 0 ]]; do
        case $1 in
            --validate)
                mode="validate"
                shift
                ;;
            --setup)
                mode="setup"
                shift
                ;;
            --fix-audio)
                mode="fix-audio"
                shift
                ;;
            --platform=*)
                PLATFORM="${1#*=}"
                shift
                ;;
            --auto-fix)
                AUTO_FIX=true
                shift
                ;;
            --skip-audio)
                CHECK_AUDIO=false
                shift
                ;;
            --skip-tools)
                CHECK_TOOLS=false
                shift
                ;;
            --verbose)
                VERBOSE=true
                shift
                ;;
            --help|-h)
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

    echo "$mode"
}

detect_platform() {
    if [[ -n "$PLATFORM" ]]; then
        print_status "Platform forced to: $PLATFORM"
        return
    fi

    print_section "DETECTING PLATFORM"

    # Check for container environment
    if [[ -f /.dockerenv ]] || [[ -n "${CONTAINER:-}" ]]; then
        IN_CONTAINER=true
        PLATFORM="docker"
        print_status "Running in Docker container"
    fi

    # Check for WSL
    if grep -qi microsoft /proc/version 2>/dev/null; then
        IN_WSL=true
        PLATFORM="wsl"
        print_status "Running in Windows Subsystem for Linux (WSL)"
    fi

    # Detect OS
    if [[ -z "$PLATFORM" ]]; then
        case "$(uname -s)" in
            Linux*)     PLATFORM="linux" ;;
            Darwin*)    PLATFORM="macos" ;;
            CYGWIN*)    PLATFORM="cygwin" ;;
            MINGW*)     PLATFORM="mingw" ;;
            *)          PLATFORM="unknown" ;;
        esac
    fi

    print_success "Platform detected: $PLATFORM"

    # Check for audio availability
    if command -v aplay >/dev/null 2>&1 || command -v pactl >/dev/null 2>&1 || [[ "$PLATFORM" == "macos" ]]; then
        HAS_AUDIO=true
    fi

    echo "Platform: $PLATFORM" | tee -a "$LOG_FILE"
    echo "Container: $IN_CONTAINER" | tee -a "$LOG_FILE"
    echo "WSL: $IN_WSL" | tee -a "$LOG_FILE"
    echo "Audio Available: $HAS_AUDIO" | tee -a "$LOG_FILE"
}

check_compilers() {
    if [[ "$CHECK_COMPILERS" != true ]]; then
        return
    fi

    print_section "CHECKING COMPILERS"

    local gcc_version=""
    local clang_version=""
    local errors=0

    # Check GCC
    if command -v gcc >/dev/null 2>&1; then
        gcc_version=$(gcc --version | head -n1)
        print_success "GCC found: $gcc_version"

        # Check version (need GCC 9+ for C++17)
        local gcc_major=$(gcc -dumpversion | cut -d. -f1)
        if [[ $gcc_major -lt 9 ]]; then
            print_warning "GCC version $gcc_major may be too old (need 9+)"
            ((errors++))
        fi
    else
        print_error "GCC not found"
        ((errors++))
    fi

    # Check Clang
    if command -v clang++ >/dev/null 2>&1; then
        clang_version=$(clang++ --version | head -n1)
        print_success "Clang++ found: $clang_version"
    else
        print_warning "Clang++ not found (optional)"
    fi

    # Auto-fix attempt
    if [[ $errors -gt 0 && "$AUTO_FIX" == true ]]; then
        print_status "Attempting to install compilers..."
        case "$PLATFORM" in
            "linux"|"wsl")
                if command -v apt-get >/dev/null 2>&1; then
                    sudo apt-get update && sudo apt-get install -y build-essential gcc-9 g++-9
                elif command -v yum >/dev/null 2>&1; then
                    sudo yum groupinstall -y "Development Tools"
                fi
                ;;
            "macos")
                if command -v brew >/dev/null 2>&1; then
                    brew install gcc
                else
                    print_warning "Please install Xcode Command Line Tools: xcode-select --install"
                fi
                ;;
        esac
    fi

    return $errors
}

check_cmake() {
    if [[ "$CHECK_CMAKE" != true ]]; then
        return 0
    fi

    print_section "CHECKING CMAKE"

    if command -v cmake >/dev/null 2>&1; then
        local cmake_version=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
        print_success "CMake found: $cmake_version"

        # Check version (need CMake 3.16+)
        local cmake_major=$(echo "$cmake_version" | cut -d. -f1)
        local cmake_minor=$(echo "$cmake_version" | cut -d. -f2)

        if [[ $cmake_major -lt 3 ]] || [[ $cmake_major -eq 3 && $cmake_minor -lt 16 ]]; then
            print_warning "CMake version $cmake_version may be too old (need 3.16+)"

            if [[ "$AUTO_FIX" == true ]]; then
                print_status "Attempting to upgrade CMake..."
                case "$PLATFORM" in
                    "linux"|"wsl")
                        # Install latest CMake from official source
                        wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
                        sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
                        sudo apt-get update && sudo apt-get install -y cmake
                        ;;
                esac
            fi
            return 1
        fi
    else
        print_error "CMake not found"

        if [[ "$AUTO_FIX" == true ]]; then
            print_status "Attempting to install CMake..."
            case "$PLATFORM" in
                "linux"|"wsl")
                    if command -v apt-get >/dev/null 2>&1; then
                        sudo apt-get update && sudo apt-get install -y cmake
                    elif command -v yum >/dev/null 2>&1; then
                        sudo yum install -y cmake
                    fi
                    ;;
                "macos")
                    if command -v brew >/dev/null 2>&1; then
                        brew install cmake
                    fi
                    ;;
            esac
        fi
        return 1
    fi

    return 0
}

check_audio_system() {
    if [[ "$CHECK_AUDIO" != true ]]; then
        return 0
    fi

    print_section "CHECKING AUDIO SYSTEM"

    local audio_errors=0

    case "$PLATFORM" in
        "linux")
            # Check ALSA
            if command -v aplay >/dev/null 2>&1; then
                print_success "ALSA tools found"

                # Test audio devices
                if aplay -l >/dev/null 2>&1; then
                    print_success "Audio devices detected"
                else
                    print_warning "No audio devices found"
                    ((audio_errors++))
                fi
            else
                print_warning "ALSA tools not found"
                ((audio_errors++))
            fi

            # Check PulseAudio
            if command -v pactl >/dev/null 2>&1; then
                print_success "PulseAudio found"

                if pactl info >/dev/null 2>&1; then
                    print_success "PulseAudio server running"
                else
                    print_warning "PulseAudio server not running"
                    ((audio_errors++))
                fi
            else
                print_warning "PulseAudio not found"
            fi
            ;;

        "wsl")
            print_status "WSL audio configuration check..."

            # Check for PulseAudio server configuration
            if [[ -f ~/.pulse/client.conf ]]; then
                if grep -q "default-server" ~/.pulse/client.conf; then
                    print_success "PulseAudio client configured for WSL"
                else
                    print_warning "PulseAudio client not configured for WSL"
                    ((audio_errors++))
                fi
            else
                print_warning "PulseAudio client configuration missing"
                ((audio_errors++))
            fi
            ;;

        "docker")
            print_warning "Audio in Docker requires host system setup"
            # Docker audio is complex and usually requires volume mounts
            ;;

        "macos")
            print_success "macOS has built-in audio support"
            ;;
    esac

    return $audio_errors
}

fix_wsl_audio() {
    if [[ "$PLATFORM" != "wsl" ]]; then
        print_warning "WSL audio fix only applies to WSL environment"
        return
    fi

    print_section "FIXING WSL AUDIO CONFIGURATION"

    # Install PulseAudio client
    print_status "Installing PulseAudio client..."
    sudo apt-get update && sudo apt-get install -y pulseaudio

    # Configure PulseAudio client for WSL
    print_status "Configuring PulseAudio client..."
    mkdir -p ~/.pulse

    # Get Windows host IP
    local host_ip=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}')

    cat > ~/.pulse/client.conf << EOF
# PulseAudio client configuration for WSL
default-server = tcp:$host_ip:4713
autospawn = no
EOF

    # Set environment variable
    echo "export PULSE_SERVER=tcp:$host_ip:4713" >> ~/.bashrc
    export PULSE_SERVER="tcp:$host_ip:4713"

    print_success "WSL audio configuration completed"
    print_status "Note: Ensure PulseAudio is running on Windows host with network access enabled"
}

check_development_tools() {
    if [[ "$CHECK_TOOLS" != true ]]; then
        return 0
    fi

    print_section "CHECKING DEVELOPMENT TOOLS"

    local tools_errors=0

    # Essential tools
    local -A essential_tools=(
        ["ninja"]="Build system"
        ["git"]="Version control"
        ["pkg-config"]="Package configuration"
    )

    for tool in "${!essential_tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            print_success "$tool found (${essential_tools[$tool]})"
        else
            print_error "$tool not found (${essential_tools[$tool]})"
            ((tools_errors++))
        fi
    done

    # Optional but recommended tools
    local -A optional_tools=(
        ["ccache"]="Compiler cache"
        ["clang-tidy"]="Static analysis"
        ["valgrind"]="Memory debugging"
        ["gdb"]="Debugger"
    )

    for tool in "${!optional_tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            print_success "$tool found (${optional_tools[$tool]})"
        else
            print_warning "$tool not found (${optional_tools[$tool]}) - optional"
        fi
    done

    # Auto-fix missing tools
    if [[ $tools_errors -gt 0 && "$AUTO_FIX" == true ]]; then
        print_status "Attempting to install missing tools..."
        case "$PLATFORM" in
            "linux"|"wsl")
                if command -v apt-get >/dev/null 2>&1; then
                    sudo apt-get update
                    sudo apt-get install -y ninja-build git pkg-config ccache clang-tidy valgrind gdb
                fi
                ;;
            "macos")
                if command -v brew >/dev/null 2>&1; then
                    brew install ninja git pkg-config ccache clang-tidy
                fi
                ;;
        esac
    fi

    return $tools_errors
}

setup_environment() {
    print_section "SETTING UP DEVELOPMENT ENVIRONMENT"

    # Force auto-fix for setup mode
    AUTO_FIX=true

    local setup_errors=0

    # Run all checks with auto-fix enabled
    check_compilers || ((setup_errors++))
    check_cmake || ((setup_errors++))
    check_development_tools || ((setup_errors++))

    # Platform-specific setup
    case "$PLATFORM" in
        "wsl")
            fix_wsl_audio
            ;;
        "docker")
            print_status "Container environment detected - skipping system-level changes"
            ;;
    esac

    # Create build directory if it doesn't exist
    if [[ ! -d "$PROJECT_ROOT/build" ]]; then
        mkdir "$PROJECT_ROOT/build"
        print_success "Created build directory"
    fi

    if [[ $setup_errors -eq 0 ]]; then
        print_success "Environment setup completed successfully"
    else
        print_warning "Environment setup completed with $setup_errors issues"
    fi

    return $setup_errors
}

validate_environment() {
    print_section "VALIDATING ENVIRONMENT"

    local validation_errors=0

    check_compilers || ((validation_errors++))
    check_cmake || ((validation_errors++))
    check_audio_system || ((validation_errors++))
    check_development_tools || ((validation_errors++))

    if [[ $validation_errors -eq 0 ]]; then
        print_success "✅ Environment validation passed"
    else
        print_warning "⚠️  Environment validation found $validation_errors issues"
    fi

    return $validation_errors
}

generate_environment_report() {
    local report_file="environment_report_$TIMESTAMP.txt"

    {
        echo "# Environment Setup Report - $(date)"
        echo "====================================="
        echo ""
        echo "## Platform Information"
        echo "Platform: $PLATFORM"
        echo "Container: $IN_CONTAINER"
        echo "WSL: $IN_WSL"
        echo "Audio Available: $HAS_AUDIO"
        echo ""
        echo "## Compiler Information"
        if command -v gcc >/dev/null 2>&1; then
            echo "GCC: $(gcc --version | head -n1)"
        fi
        if command -v clang++ >/dev/null 2>&1; then
            echo "Clang++: $(clang++ --version | head -n1)"
        fi
        echo ""
        echo "## Build Tools"
        if command -v cmake >/dev/null 2>&1; then
            echo "CMake: $(cmake --version | head -n1)"
        fi
        if command -v ninja >/dev/null 2>&1; then
            echo "Ninja: $(ninja --version)"
        fi
        echo ""
        echo "## Audio System"
        case "$PLATFORM" in
            "linux"|"wsl")
                if command -v pactl >/dev/null 2>&1; then
                    echo "PulseAudio: $(pactl --version)"
                fi
                if command -v aplay >/dev/null 2>&1; then
                    echo "ALSA: Available"
                fi
                ;;
        esac
        echo ""
        echo "## Recommendations"
        echo "- Run the project build to verify everything works"
        echo "- Check audio functionality if developing audio features"
        echo "- Consider installing optional development tools for better experience"

    } > "$report_file"

    print_success "Environment report saved to: $report_file"
}

main() {
    local mode=$(parse_arguments "$@")

    print_header "UNIFIED ENVIRONMENT SETUP - $(echo $mode | tr '[:lower:]' '[:upper:]')"

    echo "🕒 Setup Date: $(date)" | tee "$LOG_FILE"
    echo "🎯 Mode: $mode" | tee -a "$LOG_FILE"
    echo "📍 Project: Huntmaster Engine" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"

    detect_platform

    local exit_code=0

    case "$mode" in
        "validate")
            validate_environment || exit_code=$?
            ;;
        "setup")
            setup_environment || exit_code=$?
            ;;
        "fix-audio")
            fix_wsl_audio
            ;;
    esac

    generate_environment_report

    print_header "OPERATION COMPLETE"

    if [[ $exit_code -eq 0 ]]; then
        print_success "Environment setup/validation completed successfully"
    else
        print_warning "Environment setup/validation completed with issues (exit code: $exit_code)"
    fi

    echo "📄 Detailed log saved to: $(basename "$LOG_FILE")"

    exit $exit_code
}

# Execute main function with all arguments
main "$@"
