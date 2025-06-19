#!/bin/bash

echo "Setting up Kiss FFT for Huntmaster Engine..."

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}Error: Please run this script from the huntmaster-engine root directory${NC}"
    exit 1
fi

# Create libs directory if it doesn't exist
mkdir -p libs
cd libs

# Clone Kiss FFT if not already present
if [ ! -d "kissfft" ]; then
    echo "Cloning Kiss FFT..."
    git clone https://github.com/mborgerding/kissfft.git
    cd kissfft
    git checkout v1.3.0
    cd ..
else
    echo "Kiss FFT already exists in libs/kissfft"
fi

# Copy our CMakeLists.txt for Kiss FFT
cd kissfft
if [ ! -f "CMakeLists.txt" ]; then
    echo "Creating CMakeLists.txt for Kiss FFT..."
    cat > CMakeLists.txt << 'EOF'
# CMakeLists.txt for Kiss FFT integration
cmake_minimum_required(VERSION 3.14)
project(kissfft C)

# Kiss FFT source files
set(KISSFFT_SOURCES
    kiss_fft.c
    tools/kiss_fftr.c  # Real-valued FFT (what we need for audio)
)

# Create static library
add_library(kissfft STATIC ${KISSFFT_SOURCES})

# Set include directories
target_include_directories(kissfft
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tools>
        $<INSTALL_INTERFACE:include>
)

# Compile definitions
target_compile_definitions(kissfft PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
)

# Set C standard
set_target_properties(kissfft PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

# Export for use in parent project
if(NOT TARGET kissfft::kissfft)
    add_library(kissfft::kissfft ALIAS kissfft)
endif()
EOF
fi

cd ../..

echo -e "${GREEN}Kiss FFT setup complete!${NC}"
echo ""
echo "Now you can build the project:"
echo "  cd build"
echo "  rm -rf *"
echo "  cmake -G \"MinGW Makefiles\" -DCMAKE_BUILD_TYPE=Debug .."
echo "  cmake --build ."