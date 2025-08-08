// Direct YIN Algorithm Test - Phase 1 Enhanced Analyzers
// Tests the YIN pitch detection algorithm manually without factory methods

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simple YIN algorithm implementation for testing
class YinPitchDetector {
  private:
    float sampleRate_;
    size_t windowSize_;
    float threshold_;
    float minFrequency_;
    float maxFrequency_;

    std::vector<float> yinBuffer_;

  public:
    YinPitchDetector(float sampleRate,
                     size_t windowSize,
                     float threshold = 0.2f,
                     float minFreq = 80.0f,
                     float maxFreq = 2000.0f)
        : sampleRate_(sampleRate), windowSize_(windowSize), threshold_(threshold),
          minFrequency_(minFreq), maxFrequency_(maxFreq) {
        yinBuffer_.resize(windowSize / 2);
    }

    struct PitchResult {
        float frequency;
        float confidence;
        bool isVoiced;
    };

    PitchResult detectPitch(const std::vector<float>& audio) {
        if (audio.size() < windowSize_) {
            return {0.0f, 0.0f, false};
        }

        // Step 1: Calculate difference function
        calculateDifferenceFunction(audio);

        // Step 2: Cumulative mean normalized difference function
        calculateCumulativeMeanNormalizedDifference();

        // Step 3: Absolute threshold
        int tau = getAbsoluteThreshold();

        if (tau == -1) {
            return {0.0f, 0.0f, false};
        }

        // Step 4: Parabolic interpolation
        float betterTau = parabolicInterpolation(tau);

        // Step 5: Convert to frequency
        float frequency = sampleRate_ / betterTau;

        // Validate frequency range
        if (frequency < minFrequency_ || frequency > maxFrequency_) {
            return {0.0f, 0.0f, false};
        }

        // Calculate confidence
        float confidence = 1.0f - yinBuffer_[tau];
        confidence = std::clamp(confidence, 0.0f, 1.0f);

        return {frequency, confidence, confidence > threshold_};
    }

  private:
    void calculateDifferenceFunction(const std::vector<float>& audio) {
        size_t W = yinBuffer_.size();

        for (size_t tau = 0; tau < W; ++tau) {
            float sum = 0.0f;
            for (size_t i = 0; i < W; ++i) {
                float delta = audio[i] - audio[i + tau];
                sum += delta * delta;
            }
            yinBuffer_[tau] = sum;
        }
    }

    void calculateCumulativeMeanNormalizedDifference() {
        yinBuffer_[0] = 1.0f;

        float runningSum = 0.0f;
        for (size_t tau = 1; tau < yinBuffer_.size(); ++tau) {
            runningSum += yinBuffer_[tau];
            yinBuffer_[tau] = yinBuffer_[tau] * tau / runningSum;
        }
    }

    int getAbsoluteThreshold() {
        size_t tau_min = static_cast<size_t>(sampleRate_ / maxFrequency_);
        size_t tau_max = static_cast<size_t>(sampleRate_ / minFrequency_);
        tau_max = std::min(tau_max, yinBuffer_.size() - 1);

        for (size_t tau = tau_min; tau < tau_max; ++tau) {
            if (yinBuffer_[tau] < threshold_) {
                // Find local minimum
                while (tau + 1 < tau_max && yinBuffer_[tau + 1] < yinBuffer_[tau]) {
                    tau++;
                }
                return static_cast<int>(tau);
            }
        }

        return -1;
    }

    float parabolicInterpolation(int tau) {
        if (tau == 0 || tau >= static_cast<int>(yinBuffer_.size()) - 1) {
            return static_cast<float>(tau);
        }

        float s0 = yinBuffer_[tau - 1];
        float s1 = yinBuffer_[tau];
        float s2 = yinBuffer_[tau + 1];

        float a = (s0 - 2 * s1 + s2) / 2.0f;
        float b = (s2 - s0) / 2.0f;

        if (std::abs(a) < 1e-10f) {
            return static_cast<float>(tau);
        }

        float x0 = -b / (2 * a);
        return tau + x0;
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
        float harmonic_freq = fundamental * (h + 2);  // 2nd, 3rd, 4th harmonics etc.
        float amplitude = harmonics[h];

        for (size_t i = 0; i < numSamples; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            signal[i] += amplitude * std::sin(2.0f * M_PI * harmonic_freq * t);
        }
    }

    return signal;
}

// Test functions
bool testBasicPitchDetection() {
    std::cout << "\n=== Testing Basic YIN Pitch Detection ===" << std::endl;

    YinPitchDetector detector(44100.0f, 2048, 0.2f);

    // Test various frequencies
    std::vector<float> testFrequencies = {220.0f, 330.0f, 440.0f, 660.0f, 880.0f};
    const float tolerance = 5.0f;  // 5Hz tolerance

    for (float targetFreq : testFrequencies) {
        auto signal = generateSineWave(targetFreq, 0.5f, 44100.0f);
        auto result = detector.detectPitch(signal);

        std::cout << "Testing " << targetFreq << " Hz:" << std::endl;
        std::cout << "  - Detected: " << result.frequency << " Hz" << std::endl;
        std::cout << "  - Confidence: " << result.confidence << std::endl;
        std::cout << "  - Is voiced: " << (result.isVoiced ? "true" : "false") << std::endl;

        if (result.isVoiced && std::abs(result.frequency - targetFreq) <= tolerance) {
            std::cout << "  ✓ PASS - Accurate detection" << std::endl;
        } else {
            std::cout << "  ✗ FAIL - Inaccurate detection" << std::endl;
            return false;
        }
    }

    return true;
}

bool testHarmonicPitchDetection() {
    std::cout << "\n=== Testing Complex Harmonic Pitch Detection ===" << std::endl;

    YinPitchDetector detector(44100.0f, 2048, 0.15f);  // Lower threshold for complex tones

    float fundamental = 440.0f;                         // A4
    std::vector<float> harmonics = {0.3f, 0.2f, 0.1f};  // 2nd, 3rd, 4th harmonics

    auto complexSignal = generateComplexTone(fundamental, harmonics, 0.5f, 44100.0f);
    auto result = detector.detectPitch(complexSignal);

    std::cout << "Testing complex tone (F0=" << fundamental << " Hz):" << std::endl;
    std::cout << "  - Detected: " << result.frequency << " Hz" << std::endl;
    std::cout << "  - Confidence: " << result.confidence << std::endl;
    std::cout << "  - Is voiced: " << (result.isVoiced ? "true" : "false") << std::endl;

    if (result.isVoiced && std::abs(result.frequency - fundamental) <= 10.0f) {
        std::cout << "  ✓ PASS - Complex tone fundamental detected" << std::endl;
        return true;
    } else {
        std::cout << "  ✗ FAIL - Complex tone detection failed" << std::endl;
        return false;
    }
}

bool testNoiseRejection() {
    std::cout << "\n=== Testing Noise Rejection ===" << std::endl;

    YinPitchDetector detector(44100.0f, 2048, 0.2f);

    // Generate white noise
    std::vector<float> noise(22050);  // 0.5 seconds at 44.1kHz
    for (size_t i = 0; i < noise.size(); ++i) {
        noise[i] = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.1f;
    }

    auto result = detector.detectPitch(noise);

    std::cout << "Testing white noise:" << std::endl;
    std::cout << "  - Detected: " << result.frequency << " Hz" << std::endl;
    std::cout << "  - Confidence: " << result.confidence << std::endl;
    std::cout << "  - Is voiced: " << (result.isVoiced ? "true" : "false") << std::endl;

    if (!result.isVoiced || result.confidence < 0.3f) {
        std::cout << "  ✓ PASS - Noise correctly rejected" << std::endl;
        return true;
    } else {
        std::cout << "  ✗ FAIL - Noise incorrectly classified as pitched" << std::endl;
        return false;
    }
}

int main() {
    std::cout << "=== Phase 1 Enhanced Analyzers - Direct YIN Algorithm Test ===" << std::endl;
    std::cout << "Testing YIN pitch detection algorithm implementation" << std::endl;

    bool allTestsPassed = true;

    if (!testBasicPitchDetection()) {
        allTestsPassed = false;
    }

    if (!testHarmonicPitchDetection()) {
        allTestsPassed = false;
    }

    if (!testNoiseRejection()) {
        allTestsPassed = false;
    }

    std::cout << "\n=== Direct YIN Algorithm Test Results ===" << std::endl;

    if (allTestsPassed) {
        std::cout << "🎯 ALL TESTS PASSED - YIN Algorithm Implementation Working!" << std::endl;
        std::cout << "✅ Basic pitch detection: Accurate frequency estimation" << std::endl;
        std::cout << "✅ Complex harmonic detection: Fundamental frequency tracking" << std::endl;
        std::cout << "✅ Noise rejection: Proper unvoiced classification" << std::endl;
        std::cout << "\n🚀 YIN ALGORITHM READY FOR PRODUCTION INTEGRATION!" << std::endl;
        std::cout << "📋 Next Steps:" << std::endl;
        std::cout << "   - Integrate with PitchTracker class factory methods" << std::endl;
        std::cout << "   - Add vibrato detection and pitch smoothing" << std::endl;
        std::cout << "   - Implement real-time processing optimization" << std::endl;
        std::cout << "   - Test with wildlife call audio samples" << std::endl;
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED - Algorithm needs refinement" << std::endl;
        return 1;
    }
}
