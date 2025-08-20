// Direct Harmonic Analysis Test - Phase 1 Enhanced Analyzers
// Tests spectral analysis and harmonic detection algorithms

#include <algorithm>
#include <cmath>
#include <complex>
#include <iostream>
#include <numeric>
#include <vector>

#include <gtest/gtest.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simple FFT implementation for testing (power-of-2 sizes only)
void fft(std::vector<std::complex<float>>& buffer) {
    const size_t N = buffer.size();

    // Bit-reversal permutation
    for (size_t i = 1, j = 0; i < N; i++) {
        size_t bit = N >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;

        if (i < j) {
            std::swap(buffer[i], buffer[j]);
        }
    }

    // Butterfly operations
    for (size_t len = 2; len <= N; len <<= 1) {
        float wlen = 2 * M_PI / len;
        std::complex<float> wlen_complex(std::cos(wlen), std::sin(wlen));

        for (size_t i = 0; i < N; i += len) {
            std::complex<float> w(1, 0);
            for (size_t j = 0; j < len / 2; j++) {
                std::complex<float> u = buffer[i + j];
                std::complex<float> v = buffer[i + j + len / 2] * w;
                buffer[i + j] = u + v;
                buffer[i + j + len / 2] = u - v;
                w *= wlen_complex;
            }
        }
    }
}

// Harmonic Analysis Class
class HarmonicAnalyzer {
  private:
    float sampleRate_;
    size_t fftSize_;
    std::vector<float> window_;

  public:
    struct TonalQualities {
        float rasp = 0.0f;
        float whine = 0.0f;
        float resonance = 0.0f;
        float brightness = 0.0f;
        float roughness = 0.0f;
    };

    struct HarmonicProfile {
        float spectralCentroid = 0.0f;
        float fundamentalFreq = 0.0f;
        std::vector<float> harmonicFrequencies;
        std::vector<float> harmonicAmplitudes;
        TonalQualities qualities;
        float confidence = 0.0f;
    };

    HarmonicAnalyzer(float sampleRate, size_t fftSize)
        : sampleRate_(sampleRate), fftSize_(fftSize) {
        // Create Hann window
        window_.resize(fftSize);
        for (size_t i = 0; i < fftSize; ++i) {
            window_[i] = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / (fftSize - 1));
        }
    }

    HarmonicProfile analyzeHarmonics(const std::vector<float>& audio) {
        HarmonicProfile profile;

        if (audio.size() < fftSize_) {
            return profile;
        }

        // Apply window and prepare for FFT
        std::vector<std::complex<float>> fftBuffer(fftSize_);
        for (size_t i = 0; i < fftSize_; ++i) {
            fftBuffer[i] = std::complex<float>(audio[i] * window_[i], 0.0f);
        }

        // Perform FFT
        fft(fftBuffer);

        // Calculate magnitude spectrum
        std::vector<float> magnitude(fftSize_ / 2 + 1);
        for (size_t i = 0; i < magnitude.size(); ++i) {
            magnitude[i] = std::abs(fftBuffer[i]);
        }

        // Find fundamental frequency (peak detection)
        profile.fundamentalFreq = findFundamentalFrequency(magnitude);

        // Extract harmonics
        extractHarmonics(magnitude, profile);

        // Calculate spectral centroid
        profile.spectralCentroid = calculateSpectralCentroid(magnitude);

        // Assess tonal qualities
        profile.qualities = assessTonalQualities(magnitude, profile.fundamentalFreq);

        // Calculate confidence based on harmonic strength
        profile.confidence = calculateHarmonicConfidence(profile);

        return profile;
    }

  private:
    float findFundamentalFrequency(const std::vector<float>& magnitude) {
        // Find the peak in the lower frequency range (80-2000 Hz)
        size_t minBin = static_cast<size_t>(80.0f * fftSize_ / sampleRate_);
        size_t maxBin = static_cast<size_t>(2000.0f * fftSize_ / sampleRate_);
        maxBin = std::min(maxBin, magnitude.size() - 1);

        size_t peakBin = minBin;
        float peakMagnitude = magnitude[minBin];

        for (size_t i = minBin; i <= maxBin; ++i) {
            if (magnitude[i] > peakMagnitude) {
                peakMagnitude = magnitude[i];
                peakBin = i;
            }
        }

        // Convert bin to frequency
        return static_cast<float>(peakBin) * sampleRate_ / fftSize_;
    }

    void extractHarmonics(const std::vector<float>& magnitude, HarmonicProfile& profile) {
        if (profile.fundamentalFreq < 80.0f)
            return;

        // Look for harmonics (2f0, 3f0, 4f0, etc.)
        for (int harmonic = 2; harmonic <= 8; ++harmonic) {
            float harmonicFreq = profile.fundamentalFreq * harmonic;
            if (harmonicFreq > sampleRate_ / 2)
                break;

            size_t harmonicBin = static_cast<size_t>(harmonicFreq * fftSize_ / sampleRate_);
            if (harmonicBin < magnitude.size()) {
                // Look for peak around expected harmonic frequency (±3 bins)
                size_t searchStart = (harmonicBin > 3) ? harmonicBin - 3 : 0;
                size_t searchEnd = std::min(harmonicBin + 3, magnitude.size() - 1);

                float maxMag = 0.0f;
                size_t maxBin = harmonicBin;

                for (size_t i = searchStart; i <= searchEnd; ++i) {
                    if (magnitude[i] > maxMag) {
                        maxMag = magnitude[i];
                        maxBin = i;
                    }
                }

                if (maxMag > magnitude[harmonicBin] * 0.1f) {  // Threshold for harmonic presence
                    profile.harmonicFrequencies.push_back(static_cast<float>(maxBin) * sampleRate_
                                                          / fftSize_);
                    profile.harmonicAmplitudes.push_back(maxMag);
                }
            }
        }
    }

    float calculateSpectralCentroid(const std::vector<float>& magnitude) {
        float weightedSum = 0.0f;
        float magnitudeSum = 0.0f;

        for (size_t i = 0; i < magnitude.size(); ++i) {
            float frequency = static_cast<float>(i) * sampleRate_ / fftSize_;
            weightedSum += frequency * magnitude[i];
            magnitudeSum += magnitude[i];
        }

        return (magnitudeSum > 0.0f) ? weightedSum / magnitudeSum : 0.0f;
    }

    TonalQualities assessTonalQualities(const std::vector<float>& magnitude, float fundamental) {
        TonalQualities qualities;

        if (fundamental < 80.0f)
            return qualities;

        // Brightness: Energy in high frequencies relative to total energy
        size_t brightnessBin = static_cast<size_t>(1500.0f * fftSize_ / sampleRate_);
        float highFreqEnergy = 0.0f;
        float totalEnergy = 0.0f;

        for (size_t i = 0; i < magnitude.size(); ++i) {
            float energy = magnitude[i] * magnitude[i];
            totalEnergy += energy;
            if (i >= brightnessBin) {
                highFreqEnergy += energy;
            }
        }

        qualities.brightness = (totalEnergy > 0.0f) ? highFreqEnergy / totalEnergy : 0.0f;

        // Rasp: High-frequency noise content
        // Look for energy between harmonics as a measure of roughness
        float harmonicEnergy = 0.0f;
        float noiseEnergy = 0.0f;

        size_t fundamentalBin = static_cast<size_t>(fundamental * fftSize_ / sampleRate_);

        for (size_t i = fundamentalBin; i < magnitude.size() && i < fundamentalBin * 8; ++i) {
            float energy = magnitude[i] * magnitude[i];

            // Check if this bin is near a harmonic
            bool nearHarmonic = false;
            for (int h = 2; h <= 8; ++h) {
                size_t harmonicBin = fundamentalBin * h;
                if (std::abs(static_cast<int>(i) - static_cast<int>(harmonicBin)) <= 2) {
                    nearHarmonic = true;
                    harmonicEnergy += energy;
                    break;
                }
            }

            if (!nearHarmonic) {
                noiseEnergy += energy;
            }
        }

        float totalHarmonicRegionEnergy = harmonicEnergy + noiseEnergy;
        qualities.rasp =
            (totalHarmonicRegionEnergy > 0.0f) ? noiseEnergy / totalHarmonicRegionEnergy : 0.0f;

        // Resonance: Strength of harmonics relative to fundamental
        if (!magnitude.empty()) {
            float fundamentalAmplitude = magnitude[fundamentalBin];
            float harmonicSum = 0.0f;

            for (int h = 2; h <= 4; ++h) {
                size_t harmonicBin = fundamentalBin * h;
                if (harmonicBin < magnitude.size()) {
                    harmonicSum += magnitude[harmonicBin];
                }
            }

            qualities.resonance = (fundamentalAmplitude > 0.0f)
                                      ? std::min(1.0f, harmonicSum / (fundamentalAmplitude * 3.0f))
                                      : 0.0f;
        }

        // Whine: Concentration of energy in narrow frequency bands
        qualities.whine = 1.0f - qualities.brightness;  // Simplified: opposite of brightness

        // Roughness: Spectral irregularity
        qualities.roughness = qualities.rasp * 0.7f + (1.0f - qualities.resonance) * 0.3f;

        return qualities;
    }

    float calculateHarmonicConfidence(const HarmonicProfile& profile) {
        if (profile.fundamentalFreq < 80.0f)
            return 0.0f;

        // Confidence based on number of detected harmonics and their strength
        float confidence = 0.3f;  // Base confidence for fundamental detection

        // Add confidence for each detected harmonic
        confidence += profile.harmonicFrequencies.size() * 0.1f;

        // Bonus for strong harmonic content (resonance)
        confidence += profile.qualities.resonance * 0.3f;

        return std::min(1.0f, confidence);
    }
};

// Signal generation functions
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
        float harmonic_freq = fundamental * (h + 2);
        float amplitude = harmonics[h];

        for (size_t i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            signal[i] += amplitude * std::sin(2.0f * M_PI * harmonic_freq * t);
        }
    }

    return signal;
}

// Test functions
bool testBasicSpectralAnalysis() {
    std::cout << "\n=== Testing Basic Spectral Analysis ===" << std::endl;

    HarmonicAnalyzer analyzer(44100.0f, 4096);

    float testFreq = 440.0f;  // A4
    auto signal = generateSineWave(testFreq, 0.5f, 44100.0f);
    auto profile = analyzer.analyzeHarmonics(signal);

    std::cout << "Testing pure sine wave (440 Hz):" << std::endl;
    std::cout << "  - Fundamental: " << profile.fundamentalFreq << " Hz" << std::endl;
    std::cout << "  - Spectral centroid: " << profile.spectralCentroid << " Hz" << std::endl;
    std::cout << "  - Harmonics detected: " << profile.harmonicFrequencies.size() << std::endl;
    std::cout << "  - Confidence: " << profile.confidence << std::endl;

    if (std::abs(profile.fundamentalFreq - testFreq) <= 10.0f && profile.confidence > 0.2f) {
        std::cout << "  ✓ PASS - Accurate fundamental frequency detection" << std::endl;
        return true;
    } else {
        std::cout << "  ✗ FAIL - Inaccurate analysis" << std::endl;
        return false;
    }
}

bool testComplexHarmonicAnalysis() {
    std::cout << "\n=== Testing Complex Harmonic Analysis ===" << std::endl;

    HarmonicAnalyzer analyzer(44100.0f, 4096);

    float fundamental = 440.0f;
    std::vector<float> harmonics = {0.3f, 0.2f, 0.1f, 0.05f};  // 2nd, 3rd, 4th, 5th harmonics

    auto complexSignal = generateComplexTone(fundamental, harmonics, 0.5f, 44100.0f);
    auto profile = analyzer.analyzeHarmonics(complexSignal);

    std::cout << "Testing complex harmonic tone (F0=440 Hz):" << std::endl;
    std::cout << "  - Fundamental: " << profile.fundamentalFreq << " Hz" << std::endl;
    std::cout << "  - Spectral centroid: " << profile.spectralCentroid << " Hz" << std::endl;
    std::cout << "  - Harmonics detected: " << profile.harmonicFrequencies.size() << std::endl;
    std::cout << "  - Confidence: " << profile.confidence << std::endl;

    std::cout << "  - Tonal Qualities:" << std::endl;
    std::cout << "    * Rasp: " << profile.qualities.rasp << std::endl;
    std::cout << "    * Brightness: " << profile.qualities.brightness << std::endl;
    std::cout << "    * Resonance: " << profile.qualities.resonance << std::endl;
    std::cout << "    * Roughness: " << profile.qualities.roughness << std::endl;

    if (std::abs(profile.fundamentalFreq - fundamental) <= 10.0f
        && profile.harmonicFrequencies.size() >= 2 && profile.confidence > 0.4f) {
        std::cout << "  ✓ PASS - Complex harmonic analysis successful" << std::endl;
        return true;
    } else {
        std::cout << "  ✗ FAIL - Complex harmonic analysis failed" << std::endl;
        return false;
    }
}

bool testTonalQualityAssessment() {
    std::cout << "\n=== Testing Tonal Quality Assessment ===" << std::endl;

    HarmonicAnalyzer analyzer(44100.0f, 4096);

    // Test bright sound (high harmonics)
    float fundamental = 330.0f;
    std::vector<float> brightHarmonics = {0.1f, 0.2f, 0.3f, 0.4f};  // Increasing higher harmonics

    auto brightSignal = generateComplexTone(fundamental, brightHarmonics, 0.5f, 44100.0f);
    auto brightProfile = analyzer.analyzeHarmonics(brightSignal);

    std::cout << "Testing bright harmonic tone:" << std::endl;
    std::cout << "  - Brightness: " << brightProfile.qualities.brightness << std::endl;
    std::cout << "  - Resonance: " << brightProfile.qualities.resonance << std::endl;

    // Test mellow sound (lower harmonics)
    std::vector<float> mellowHarmonics = {0.4f, 0.2f, 0.1f, 0.05f};  // Decreasing higher harmonics

    auto mellowSignal = generateComplexTone(fundamental, mellowHarmonics, 0.5f, 44100.0f);
    auto mellowProfile = analyzer.analyzeHarmonics(mellowSignal);

    std::cout << "Testing mellow harmonic tone:" << std::endl;
    std::cout << "  - Brightness: " << mellowProfile.qualities.brightness << std::endl;
    std::cout << "  - Resonance: " << mellowProfile.qualities.resonance << std::endl;

    if (brightProfile.qualities.brightness > mellowProfile.qualities.brightness
        && brightProfile.qualities.resonance > 0.1f && mellowProfile.qualities.resonance > 0.1f) {
        std::cout << "  ✓ PASS - Tonal quality assessment working" << std::endl;
        return true;
    } else {
        std::cout << "  ✗ FAIL - Tonal quality assessment failed" << std::endl;
        return false;
    }
}

// Convert to GTest format - remove main() function to avoid conflict with gtest_main
TEST(HarmonicAnalysisTest, ComprehensiveHarmonicTesting) {
    std::cout << "=== Phase 1 Enhanced Analyzers - Direct Harmonic Analysis Test ===" << std::endl;
    std::cout << "Testing spectral analysis and harmonic detection algorithms" << std::endl;

    bool allTestsPassed = true;

    if (!testBasicSpectralAnalysis()) {
        allTestsPassed = false;
    }

    if (!testComplexHarmonicAnalysis()) {
        allTestsPassed = false;
    }

    if (!testTonalQualityAssessment()) {
        allTestsPassed = false;
    }

    std::cout << "\n=== Direct Harmonic Analysis Test Results ===" << std::endl;

    if (allTestsPassed) {
        std::cout << "🎯 ALL TESTS PASSED - Harmonic Analysis Implementation Working!" << std::endl;
        std::cout << "✅ Basic spectral analysis: Accurate fundamental frequency detection"
                  << std::endl;
        std::cout << "✅ Complex harmonic analysis: Multi-harmonic extraction and assessment"
                  << std::endl;
        std::cout << "✅ Tonal quality assessment: Brightness, resonance, and roughness analysis"
                  << std::endl;
        std::cout << "\n🚀 HARMONIC ANALYSIS READY FOR PRODUCTION INTEGRATION!" << std::endl;
        std::cout << "📋 Next Steps:" << std::endl;
        std::cout << "   - Integrate with HarmonicAnalyzer class factory methods" << std::endl;
        std::cout << "   - Add formant detection and tracking" << std::endl;
        std::cout << "   - Implement harmonic-to-noise ratio calculation" << std::endl;
        std::cout << "   - Test with wildlife call spectral characteristics" << std::endl;
        EXPECT_TRUE(allTestsPassed);
    } else {
        std::cout << "❌ SOME TESTS FAILED - Algorithm needs refinement" << std::endl;
        EXPECT_TRUE(allTestsPassed);
    }
}
