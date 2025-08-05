// Simple test for Phase 1 Enhanced Analyzers
// Tests basic instantiation and configuration of PitchTracker, HarmonicAnalyzer, CadenceAnalyzer

#include <cmath>
#include <iostream>
#include <vector>

#include "huntmaster/core/CadenceAnalyzer.h"
#include "huntmaster/core/HarmonicAnalyzer.h"
#include "huntmaster/core/PitchTracker.h"

using namespace huntmaster;

// Generate a simple test signal
std::vector<float> generateTestSignal(float frequency, float duration, float sampleRate) {
    size_t numSamples = static_cast<size_t>(duration * sampleRate);
    std::vector<float> signal(numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        signal[i] = 0.5f * std::sin(2.0f * M_PI * frequency * i / sampleRate);
    }
    return signal;
}

int main() {
    std::cout << "=== Phase 1 Enhanced Analyzers Test ===" << std::endl;

    const float sampleRate = 44100.0f;
    const float testFreq = 440.0f;  // A4 note
    const float duration = 1.0f;    // 1 second

    // Generate test audio signal
    auto testAudio = generateTestSignal(testFreq, duration, sampleRate);
    std::cout << "✓ Generated test signal: " << testFreq << "Hz, " << duration << "s, "
              << testAudio.size() << " samples" << std::endl;

    // Test PitchTracker
    std::cout << "\n--- Testing PitchTracker ---" << std::endl;
    try {
        PitchTracker::Config pitchConfig;
        pitchConfig.sampleRate = sampleRate;
        pitchConfig.windowSize = 2048;
        pitchConfig.enableVibratoDetection = true;

        auto pitchResult = PitchTracker::create(pitchConfig);
        if (pitchResult.has_value()) {
            std::cout << "✓ PitchTracker created successfully" << std::endl;
            auto tracker = std::move(pitchResult.value());

            // Test pitch detection
            auto detectionResult = tracker->detectPitch(std::span<const float>(testAudio));
            if (detectionResult.has_value()) {
                auto result = detectionResult.value();
                std::cout << "✓ Pitch detection successful - Frequency: " << result.frequency
                          << "Hz, Confidence: " << result.confidence << std::endl;
            } else {
                std::cout << "✗ Pitch detection failed" << std::endl;
            }
        } else {
            std::cout << "✗ Failed to create PitchTracker" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ PitchTracker exception: " << e.what() << std::endl;
    }

    // Test HarmonicAnalyzer
    std::cout << "\n--- Testing HarmonicAnalyzer ---" << std::endl;
    try {
        HarmonicAnalyzer::Config harmonicConfig;
        harmonicConfig.sampleRate = sampleRate;
        harmonicConfig.fftSize = 2048;
        harmonicConfig.enableTonalAnalysis = true;

        auto harmonicResult = HarmonicAnalyzer::create(harmonicConfig);
        if (harmonicResult.has_value()) {
            std::cout << "✓ HarmonicAnalyzer created successfully" << std::endl;
            auto analyzer = std::move(harmonicResult.value());

            // Test harmonic analysis
            auto analysisResult = analyzer->analyzeHarmonics(std::span<const float>(testAudio));
            if (analysisResult.has_value()) {
                auto profile = analysisResult.value();
                std::cout << "✓ Harmonic analysis successful - Spectral Centroid: "
                          << profile.spectralCentroid << "Hz, Confidence: " << profile.confidence
                          << std::endl;
            } else {
                std::cout << "✗ Harmonic analysis failed" << std::endl;
            }
        } else {
            std::cout << "✗ Failed to create HarmonicAnalyzer" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ HarmonicAnalyzer exception: " << e.what() << std::endl;
    }

    // Test CadenceAnalyzer
    std::cout << "\n--- Testing CadenceAnalyzer ---" << std::endl;
    try {
        CadenceAnalyzer::Config cadenceConfig;
        cadenceConfig.sampleRate = sampleRate;
        cadenceConfig.frameSize = 0.025f;  // 25ms frames
        cadenceConfig.enableBeatTracking = true;

        auto cadenceResult = CadenceAnalyzer::create(cadenceConfig);
        if (cadenceResult.has_value()) {
            std::cout << "✓ CadenceAnalyzer created successfully" << std::endl;
            auto analyzer = std::move(cadenceResult.value());

            // Test cadence analysis
            auto analysisResult = analyzer->analyzeCadence(std::span<const float>(testAudio));
            if (analysisResult.has_value()) {
                auto profile = analysisResult.value();
                std::cout << "✓ Cadence analysis successful - Estimated Tempo: "
                          << profile.estimatedTempo << " BPM, Confidence: " << profile.confidence
                          << std::endl;
            } else {
                std::cout << "✗ Cadence analysis failed" << std::endl;
            }
        } else {
            std::cout << "✗ Failed to create CadenceAnalyzer" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "✗ CadenceAnalyzer exception: " << e.what() << std::endl;
    }

    std::cout << "\n=== Phase 1 Enhanced Analyzers Test Complete ===" << std::endl;
    return 0;
}
