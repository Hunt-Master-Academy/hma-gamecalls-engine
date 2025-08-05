// Simple Enhanced Analyzers Integration Test
// Tests if the enhanced analyzers can be properly integrated

#include <iostream>
#include <memory>

// Include the enhanced analyzer headers
#include "huntmaster/core/CadenceAnalyzer.h"
#include "huntmaster/core/HarmonicAnalyzer.h"
#include "huntmaster/core/PitchTracker.h"

using namespace huntmaster;

int main() {
    std::cout << "=== Enhanced Analyzers Integration Test ===" << std::endl;

    // Test PitchTracker factory method
    {
        PitchTracker::Config config;
        config.sampleRate = 44100.0f;
        config.windowSize = 1024;
        config.hopSize = 512;

        std::cout << "Testing PitchTracker::create..." << std::endl;
        auto result = PitchTracker::create(config);

        if (result) {
            std::cout << "✓ PitchTracker factory method working" << std::endl;
        } else {
            std::cout << "✗ PitchTracker factory method failed" << std::endl;
            return 1;
        }
    }

    // Test HarmonicAnalyzer factory method
    {
        HarmonicAnalyzer::Config config;
        config.sampleRate = 44100.0f;
        config.fftSize = 4096;
        config.hopSize = 1024;

        std::cout << "Testing HarmonicAnalyzer::create..." << std::endl;
        auto result = HarmonicAnalyzer::create(config);

        if (result) {
            std::cout << "✓ HarmonicAnalyzer factory method working" << std::endl;
        } else {
            std::cout << "✗ HarmonicAnalyzer factory method failed" << std::endl;
            return 1;
        }
    }

    // Test CadenceAnalyzer factory method
    {
        CadenceAnalyzer::Config config;
        config.sampleRate = 44100.0f;
        config.frameSize = 0.025f;
        config.hopSize = 0.010f;

        std::cout << "Testing CadenceAnalyzer::create..." << std::endl;
        auto result = CadenceAnalyzer::create(config);

        if (result) {
            std::cout << "✓ CadenceAnalyzer factory method working" << std::endl;
        } else {
            std::cout << "✗ CadenceAnalyzer factory method failed" << std::endl;
            return 1;
        }
    }

    std::cout << "\n🎯 ALL ENHANCED ANALYZERS INTEGRATED SUCCESSFULLY!" << std::endl;
    std::cout << "✅ PitchTracker: Factory method operational" << std::endl;
    std::cout << "✅ HarmonicAnalyzer: Factory method operational" << std::endl;
    std::cout << "✅ CadenceAnalyzer: Factory method operational" << std::endl;
    std::cout << "\n🚀 READY FOR COMMIT TO GITHUB REPOSITORY!" << std::endl;

    return 0;
}
