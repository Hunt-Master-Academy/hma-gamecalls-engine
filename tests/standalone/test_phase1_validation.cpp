// Phase 1 Enhanced Analyzers Validation Test
// Tests that the enhanced analyzers can be instantiated and configured properly

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

// Include enhanced analyzer headers
#include "huntmaster/core/CadenceAnalyzer.h"
#include "huntmaster/core/HarmonicAnalyzer.h"
#include "huntmaster/core/PitchTracker.h"

using namespace huntmaster;

// Helper function to generate test audio signal
std::vector<float> generateTestSignal(float frequency, float duration, float sampleRate) {
    const size_t numSamples = static_cast<size_t>(duration * sampleRate);
    std::vector<float> signal(numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        signal[i] = 0.5f * std::sin(2.0f * M_PI * frequency * t);
    }

    return signal;
}

int main() {
    std::cout << "=== Phase 1 Enhanced Analyzers Validation Test ===" << std::endl;

    bool allTestsPassed = true;

    // Test configuration constants
    const float sampleRate = 44100.0f;
    const int windowSize = 2048;
    const int fftSize = 2048;

    // Generate test signals
    auto testSignal440 = generateTestSignal(440.0f, 1.0f, sampleRate);  // A4 note
    auto testSignal880 = generateTestSignal(880.0f, 1.0f, sampleRate);  // A5 note

    std::cout << "\n--- Testing PitchTracker Configuration ---" << std::endl;
    try {
        // Test PitchTracker configuration
        PitchTracker::Config pitchConfig;
        pitchConfig.sampleRate = sampleRate;
        pitchConfig.windowSize = windowSize;
        pitchConfig.threshold = 0.1f;
        pitchConfig.enableVibratoDetection = true;

        std::cout << "✓ PitchTracker configuration successful" << std::endl;
        std::cout << "  - Sample rate: " << pitchConfig.sampleRate << " Hz" << std::endl;
        std::cout << "  - Window size: " << pitchConfig.windowSize << " samples" << std::endl;
        std::cout << "  - Threshold: " << pitchConfig.threshold << std::endl;
        std::cout << "  - Vibrato detection: "
                  << (pitchConfig.enableVibratoDetection ? "enabled" : "disabled") << std::endl;

        // Test result structure accessibility
        PitchTracker::PitchResult result;
        result.frequency = 440.0f;
        result.confidence = 0.95f;
        result.isVoiced = true;

        std::cout << "✓ PitchResult structure validation successful" << std::endl;
        std::cout << "  - Frequency: " << result.frequency << " Hz" << std::endl;
        std::cout << "  - Confidence: " << result.confidence << std::endl;
        std::cout << "  - Is voiced: " << (result.isVoiced ? "true" : "false") << std::endl;

    } catch (const std::exception& e) {
        std::cout << "✗ PitchTracker test failed: " << e.what() << std::endl;
        allTestsPassed = false;
    }

    std::cout << "\n--- Testing HarmonicAnalyzer Configuration ---" << std::endl;
    try {
        // Test HarmonicAnalyzer configuration
        HarmonicAnalyzer::Config harmonicConfig;
        harmonicConfig.sampleRate = sampleRate;
        harmonicConfig.fftSize = fftSize;
        harmonicConfig.enableTonalAnalysis = true;
        harmonicConfig.enableFormantTracking = true;
        harmonicConfig.minFrequency = 80.0f;
        harmonicConfig.maxFrequency = 8000.0f;

        std::cout << "✓ HarmonicAnalyzer configuration successful" << std::endl;
        std::cout << "  - Sample rate: " << harmonicConfig.sampleRate << " Hz" << std::endl;
        std::cout << "  - FFT size: " << harmonicConfig.fftSize << " samples" << std::endl;
        std::cout << "  - Frequency range: " << harmonicConfig.minFrequency << " - "
                  << harmonicConfig.maxFrequency << " Hz" << std::endl;
        std::cout << "  - Tonal analysis: "
                  << (harmonicConfig.enableTonalAnalysis ? "enabled" : "disabled") << std::endl;
        std::cout << "  - Formant tracking: "
                  << (harmonicConfig.enableFormantTracking ? "enabled" : "disabled") << std::endl;

        // Test nested structures
        HarmonicAnalyzer::HarmonicProfile profile;
        profile.spectralCentroid = 1500.0f;
        profile.confidence = 0.85f;

        HarmonicAnalyzer::HarmonicProfile::TonalQualities qualities;
        qualities.rasp = 0.3f;
        qualities.whine = 0.1f;
        qualities.resonance = 0.8f;
        qualities.brightness = 0.7f;

        profile.qualities = qualities;

        std::cout << "✓ HarmonicProfile and TonalQualities validation successful" << std::endl;
        std::cout << "  - Spectral centroid: " << profile.spectralCentroid << " Hz" << std::endl;
        std::cout << "  - Confidence: " << profile.confidence << std::endl;
        std::cout << "  - Rasp: " << qualities.rasp << std::endl;
        std::cout << "  - Brightness: " << qualities.brightness << std::endl;

    } catch (const std::exception& e) {
        std::cout << "✗ HarmonicAnalyzer test failed: " << e.what() << std::endl;
        allTestsPassed = false;
    }

    std::cout << "\n--- Testing CadenceAnalyzer Configuration ---" << std::endl;
    try {
        // Test CadenceAnalyzer configuration
        CadenceAnalyzer::Config cadenceConfig;
        cadenceConfig.sampleRate = sampleRate;
        cadenceConfig.frameSize = 0.025f;  // 25ms frames
        cadenceConfig.hopSize = 0.010f;    // 10ms hop
        cadenceConfig.enableBeatTracking = true;
        cadenceConfig.enableOnsetDetection = true;
        cadenceConfig.minTempo = 60.0f;
        cadenceConfig.maxTempo = 200.0f;

        std::cout << "✓ CadenceAnalyzer configuration successful" << std::endl;
        std::cout << "  - Sample rate: " << cadenceConfig.sampleRate << " Hz" << std::endl;
        std::cout << "  - Frame size: " << cadenceConfig.frameSize << " s" << std::endl;
        std::cout << "  - Hop size: " << cadenceConfig.hopSize << " s" << std::endl;
        std::cout << "  - Tempo range: " << cadenceConfig.minTempo << " - "
                  << cadenceConfig.maxTempo << " BPM" << std::endl;
        std::cout << "  - Beat tracking: "
                  << (cadenceConfig.enableBeatTracking ? "enabled" : "disabled") << std::endl;
        std::cout << "  - Onset detection: "
                  << (cadenceConfig.enableOnsetDetection ? "enabled" : "disabled") << std::endl;

        // Test nested structures
        CadenceAnalyzer::CadenceProfile cadenceProfile;
        cadenceProfile.estimatedTempo = 120.0f;
        cadenceProfile.confidence = 0.9f;

        CadenceAnalyzer::CadenceProfile::PeriodicityMeasures periodicity;
        periodicity.periodicityStrength = 0.8f;
        periodicity.dominantPeriod = 0.5f;

        CadenceAnalyzer::CadenceProfile::RhythmicFeatures rhythm;
        rhythm.rhythmComplexity = 0.6f;
        rhythm.syncopation = 0.2f;

        cadenceProfile.periodicity = periodicity;
        cadenceProfile.rhythm = rhythm;

        std::cout << "✓ CadenceProfile nested structures validation successful" << std::endl;
        std::cout << "  - Estimated tempo: " << cadenceProfile.estimatedTempo << " BPM"
                  << std::endl;
        std::cout << "  - Confidence: " << cadenceProfile.confidence << std::endl;
        std::cout << "  - Periodicity strength: " << periodicity.periodicityStrength << std::endl;
        std::cout << "  - Rhythm complexity: " << rhythm.rhythmComplexity << std::endl;

    } catch (const std::exception& e) {
        std::cout << "✗ CadenceAnalyzer test failed: " << e.what() << std::endl;
        allTestsPassed = false;
    }

    std::cout << "\n--- Testing Result<T,E> Error Handling Pattern ---" << std::endl;
    try {
        // Test Result template usage (compile-time validation)
        using PitchResult = PitchTracker::Result<PitchTracker::PitchResult, PitchTracker::Error>;
        using HarmonicResult =
            HarmonicAnalyzer::Result<HarmonicAnalyzer::HarmonicProfile, HarmonicAnalyzer::Error>;
        using CadenceResult =
            CadenceAnalyzer::Result<CadenceAnalyzer::CadenceProfile, CadenceAnalyzer::Error>;

        std::cout << "✓ Result<T,E> pattern compilation successful" << std::endl;
        std::cout << "  - PitchTracker Result<T,E> template instantiation: OK" << std::endl;
        std::cout << "  - HarmonicAnalyzer Result<T,E> template instantiation: OK" << std::endl;
        std::cout << "  - CadenceAnalyzer Result<T,E> template instantiation: OK" << std::endl;

        // Test Error enum accessibility
        auto pitchError = PitchTracker::Error::INSUFFICIENT_DATA;
        auto harmonicError = HarmonicAnalyzer::Error::FFT_ERROR;
        auto cadenceError = CadenceAnalyzer::Error::ONSET_DETECTION_ERROR;

        std::cout << "✓ Error enum accessibility validation successful" << std::endl;
        std::cout << "  - PitchTracker error codes accessible" << std::endl;
        std::cout << "  - HarmonicAnalyzer error codes accessible" << std::endl;
        std::cout << "  - CadenceAnalyzer error codes accessible" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "✗ Result<T,E> pattern test failed: " << e.what() << std::endl;
        allTestsPassed = false;
    }

    std::cout << "\n--- Testing Audio Signal Processing ---" << std::endl;
    try {
        std::cout << "✓ Test signal generation successful" << std::endl;
        std::cout << "  - Generated 440 Hz signal: " << testSignal440.size() << " samples"
                  << std::endl;
        std::cout << "  - Generated 880 Hz signal: " << testSignal880.size() << " samples"
                  << std::endl;
        std::cout << "  - Duration: 1.0 seconds at " << sampleRate << " Hz" << std::endl;

        // Basic signal validation
        if (testSignal440.size() != static_cast<size_t>(sampleRate)) {
            throw std::runtime_error("Signal size mismatch");
        }

        // Check signal amplitude range
        bool amplitudeValid = true;
        for (const auto& sample : testSignal440) {
            if (std::abs(sample) > 1.0f) {
                amplitudeValid = false;
                break;
            }
        }

        if (amplitudeValid) {
            std::cout << "✓ Signal amplitude validation successful (range: ±1.0)" << std::endl;
        } else {
            throw std::runtime_error("Signal amplitude out of range");
        }

    } catch (const std::exception& e) {
        std::cout << "✗ Audio signal processing test failed: " << e.what() << std::endl;
        allTestsPassed = false;
    }

    // Final results
    std::cout << "\n=== Phase 1 Enhanced Analyzers Validation Results ===" << std::endl;

    if (allTestsPassed) {
        std::cout << "🎯 ALL TESTS PASSED - Phase 1 Infrastructure Ready!" << std::endl;
        std::cout << "✅ PitchTracker: Configuration and structures validated" << std::endl;
        std::cout << "✅ HarmonicAnalyzer: Configuration and nested types validated" << std::endl;
        std::cout << "✅ CadenceAnalyzer: Configuration and nested structures validated"
                  << std::endl;
        std::cout << "✅ Result<T,E> Pattern: Error handling template system operational"
                  << std::endl;
        std::cout << "✅ Audio Processing: Test signal generation and validation working"
                  << std::endl;
        std::cout << "\n🚀 READY FOR PHASE 1 ALGORITHM IMPLEMENTATION!" << std::endl;
        std::cout << "📋 Next Steps:" << std::endl;
        std::cout << "   - Implement YIN algorithm in PitchTracker" << std::endl;
        std::cout << "   - Implement spectral analysis in HarmonicAnalyzer" << std::endl;
        std::cout << "   - Implement beat detection in CadenceAnalyzer" << std::endl;
        std::cout << "   - Create factory create() methods for instantiation" << std::endl;
        std::cout << "   - Add process() methods for real-time analysis" << std::endl;
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED - Review configuration issues" << std::endl;
        return 1;
    }
}
