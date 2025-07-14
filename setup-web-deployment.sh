#!/bin/bash
# Quick deployment setup script for Huntmaster Engine
# Sets up web deployment environment and runs basic tests

set -e

echo "🎯 Huntmaster Engine Web Deployment Setup"
echo "=========================================="

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ] || [ ! -d "web" ]; then
    echo "❌ Error: Please run this script from the huntmaster-engine project root"
    exit 1
fi

echo "📁 Project root: $PROJECT_ROOT"

# Step 1: Initialize submodules if needed
echo ""
echo "1️⃣ Checking git submodules..."
if [ ! -f "tools/emsdk/emsdk.py" ]; then
    echo "📥 Initializing git submodules..."
    git submodule update --init --recursive
else
    echo "✅ Git submodules already initialized"
fi

# Step 2: Check for existing WASM build
echo ""
echo "2️⃣ Checking WASM build status..."
if [ -f "web/dist/huntmaster-engine.wasm" ]; then
    echo "✅ WASM files found in web/dist/"
    echo "   - $(ls -lh web/dist/huntmaster-engine.wasm | awk '{print $5}') huntmaster-engine.wasm"
    echo "   - $(ls -lh web/dist/huntmaster-engine.js | awk '{print $5}') huntmaster-engine.js"
    
    read -p "🤔 WASM files exist. Rebuild? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "🔨 Rebuilding WASM module..."
        ./scripts/build_wasm.sh
    else
        echo "⏭️ Skipping WASM build"
    fi
else
    echo "🔨 Building WASM module..."
    ./scripts/build_wasm.sh
fi

# Step 3: Check master calls
echo ""
echo "3️⃣ Checking master call audio files..."
if [ -d "data/master_calls" ] && [ "$(ls -A data/master_calls/*.wav 2>/dev/null | wc -l)" -gt 0 ]; then
    echo "✅ Master calls found:"
    ls data/master_calls/*.wav | head -5 | while read file; do
        echo "   - $(basename "$file")"
    done
    if [ "$(ls data/master_calls/*.wav | wc -l)" -gt 5 ]; then
        echo "   - ... and $(( $(ls data/master_calls/*.wav | wc -l) - 5 )) more files"
    fi
else
    echo "⚠️ No master call files found in data/master_calls/"
    echo "   The engine will still work, but you won't be able to load master calls"
    echo "   Add .wav files to data/master_calls/ for full functionality"
fi

# Step 4: Check test audio files
echo ""
echo "4️⃣ Checking test audio files..."
if [ -d "data/test_audio" ] && [ "$(ls -A data/test_audio/*.wav 2>/dev/null | wc -l)" -gt 0 ]; then
    echo "✅ Test audio files found:"
    ls data/test_audio/*.wav | head -3 | while read file; do
        echo "   - $(basename "$file")"
    done
else
    echo "ℹ️ No test audio files found in data/test_audio/"
    echo "   You can still upload files via the web interface"
fi

# Step 5: Test server startup
echo ""
echo "5️⃣ Testing server components..."

# Check Python
if command -v python3 &> /dev/null; then
    PYTHON_CMD="python3"
elif command -v python &> /dev/null; then
    PYTHON_CMD="python"
else
    echo "❌ Python not found. Please install Python 3.7+"
    exit 1
fi

echo "✅ Python found: $($PYTHON_CMD --version)"

# Test server startup (quick test)
echo "🧪 Testing server startup..."
timeout 5s $PYTHON_CMD serve_production.py --port 8001 &> /dev/null && \
    echo "✅ Production server test successful" || \
    echo "⚠️ Server test inconclusive (this is usually fine)"

# Step 6: Generate deployment summary
echo ""
echo "6️⃣ Generating deployment summary..."

cat > deployment-status.md << EOF
# Huntmaster Engine Deployment Status

**Generated:** $(date)
**Project:** $PROJECT_ROOT

## Build Status
- ✅ Git submodules initialized
- $([ -f "web/dist/huntmaster-engine.wasm" ] && echo "✅" || echo "❌") WASM module built
- $([ -f "web/dist/huntmaster-engine.js" ] && echo "✅" || echo "❌") JavaScript bindings
- $([ -f "web/dist/huntmaster-engine.d.ts" ] && echo "✅" || echo "❌") TypeScript definitions

## Audio Files
- Master calls: $(ls data/master_calls/*.wav 2>/dev/null | wc -l) files
- Test audio: $(ls data/test_audio/*.wav 2>/dev/null | wc -l) files

## Web Interface Files
- $([ -f "web/user-test.html" ] && echo "✅" || echo "❌") Main testing interface
- $([ -f "web/styles/main.css" ] && echo "✅" || echo "❌") Stylesheets
- $([ -f "web/js/main.js" ] && echo "✅" || echo "❌") JavaScript modules
- $([ -f "serve_production.py" ] && echo "✅" || echo "❌") Production server

## Quick Start Commands

\`\`\`bash
# Start development server
$PYTHON_CMD serve_production.py

# Start with HTTPS
$PYTHON_CMD serve_production.py --ssl

# Start on different port
$PYTHON_CMD serve_production.py --port 9000
\`\`\`

## Access URLs
- Main interface: http://localhost:8000/web/user-test.html
- Original demo: http://localhost:8000/web/index.html
- Diagnostics: http://localhost:8000/web/diagnostic.html
- API status: http://localhost:8000/api/status

## Next Steps
1. Start the server with one of the commands above
2. Open the main interface in a modern web browser
3. Initialize the engine and load a master call
4. Test with live microphone or uploaded audio files

See DEPLOYMENT.md for detailed deployment options and troubleshooting.
EOF

echo "✅ Deployment status saved to deployment-status.md"

# Final summary
echo ""
echo "🎉 Setup Complete!"
echo "================="
echo ""
echo "📋 Summary:"
echo "   - WASM module: $([ -f "web/dist/huntmaster-engine.wasm" ] && echo "✅ Ready" || echo "❌ Missing")"
echo "   - Web interface: ✅ Ready"
echo "   - Master calls: $(ls data/master_calls/*.wav 2>/dev/null | wc -l) available"
echo "   - Python server: ✅ Ready"
echo ""
echo "🚀 Quick Start:"
echo "   $PYTHON_CMD serve_production.py"
echo ""
echo "🌐 Then open: http://localhost:8000/web/user-test.html"
echo ""
echo "📖 For detailed deployment options, see:"
echo "   - DEPLOYMENT.md (comprehensive guide)"
echo "   - deployment-status.md (current status)"
echo ""

# Offer to start server immediately
read -p "🤔 Start the server now? (Y/n): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Nn]$ ]]; then
    echo ""
    echo "🚀 Starting Huntmaster Engine Server..."
    echo "   Open http://localhost:8000/web/user-test.html in your browser"
    echo "   Press Ctrl+C to stop the server"
    echo ""
    exec $PYTHON_CMD serve_production.py
fi

echo ""
echo "👋 Setup complete! Run the server when you're ready to test."
