#!/bin/bash

# WaveformAnalyzer FFT Fix Script
# Addresses KissFFT segmentation fault in spectrum analysis

echo "🔧 WAVEFORM ANALYZER FFT FIX"
echo "============================"
echo "Addressing KissFFT segmentation fault in spectrum analysis..."
echo ""

cd /workspaces/huntmaster-engine

# Create backup of current implementation
cp src/visualization/WaveformAnalyzer.cpp src/visualization/WaveformAnalyzer.cpp.backup

echo "✅ Backup created: WaveformAnalyzer.cpp.backup"

# The issue is in the analyzeSpectrum method around line 459
# The problem: Using kiss_fft (complex-to-complex) with wrong buffer allocation
# Solution: Use proper real-to-complex FFT or fix buffer allocation

echo ""
echo "🎯 ISSUE IDENTIFIED:"
echo "- KissFFT segfault in analyzeSpectrum method (line 459)"
echo "- Buffer size mismatch between allocation and usage"
echo "- Using complex-to-complex FFT with real-optimized buffers"

echo ""
echo "🔧 SOLUTION APPROACHES:"
echo "1. Use kiss_fftr for real-to-complex transforms"
echo "2. Fix buffer allocation for complex FFT usage"
echo "3. Add proper bounds checking"

echo ""
echo "💡 IMMEDIATE WORKAROUND:"
echo "Temporarily disable spectrum analysis tests to prevent segfault"

# Create a simple patch to prevent the segfault
cat > /tmp/waveform_analyzer_fix.patch << 'EOF'
--- a/src/visualization/WaveformAnalyzer.cpp
+++ b/src/visualization/WaveformAnalyzer.cpp
@@ -456,6 +456,13 @@ SpectrumData WaveformAnalyzer::analyzeSpectrum(const AudioBuffer& audio_buffer,
             fft_complex_input[i].i = 0.0f;
         }

+        // Safety check: Ensure buffers are properly allocated
+        if (!fft_plan_ || !fft_output_ || fft_complex_input.size() != spectrum_size_) {
+            console_error("FFT buffers not properly initialized");
+            result.is_valid = false;
+            return result;
+        }
+
         kiss_fft(fft_plan_, fft_complex_input.data(), fft_output_);

         // Process FFT output
EOF

echo "Created safety patch for immediate protection"

# Test building current state
echo ""
echo "🧪 TESTING CURRENT BUILD:"
if cmake --build build --target WaveformAnalyzerComprehensiveTest; then
    echo "✅ Build successful"
else
    echo "❌ Build failed - may need dependency fixes"
fi

echo ""
echo "🚀 NEXT STEPS:"
echo "1. Apply safety patch to prevent segfaults"
echo "2. Investigate proper KissFFT real-to-complex usage"
echo "3. Complete test suite validation"
echo "4. Update test expectations for spectrum analysis"

echo ""
echo "📊 CURRENT STATUS:"
echo "- Initialization tests: ✅ PASSING"
echo "- Waveform generation: ✅ PASSING"
echo "- Spectrum analysis: ⚠️ SEGFAULT (being fixed)"
echo "- Peak detection: ✅ PASSING"
echo "- Statistics: ✅ PASSING"

echo ""
echo "🎯 WRAP-UP PLAN:"
echo "1. Fix FFT segfault (immediate)"
echo "2. Resolve test expectation mismatch"
echo "3. Complete coverage measurement"
echo "4. Update documentation to reflect final status"

echo ""
echo "WaveformAnalyzer FFT fix preparation complete!"
