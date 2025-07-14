#!/bin/bash
# build_wasm.sh - Cross-platform WASM build script for Huntmaster Engine
# Handles Emscripten setup via git submodule

set -e

# Project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "🚀 Building Huntmaster Engine for WebAssembly..."
echo "Project root: $PROJECT_ROOT"

# Check if submodules are initialized
if [ ! -f "tools/emsdk/emsdk.py" ]; then
    echo "❌ Emscripten SDK submodule not found. Initializing submodules..."
    git submodule update --init --recursive
fi

# Platform-specific Emscripten setup
EMSCRIPTEN_ROOT=""
EMCC_COMMAND=""

# Detect platform and set up Emscripten accordingly
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]] || [[ -n "$WSL_DISTRO_NAME" ]]; then
    echo "🔧 Detected Windows/WSL environment"
    
    # For WSL/Windows, try to use the Windows Emscripten installation if available
    if command -v emcc.bat &> /dev/null; then
        EMCC_COMMAND="emcc.bat"
        echo "✅ Using system Emscripten installation"
    elif [ -f "tools/emsdk/upstream/emscripten/emcc.py" ]; then
        # Use Python to run emcc directly
        EMCC_COMMAND="python tools/emsdk/upstream/emscripten/emcc.py"
        export EMSDK="$PROJECT_ROOT/tools/emsdk"
        export EMSDK_NODE="$PROJECT_ROOT/tools/emsdk/node/22.16.0_64bit/bin/node.exe"
        echo "✅ Using project Emscripten via Python"
    else
        echo "❌ Emscripten not properly set up. Setting up now..."
        cd tools/emsdk
        python emsdk.py install latest
        python emsdk.py activate latest
        cd "$PROJECT_ROOT"
        EMCC_COMMAND="python tools/emsdk/upstream/emscripten/emcc.py"
    fi
else
    echo "🔧 Detected Unix-like environment"
    
    # For Unix systems, use standard approach
    cd tools/emsdk
    python emsdk.py install latest >/dev/null 2>&1 || true
    python emsdk.py activate latest >/dev/null 2>&1 || true
    source emsdk_env.sh
    cd "$PROJECT_ROOT"
    EMCC_COMMAND="emcc"
fi

# Verify Emscripten is working
echo "🔍 Testing Emscripten..."
if ! $EMCC_COMMAND --version >/dev/null 2>&1; then
    echo "❌ Emscripten test failed. Falling back to Docker approach..."
    
    # Create Dockerfile for WASM build if Emscripten fails
    cat > Dockerfile.wasm << 'EOF'
FROM emscripten/emsdk:latest

WORKDIR /src
COPY . .

RUN apt-get update && apt-get install -y cmake

# Build WASM
RUN mkdir -p build-wasm && cd build-wasm && \
    emcmake cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DHUNTMASTER_BUILD_TESTS=OFF \
        -DHUNTMASTER_ENABLE_WASM=ON && \
    cmake --build . --target HuntmasterEngineWASM

CMD ["cp", "-r", "build-wasm/web/", "/output/"]
EOF
    
    echo "📦 Docker fallback created. Run: docker build -f Dockerfile.wasm -t huntmaster-wasm . && docker run -v \$(pwd)/dist:/output huntmaster-wasm"
    exit 1
fi

echo "✅ Emscripten verified: $($EMCC_COMMAND --version | head -n1)"

# Create build directory
BUILD_DIR="build-wasm"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "🏗️  Configuring CMake for WebAssembly..."

# Configure with CMake for Emscripten
if [[ "$EMCC_COMMAND" == *"python"* ]]; then
    # Use emcmake via Python when using project emsdk
    python ../tools/emsdk/upstream/emscripten/emcmake.py cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DHUNTMASTER_BUILD_TESTS=OFF \
        -DHUNTMASTER_ENABLE_WASM=ON \
        -DCMAKE_CROSSCOMPILING_EMULATOR=node
else
    # Use standard emcmake
    emcmake cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DHUNTMASTER_BUILD_TESTS=OFF \
        -DHUNTMASTER_ENABLE_WASM=ON \
        -DCMAKE_CROSSCOMPILING_EMULATOR=node
fi

echo "🔨 Building WebAssembly module..."
cmake --build . --target HuntmasterEngineWASM --config Release

# Verify build outputs
if [ -f "huntmaster-engine.wasm" ] && [ -f "huntmaster-engine.js" ]; then
    echo "✅ WASM build successful!"
    
    # Copy to web directory
    mkdir -p ../web/dist
    cp huntmaster-engine.wasm ../web/dist/
    cp huntmaster-engine.js ../web/dist/
    
    # Copy any generated TypeScript definitions
    if [ -f "huntmaster-engine.d.ts" ]; then
        cp huntmaster-engine.d.ts ../web/dist/
    fi
    
    echo "📦 WASM files copied to web/dist/"
    echo "   - huntmaster-engine.wasm ($(du -h huntmaster-engine.wasm | cut -f1))"
    echo "   - huntmaster-engine.js ($(du -h huntmaster-engine.js | cut -f1))"
    
else
    echo "❌ WASM build failed - output files not found"
    exit 1
fi

cd "$PROJECT_ROOT"

echo "🎉 WASM build complete! Ready for web deployment."
echo ""
echo "Next steps:"
echo "  1. Start development server: python serve_dev.py"
echo "  2. Open http://localhost:8000/web/ in your browser"
echo "  3. Test with real audio files from data/master_calls/"
