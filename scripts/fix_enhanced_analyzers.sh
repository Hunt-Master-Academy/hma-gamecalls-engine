#!/bin/bash
# Quick fix script for enhanced analyzer compilation issues

cd /workspaces/huntmaster-engine

echo "🔧 Fixing Enhanced Analyzer compilation issues..."

# Fix PitchTracker.cpp
echo "Fixing PitchTracker.cpp..."
sed -i 's/Result<.*>::ok(/expected(/g' src/core/PitchTracker.cpp
sed -i 's/Result<.*>::error(/unexpected(/g' src/core/PitchTracker.cpp
sed -i 's/\.isOk()/.has_value()/g' src/core/PitchTracker.cpp
sed -i 's/DebugComponent::CORE/Component::GENERAL/g' src/core/PitchTracker.cpp

# Fix HarmonicAnalyzer.cpp
echo "Fixing HarmonicAnalyzer.cpp..."
sed -i 's/Result<.*>::success(/expected(/g' src/core/HarmonicAnalyzer.cpp
sed -i 's/Result<.*>::failure(/unexpected(/g' src/core/HarmonicAnalyzer.cpp
sed -i 's/\.isOk()/.has_value()/g' src/core/HarmonicAnalyzer.cpp
sed -i 's/DEBUG_LOG.*$/\/\/ DEBUG_LOG removed for compilation/g' src/core/HarmonicAnalyzer.cpp

# Fix CadenceAnalyzer.cpp
echo "Fixing CadenceAnalyzer.cpp..."
sed -i 's/Result<.*>::success(/expected(/g' src/core/CadenceAnalyzer.cpp
sed -i 's/Result<.*>::failure(/unexpected(/g' src/core/CadenceAnalyzer.cpp
sed -i 's/\.isOk()/.has_value()/g' src/core/CadenceAnalyzer.cpp
sed -i 's/DEBUG_LOG.*$/\/\/ DEBUG_LOG removed for compilation/g' src/core/CadenceAnalyzer.cpp

echo "✅ Enhanced analyzers compilation fixes applied"
echo "🔄 Attempting to build..."

timeout 60 cmake --build build
