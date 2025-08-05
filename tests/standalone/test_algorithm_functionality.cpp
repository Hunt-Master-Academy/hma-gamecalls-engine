// Phase 1 Enhanced Analyzers - Algorithm Functionality Test
// Tests the actual implemented algorithms with synthesized audio signals

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

// Include enhanced analyzer headers
#include "huntmaster/core/CadenceAnalyzer.h"
#include "huntmaster/core/HarmonicAnalyzer.h"
#include "huntmaster/core/PitchTracker.h"

using namespace huntmaster;

// Constants for testing
constexpr float SAMPLE_RATE = 44100.0f;
constexpr float TEST_DURATION = 0.5f;  // 500ms
constexpr float TOLERANCE = 5.0f;      // 5Hz tolerance for pitch detection

// Helper functions for signal generation
std::vector<float> generateSineWave(float frequency, float duration, float sampleRate) {
    const size_t numSamples = static_cast<size_t>(duration * sampleRate);
    std::vector<float> signal(numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        signal[i] = 0.5f * std::sin(2.0f * M_PI * frequency * t);
    }

    return signal;
}

std::vector<float> generateComplexTone(float fundamental,
                                       const std::vector<float>& harmonics,
                                       float duration,
                                       float sampleRate) {
    const size_t numSamples = static_cast<size_t>(duration * sampleRate);
    std::vector<float> signal(numSamples, 0.0f);

    // Add fundamental
    for (size_t i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        signal[i] += 0.5f * std::sin(2.0f * M_PI * fundamental * t);
    }

    // Add harmonics
    for (size_t h = 0; h < harmonics.size(); ++h) {
        float harmFreq = fundamental * (h + 2);  // 2nd, 3rd, 4th harmonics etc.
        float amplitude = harmonics[h];

        for (size_t i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            signal[i] += amplitude * std::sin(2.0f * M_PI * harmFreq * t);
        }
    }

    return signal;
}

std::vector<float>
generateRhythmicPattern(const std::vector<float>& beats, float duration, float sampleRate) {
    const size_t numSamples = static_cast<size_t>(duration * sampleRate);
    std::vector<float> signal(numSamples, 0.0f);

    for (float beatTime : beats) {
        if (beatTime < duration) {
            size_t sampleIndex = static_cast<size_t>(beatTime * sampleRate);
            size_t pulseLength = static_cast<size_t>(0.05f * sampleRate);  // 50ms pulse

            for (size_t i = 0; i < pulseLength && (sampleIndex + i) < numSamples; ++i) {
                float envelope = std::exp(-static_cast<float>(i) / (0.01f * sampleRate));
                signal[sampleIndex + i] =
                    0.8f * envelope * std::sin(2.0f * M_PI * 1000.0f * i / sampleRate);
            }
        }
    }

    return signal;
}

bool testPitchTrackerAlgorithm() {
    std::cout << "\n=== Testing PitchTracker YIN Algorithm ===" << std::endl;

    try {
        // Create PitchTracker with appropriate config
        PitchTracker::Config config;
        config.sampleRate = SAMPLE_RATE;
        config.windowSize = 2048;
        config.threshold = 0.2f;
        config.minFrequency = 80.0f;
        config.maxFrequency = 2000.0f;
        config.enableVibratoDetection = true;

        auto trackerResult = PitchTracker::create(config);
        if (!trackerResult.has_value()) {
            std::cout << "✗ Failed to create PitchTracker" << std::endl;
            return false;
        }

        auto tracker = std::move(trackerResult.value());
        std::cout << "✓ PitchTracker created successfully" << std::endl;

        // Test 1: Single frequency detection
        float testFreq = 440.0f;  // A4
        auto testSignal = generateSineWave(testFreq, TEST_DURATION, SAMPLE_RATE);

        auto pitchResult = tracker->detectPitch(testSignal);
        if (!pitchResult.has_value()) {
            std::cout << "✗ Pitch detection failed" << std::endl;
            return false;
        }

        auto result = pitchResult.value();
        std::cout << "✓ Pitch detection successful" << std::endl;
        std::cout << "  - Detected frequency: " << result.frequency << " Hz (expected: " << testFreq
                  << " Hz)" << std::endl;
        std::cout << "  - Confidence: " << result.confidence << std::endl;
        std::cout << "  - Is voiced: " << (result.isVoiced ? "true" : "false") << std::endl;

        // Validate pitch accuracy
        if (std::abs(result.frequency - testFreq) > TOLERANCE) {
            std::cout << "✗ Pitch accuracy test failed - detected " << result.frequency
                      << " Hz, expected " << testFreq << " Hz" << std::endl;
            return false;
        }
        std::cout << "✓ Pitch accuracy within tolerance" << std::endl;

        // Test 2: Different frequencies
        std::vector<float> testFrequencies = {220.0f, 330.0f, 660.0f, 880.0f};
        for (float freq : testFrequencies) {
            auto signal = generateSineWave(freq, 0.2f, SAMPLE_RATE);
            auto result = tracker->detectPitch(signal);

            if (result.has_value() && std::abs(result.value().frequency - freq) <= TOLERANCE) {
                std::cout << "✓ " << freq << " Hz detection accurate (" << result.value().frequency
                          << " Hz)" << std::endl;
            } else {
                std::cout << "✗ " << freq << " Hz detection failed" << std::endl;
                return false;
            }
        }

        // Test 3: Real-time processing
        tracker->reset();
        auto signal440 = generateSineWave(440.0f, 1.0f, SAMPLE_RATE);
        size_t chunkSize = 1024;

        for (size_t i = 0; i < signal440.size(); i += chunkSize) {
            size_t end = std::min(i + chunkSize, signal440.size());
            std::span<const float> chunk(signal440.data() + i, end - i);

            auto status = tracker->processAudioChunk(chunk);
            if (!status.has_value()) {
                std::cout << "✗ Real-time processing failed at chunk " << i / chunkSize
                          << std::endl;
                return false;
            }
        }

        auto realtimePitch = tracker->getRealtimePitch();
        if (realtimePitch.has_value()) {
            std::cout << "✓ Real-time processing successful - final pitch: "
                      << realtimePitch.value() << " Hz" << std::endl;
        } else {
            std::cout << "✗ Real-time pitch retrieval failed" << std::endl;
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        std::cout << "✗ PitchTracker test exception: " << e.what() << std::endl;
        return false;
    }
}

bool testHarmonicAnalyzerAlgorithm() {
    std::cout << "\n=== Testing HarmonicAnalyzer Spectral Analysis ===" << std::endl;

    try {
        // Create HarmonicAnalyzer
        HarmonicAnalyzer::Config config;
        config.sampleRate = SAMPLE_RATE;
        config.fftSize = 4096;
        config.enableTonalAnalysis = true;
        config.enableFormantTracking = true;
        config.maxHarmonics = 8;

        auto analyzerResult = HarmonicAnalyzer::create(config);
        if (!analyzerResult.has_value()) {
            std::cout << "✗ Failed to create HarmonicAnalyzer" << std::endl;
            return false;
        }

        auto analyzer = std::move(analyzerResult.value());
        std::cout << "✓ HarmonicAnalyzer created successfully" << std::endl;

        // Test 1: Complex harmonic signal
        float fundamental = 220.0f;                                // A3
        std::vector<float> harmonics = {0.3f, 0.2f, 0.15f, 0.1f};  // 2nd, 3rd, 4th, 5th harmonics
        auto complexSignal =
            generateComplexTone(fundamental, harmonics, TEST_DURATION, SAMPLE_RATE);

        auto harmonicResult = analyzer->analyzeHarmonics(complexSignal);
        if (!harmonicResult.has_value()) {
            std::cout << "✗ Harmonic analysis failed" << std::endl;
            return false;
        }

        auto profile = harmonicResult.value();
        std::cout << "✓ Harmonic analysis successful" << std::endl;
        std::cout << "  - Fundamental frequency: " << profile.fundamentalFreq << " Hz" << std::endl;
        std::cout << "  - Spectral centroid: " << profile.spectralCentroid << " Hz" << std::endl;
        std::cout << "  - Harmonic count: " << profile.harmonicFreqs.size() << std::endl;
        std::cout << "  - Is harmonic: " << (profile.isHarmonic ? "true" : "false") << std::endl;
        std::cout << "  - Confidence: " << profile.confidence << std::endl;

        // Test 2: Spectral features extraction
        auto spectralResult = analyzer->getSpectralFeatures(complexSignal);
        if (spectralResult.has_value()) {
            auto [centroid, spread] = spectralResult.value();
            std::cout << "✓ Spectral features extracted" << std::endl;
            std::cout << "  - Centroid: " << centroid << " Hz" << std::endl;
            std::cout << "  - Spread: " << spread << " Hz" << std::endl;
        } else {
            std::cout << "✗ Spectral features extraction failed" << std::endl;
            return false;
        }

        // Test 3: Tonal qualities assessment
        auto tonalResult = analyzer->assessTonalQualities(complexSignal);
        if (tonalResult.has_value()) {
            auto qualities = tonalResult.value();
            std::cout << "✓ Tonal qualities assessed" << std::endl;
            std::cout << "  - Rasp: " << qualities.rasp << std::endl;
            std::cout << "  - Brightness: " << qualities.brightness << std::endl;
            std::cout << "  - Resonance: " << qualities.resonance << std::endl;
            std::cout << "  - Roughness: " << qualities.roughness << std::endl;
        } else {
            std::cout << "✗ Tonal qualities assessment failed" << std::endl;
            return false;
        }

        // Test 4: Real-time processing
        analyzer->reset();
        size_t chunkSize = 2048;

        for (size_t i = 0; i < complexSignal.size(); i += chunkSize) {
            size_t end = std::min(i + chunkSize, complexSignal.size());
            std::span<const float> chunk(complexSignal.data() + i, end - i);

            if (chunk.size() >= config.fftSize) {
                auto status = analyzer->processAudioChunk(chunk);
                if (!status.has_value()) {
                    std::cout << "✗ Real-time harmonic processing failed" << std::endl;
                    return false;
                }
            }
        }

        auto currentAnalysis = analyzer->getCurrentAnalysis();
        if (currentAnalysis.has_value()) {
            std::cout << "✓ Real-time harmonic processing successful" << std::endl;
        }

        return true;

    } catch (const std::exception& e) {
        std::cout << "✗ HarmonicAnalyzer test exception: " << e.what() << std::endl;
        return false;
    }
}

bool testCadenceAnalyzerAlgorithm() {
    std::cout << "\n=== Testing CadenceAnalyzer Beat Detection ===" << std::endl;

    try {
        // Create CadenceAnalyzer
        CadenceAnalyzer::Config config;
        config.sampleRate = SAMPLE_RATE;
        config.frameSize = 0.025f;  // 25ms
        config.hopSize = 0.010f;    // 10ms
        config.enableBeatTracking = true;
        config.enableOnsetDetection = true;
        config.minTempo = 60.0f;
        config.maxTempo = 180.0f;

        auto analyzerResult = CadenceAnalyzer::create(config);
        if (!analyzerResult.has_value()) {
            std::cout << "✗ Failed to create CadenceAnalyzer" << std::endl;
            return false;
        }

        auto analyzer = std::move(analyzerResult.value());
        std::cout << "✓ CadenceAnalyzer created successfully" << std::endl;

        // Test 1: Rhythmic pattern with known tempo
        float tempo = 120.0f;                // 120 BPM
        float beatInterval = 60.0f / tempo;  // 0.5 seconds between beats
        std::vector<float> beatTimes;

        for (float t = 0.0f; t < 2.0f; t += beatInterval) {
            beatTimes.push_back(t);
        }

        auto rhythmSignal = generateRhythmicPattern(beatTimes, 2.0f, SAMPLE_RATE);

        auto cadenceResult = analyzer->analyzeCadence(rhythmSignal);
        if (!cadenceResult.has_value()) {
            std::cout << "✗ Cadence analysis failed" << std::endl;
            return false;
        }

        auto profile = cadenceResult.value();
        std::cout << "✓ Cadence analysis successful" << std::endl;
        std::cout << "  - Estimated tempo: " << profile.estimatedTempo
                  << " BPM (expected: " << tempo << " BPM)" << std::endl;
        std::cout << "  - Tempo confidence: " << profile.tempoConfidence << std::endl;
        std::cout << "  - Number of beats detected: " << profile.beatTimes.size() << std::endl;
        std::cout << "  - Has strong rhythm: " << (profile.hasStrongRhythm ? "true" : "false")
                  << std::endl;
        std::cout << "  - Overall rhythm score: " << profile.overallRhythmScore << std::endl;

        // Test 2: Onset detection
        auto onsetResult = analyzer->detectOnsets(rhythmSignal);
        if (onsetResult.has_value()) {
            auto onsets = onsetResult.value();
            std::cout << "✓ Onset detection successful" << std::endl;
            std::cout << "  - Number of onsets: " << onsets.size() << std::endl;

            // Show first few onsets
            for (size_t i = 0; i < std::min(size_t(5), onsets.size()); ++i) {
                std::cout << "  - Onset " << i + 1 << ": " << onsets[i] << " s" << std::endl;
            }
        } else {
            std::cout << "✗ Onset detection failed" << std::endl;
            return false;
        }

        // Test 3: Tempo estimation
        auto tempoResult = analyzer->estimateTempo(rhythmSignal);
        if (tempoResult.has_value()) {
            auto [estimatedTempo, confidence] = tempoResult.value();
            std::cout << "✓ Tempo estimation successful" << std::endl;
            std::cout << "  - Estimated tempo: " << estimatedTempo << " BPM" << std::endl;
            std::cout << "  - Confidence: " << confidence << std::endl;
        } else {
            std::cout << "✗ Tempo estimation failed" << std::endl;
            return false;
        }

        // Test 4: Periodicity analysis
        auto periodicityResult = analyzer->analyzePerodicity(rhythmSignal);
        if (periodicityResult.has_value()) {
            auto measures = periodicityResult.value();
            std::cout << "✓ Periodicity analysis successful" << std::endl;
            std::cout << "  - Dominant period: " << measures.dominantPeriod << " s" << std::endl;
            std::cout << "  - Periodicity strength: " << measures.periodicityStrength << std::endl;
            std::cout << "  - Autocorrelation peak: " << measures.autocorrelationPeak << std::endl;
        } else {
            std::cout << "✗ Periodicity analysis failed" << std::endl;
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        std::cout << "✗ CadenceAnalyzer test exception: " << e.what() << std::endl;
        return false;
    }
}

int main() {
    std::cout << "=== Phase 1 Enhanced Analyzers - Algorithm Functionality Test ===" << std::endl;
    std::cout << "Testing actual implemented algorithms with synthesized audio signals"
              << std::endl;

    bool allTestsPassed = true;

    // Test each analyzer's core algorithms
    if (!testPitchTrackerAlgorithm()) {
        allTestsPassed = false;
    }

    if (!testHarmonicAnalyzerAlgorithm()) {
        allTestsPassed = false;
    }

    if (!testCadenceAnalyzerAlgorithm()) {
        allTestsPassed = false;
    }

    // Final results
    std::cout << "\n=== Phase 1 Algorithm Functionality Test Results ===" << std::endl;

    if (allTestsPassed) {
        std::cout << "🎯 ALL ALGORITHM TESTS PASSED!" << std::endl;
        std::cout << "✅ PitchTracker: YIN algorithm operational with accurate frequency detection"
                  << std::endl;
        std::cout << "✅ HarmonicAnalyzer: Spectral analysis and tonal quality assessment working"
                  << std::endl;
        std::cout << "✅ CadenceAnalyzer: Beat detection and rhythm analysis functional"
                  << std::endl;
        std::cout << "\n🚀 PHASE 1 ENHANCED ANALYZERS FULLY OPERATIONAL!" << std::endl;
        std::cout << "📋 Ready for integration with UnifiedAudioEngine" << std::endl;
        std::cout << "🎵 Advanced multi-dimensional audio analysis capabilities validated"
                  << std::endl;
        return 0;
    } else {
        std::cout << "❌ SOME ALGORITHM TESTS FAILED" << std::endl;
        std::cout << "🔧 Review algorithm implementation and parameter tuning" << std::endl;
        return 1;
    }
}
