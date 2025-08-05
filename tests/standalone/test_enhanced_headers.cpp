// Header-only test for Phase 1 Enhanced Analyzers
// Tests that headers can be included and basic types are accessible

#include <iostream>
#include <vector>

// Include the enhanced analyzer headers
#include "huntmaster/core/CadenceAnalyzer.h"
#include "huntmaster/core/HarmonicAnalyzer.h"
#include "huntmaster/core/PitchTracker.h"

using namespace huntmaster;

int main() {
    std::cout << "=== Phase 1 Enhanced Analyzers Header Test ===" << std::endl;

    // Test that we can reference the types and configs
    std::cout << "\n--- Testing Type Accessibility ---" << std::endl;

    try {
        // Test PitchTracker types
        PitchTracker::Config pitchConfig;
        pitchConfig.sampleRate = 44100.0f;
        pitchConfig.windowSize = 2048;
        std::cout << "✓ PitchTracker::Config accessible - sample rate: " << pitchConfig.sampleRate
                  << std::endl;

        PitchTracker::PitchResult pitchResult;
        pitchResult.frequency = 440.0f;
        pitchResult.confidence = 0.95f;
        std::cout << "✓ PitchTracker::PitchResult accessible - frequency: " << pitchResult.frequency
                  << std::endl;

        // Test HarmonicAnalyzer types
        HarmonicAnalyzer::Config harmonicConfig;
        harmonicConfig.sampleRate = 44100.0f;
        harmonicConfig.fftSize = 2048;
        std::cout << "✓ HarmonicAnalyzer::Config accessible - FFT size: " << harmonicConfig.fftSize
                  << std::endl;

        HarmonicAnalyzer::HarmonicProfile profile;
        profile.spectralCentroid = 1500.0f;
        profile.confidence = 0.85f;
        std::cout << "✓ HarmonicAnalyzer::HarmonicProfile accessible - centroid: "
                  << profile.spectralCentroid << std::endl;

        // Test nested TonalQualities
        HarmonicAnalyzer::HarmonicProfile::TonalQualities qualities;
        qualities.rasp = 0.3f;
        qualities.brightness = 0.7f;
        std::cout << "✓ HarmonicProfile::TonalQualities accessible - rasp: " << qualities.rasp
                  << std::endl;

        // Test CadenceAnalyzer types
        CadenceAnalyzer::Config cadenceConfig;
        cadenceConfig.sampleRate = 44100.0f;
        cadenceConfig.frameSize = 0.025f;
        std::cout << "✓ CadenceAnalyzer::Config accessible - frame size: "
                  << cadenceConfig.frameSize << std::endl;

        CadenceAnalyzer::CadenceProfile cadenceProfile;
        cadenceProfile.estimatedTempo = 120.0f;
        cadenceProfile.confidence = 0.9f;
        std::cout << "✓ CadenceAnalyzer::CadenceProfile accessible - tempo: "
                  << cadenceProfile.estimatedTempo << std::endl;

        // Test nested structures
        CadenceAnalyzer::CadenceProfile::PeriodicityMeasures periodicity;
        periodicity.periodicityStrength = 0.8f;
        std::cout << "✓ CadenceProfile::PeriodicityMeasures accessible - strength: "
                  << periodicity.periodicityStrength << std::endl;

        CadenceAnalyzer::CadenceProfile::RhythmicFeatures rhythm;
        rhythm.rhythmComplexity = 0.6f;
        std::cout << "✓ CadenceProfile::RhythmicFeatures accessible - complexity: "
                  << rhythm.rhythmComplexity << std::endl;

    } catch (const std::exception& e) {
        std::cout << "✗ Exception during type testing: " << e.what() << std::endl;
        return 1;
    }

    // Test Result template usage
    std::cout << "\n--- Testing Result<T,E> Pattern ---" << std::endl;
    try {
        // These should compile even if we can't run them
        using PitchResult = PitchTracker::Result<PitchTracker::PitchResult, PitchTracker::Error>;
        using HarmonicResult =
            HarmonicAnalyzer::Result<HarmonicAnalyzer::HarmonicProfile, HarmonicAnalyzer::Error>;
        using CadenceResult =
            CadenceAnalyzer::Result<CadenceAnalyzer::CadenceProfile, CadenceAnalyzer::Error>;

        std::cout << "✓ PitchTracker Result<T,E> pattern accessible" << std::endl;
        std::cout << "✓ HarmonicAnalyzer Result<T,E> pattern accessible" << std::endl;
        std::cout << "✓ CadenceAnalyzer Result<T,E> pattern accessible" << std::endl;

    } catch (const std::exception& e) {
        std::cout << "✗ Exception during Result<T,E> testing: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n=== Phase 1 Enhanced Analyzers Infrastructure Validation PASSED ==="
              << std::endl;
    std::cout << "✅ All headers included successfully" << std::endl;
    std::cout << "✅ All configuration types accessible" << std::endl;
    std::cout << "✅ All result types accessible" << std::endl;
    std::cout << "✅ Nested structures properly qualified" << std::endl;
    std::cout << "✅ Result<T,E> error handling pattern established" << std::endl;
    std::cout << "🎯 Ready for Phase 1 algorithm implementation" << std::endl;

    return 0;
}
