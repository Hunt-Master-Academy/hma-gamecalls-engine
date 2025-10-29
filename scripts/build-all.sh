#!/bin/bash
# [20251028-DOCKER-001] Build script for GameCalls Engine backend + native bindings
# Builds C++ engine, Node-API bindings, then installs Node.js dependencies

set -e  # Exit on error

echo "=== GameCalls Engine Build Script ==="

# [20251028-DOCKER-002] Build C++ engine first
echo ""
echo "Step 1: Building C++ engine..."
cd /home/xbyooki/projects/hma-gamecalls-engine

if [ ! -d "build" ]; then
    mkdir -p build
fi

cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build . -j$(nproc)

echo "✓ C++ engine built successfully"

# [20251028-DOCKER-003] Build Node-API bindings
echo ""
echo "Step 2: Building Node-API bindings..."
cd ../bindings/node-api
npm install
npm run build

echo "✓ Node-API bindings built successfully"

# [20251028-DOCKER-004] Install backend dependencies
echo ""
echo "Step 3: Installing backend dependencies..."
cd ../../backend
npm install

echo "✓ Backend dependencies installed"

echo ""
echo "=== Build Complete ==="
echo "C++ Engine: build/src/libhuntmaster_core.so"
echo "Node-API Addon: bindings/node-api/build/Release/gamecalls_engine.node"
echo "Backend: backend/node_modules/"
echo ""
echo "Start server: cd backend && npm start"
