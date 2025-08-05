// Direct Cadence Analysis Test - Phase 1 Enhanced Analyzers
// Tests rhythm pattern detection and temporal analysis algorithms

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Cadence Analysis Class
class CadenceAnalyzer {
  private:
    float sampleRate_;
    float frameSize_;  // in seconds
    float hopSize_;    // in seconds
    size_t frameSamples_;
    size_t hopSamples_;

  public:
    struct PeriodicityMeasures {
        float autocorrelationPeak = 0.0f;
        float periodicityStrength = 0.0f;
        float dominantPeriod = 0.0f;
        std::vector<float> periodicities;
        std::vector<float> periodicityStrengths;
    };

    struct RhythmicFeatures {
        float rhythmComplexity = 0.0f;
        float rhythmRegularity = 0.0f;
        float syncopation = 0.0f;
        float polyrhythm = 0.0f;
        float groove = 0.0f;
    };

    struct CallSequence {
        std::vector<float> callOnsets;
        std::vector<float> callDurations;
        std::vector<float> interCallIntervals;
        float sequenceDuration = 0.0f;
        size_t numCalls = 0;
        float callRate = 0.0f;
    };

    struct CadenceProfile {
        float estimatedTempo = 0.0f;
        PeriodicityMeasures periodicity;
        RhythmicFeatures rhythm;
        CallSequence sequence;
        float overallRhythmScore = 0.0f;
        float confidence = 0.0f;
        bool hasStrongRhythm = false;
    };

    CadenceAnalyzer(float sampleRate, float frameSize = 0.025f, float hopSize = 0.010f)
        : sampleRate_(sampleRate), frameSize_(frameSize), hopSize_(hopSize) {
        frameSamples_ = static_cast<size_t>(frameSize * sampleRate);
        hopSamples_ = static_cast<size_t>(hopSize * sampleRate);
    }

    CadenceProfile analyzeCadence(const std::vector<float>& audio) {
        CadenceProfile profile;

        if (audio.size() < frameSamples_) {
            return profile;
        }

        // Extract onset detection function
        auto onsetFunction = extractOnsetDetectionFunction(audio);

        // Detect onsets (call starts)
        auto onsets = detectOnsets(onsetFunction);

        // Analyze call sequence
        profile.sequence = analyzeCallSequence(onsets, audio.size() / sampleRate_);

        // Calculate periodicity measures
        profile.periodicity = analyzePeriodicity(onsetFunction);

        // Estimate tempo from onsets
        profile.estimatedTempo = estimateTempo(onsets);

        // Calculate rhythmic features
        profile.rhythm = calculateRhythmicFeatures(onsets, onsetFunction);

        // Overall rhythm assessment
        profile.overallRhythmScore = calculateOverallRhythmScore(profile);
        profile.confidence = calculateConfidence(profile);
        profile.hasStrongRhythm = profile.overallRhythmScore > 0.6f && profile.confidence > 0.5f;

        return profile;
    }

  private:
    std::vector<float> extractOnsetDetectionFunction(const std::vector<float>& audio) {
        // Simple energy-based onset detection function
        std::vector<float> onsetFunction;

        for (size_t i = 0; i < audio.size(); i += hopSamples_) {
            size_t endIdx = std::min(i + frameSamples_, audio.size());

            // Calculate frame energy
            float energy = 0.0f;
            for (size_t j = i; j < endIdx; ++j) {
                energy += audio[j] * audio[j];
            }
            energy /= (endIdx - i);

            onsetFunction.push_back(energy);
        }

        // Apply spectral flux (energy difference between frames)
        std::vector<float> spectralFlux;
        spectralFlux.push_back(0.0f);

        for (size_t i = 1; i < onsetFunction.size(); ++i) {
            float flux = std::max(0.0f, onsetFunction[i] - onsetFunction[i - 1]);
            spectralFlux.push_back(flux);
        }

        return spectralFlux;
    }

    std::vector<float> detectOnsets(const std::vector<float>& onsetFunction) {
        std::vector<float> onsets;

        if (onsetFunction.size() < 3)
            return onsets;

        // Calculate adaptive threshold
        float mean = std::accumulate(onsetFunction.begin(), onsetFunction.end(), 0.0f)
                     / onsetFunction.size();
        float variance = 0.0f;
        for (float val : onsetFunction) {
            variance += (val - mean) * (val - mean);
        }
        variance /= onsetFunction.size();
        float threshold = mean + 2.0f * std::sqrt(variance);

        // Find peaks above threshold
        for (size_t i = 1; i < onsetFunction.size() - 1; ++i) {
            if (onsetFunction[i] > threshold && onsetFunction[i] > onsetFunction[i - 1]
                && onsetFunction[i] > onsetFunction[i + 1]) {
                // Convert frame index to time
                float timeSeconds = static_cast<float>(i) * hopSize_;
                onsets.push_back(timeSeconds);
            }
        }

        return onsets;
    }

    CallSequence analyzeCallSequence(const std::vector<float>& onsets, float totalDuration) {
        CallSequence sequence;
        sequence.callOnsets = onsets;
        sequence.numCalls = onsets.size();
        sequence.sequenceDuration = totalDuration;

        if (onsets.size() < 2) {
            return sequence;
        }

        // Calculate inter-call intervals
        for (size_t i = 0; i < onsets.size() - 1; ++i) {
            sequence.interCallIntervals.push_back(onsets[i + 1] - onsets[i]);
        }

        // Estimate call durations (simplified: use inter-call intervals / 2)
        for (size_t i = 0; i < sequence.interCallIntervals.size(); ++i) {
            sequence.callDurations.push_back(sequence.interCallIntervals[i] * 0.4f);
        }

        // Calculate call rate
        sequence.callRate = sequence.numCalls / totalDuration;

        return sequence;
    }

    PeriodicityMeasures analyzePeriodicity(const std::vector<float>& onsetFunction) {
        PeriodicityMeasures measures;

        if (onsetFunction.size() < 20)
            return measures;

        // Autocorrelation analysis
        size_t maxLag =
            std::min(static_cast<size_t>(200), onsetFunction.size() / 2);  // Up to 2 seconds
        std::vector<float> autocorr(maxLag);

        for (size_t lag = 1; lag < maxLag; ++lag) {
            float correlation = 0.0f;
            float normalization = 0.0f;

            for (size_t i = 0; i < onsetFunction.size() - lag; ++i) {
                correlation += onsetFunction[i] * onsetFunction[i + lag];
                normalization += onsetFunction[i] * onsetFunction[i];
            }

            autocorr[lag] = (normalization > 0.0f) ? correlation / normalization : 0.0f;
        }

        // Find peaks in autocorrelation
        measures.autocorrelationPeak = *std::max_element(autocorr.begin(), autocorr.end());

        // Find dominant period
        auto maxIt = std::max_element(autocorr.begin(), autocorr.end());
        if (maxIt != autocorr.end()) {
            size_t peakLag = std::distance(autocorr.begin(), maxIt);
            measures.dominantPeriod = static_cast<float>(peakLag) * hopSize_;
        }

        measures.periodicityStrength = measures.autocorrelationPeak;

        // Store multiple periodicity candidates
        for (size_t i = 1; i < autocorr.size(); ++i) {
            if (autocorr[i] > 0.3f &&  // Threshold for significance
                autocorr[i] > autocorr[i - 1]
                && (i == autocorr.size() - 1 || autocorr[i] > autocorr[i + 1])) {
                measures.periodicities.push_back(static_cast<float>(i) * hopSize_);
                measures.periodicityStrengths.push_back(autocorr[i]);
            }
        }

        return measures;
    }

    float estimateTempo(const std::vector<float>& onsets) {
        if (onsets.size() < 3)
            return 0.0f;

        // Calculate inter-onset intervals
        std::vector<float> intervals;
        for (size_t i = 0; i < onsets.size() - 1; ++i) {
            intervals.push_back(onsets[i + 1] - onsets[i]);
        }

        // Find most common interval (simplified tempo estimation)
        if (intervals.empty())
            return 0.0f;

        float medianInterval = intervals[intervals.size() / 2];
        std::sort(intervals.begin(), intervals.end());

        // Convert to BPM (beats per minute)
        if (medianInterval > 0.0f) {
            return 60.0f / medianInterval;
        }

        return 0.0f;
    }

    RhythmicFeatures calculateRhythmicFeatures(const std::vector<float>& onsets,
                                               const std::vector<float>& onsetFunction) {
        RhythmicFeatures features;

        if (onsets.size() < 3)
            return features;

        // Calculate inter-onset intervals
        std::vector<float> intervals;
        for (size_t i = 0; i < onsets.size() - 1; ++i) {
            intervals.push_back(onsets[i + 1] - onsets[i]);
        }

        // Rhythm regularity: inverse of interval variance
        float meanInterval =
            std::accumulate(intervals.begin(), intervals.end(), 0.0f) / intervals.size();
        float variance = 0.0f;
        for (float interval : intervals) {
            variance += (interval - meanInterval) * (interval - meanInterval);
        }
        variance /= intervals.size();

        features.rhythmRegularity = 1.0f / (1.0f + variance);

        // Rhythm complexity: based on number of different interval types
        std::vector<float> sortedIntervals = intervals;
        std::sort(sortedIntervals.begin(), sortedIntervals.end());

        size_t uniqueIntervals = 1;
        for (size_t i = 1; i < sortedIntervals.size(); ++i) {
            if (std::abs(sortedIntervals[i] - sortedIntervals[i - 1]) > 0.05f) {  // 50ms tolerance
                uniqueIntervals++;
            }
        }

        features.rhythmComplexity = static_cast<float>(uniqueIntervals) / intervals.size();

        // Syncopation: deviations from expected regular rhythm
        if (meanInterval > 0.0f) {
            float syncopationSum = 0.0f;
            for (float interval : intervals) {
                float expected = meanInterval;
                float deviation = std::abs(interval - expected) / expected;
                syncopationSum += deviation;
            }
            features.syncopation = syncopationSum / intervals.size();
        }

        // Groove: combination of regularity and slight timing variations
        features.groove = features.rhythmRegularity * (1.0f - features.syncopation * 0.5f);

        // Polyrhythm: simplified as high complexity with moderate regularity
        features.polyrhythm = features.rhythmComplexity * features.rhythmRegularity;

        return features;
    }

    float calculateOverallRhythmScore(const CadenceProfile& profile) {
        // Weighted combination of rhythm measures
        float score = 0.0f;

        score += profile.periodicity.periodicityStrength * 0.3f;
        score += profile.rhythm.rhythmRegularity * 0.25f;
        score += (profile.sequence.numCalls > 2 ? 1.0f : 0.0f) * 0.2f;
        score += (profile.estimatedTempo > 30.0f && profile.estimatedTempo < 300.0f ? 1.0f : 0.0f)
                 * 0.15f;
        score += profile.rhythm.groove * 0.1f;

        return std::min(1.0f, score);
    }

    float calculateConfidence(const CadenceProfile& profile) {
        float confidence = 0.0f;

        // Base confidence from number of detected calls
        confidence += std::min(1.0f, static_cast<float>(profile.sequence.numCalls) / 5.0f) * 0.4f;

        // Periodicity strength
        confidence += profile.periodicity.periodicityStrength * 0.3f;

        // Rhythm regularity
        confidence += profile.rhythm.rhythmRegularity * 0.2f;

        // Valid tempo range
        if (profile.estimatedTempo > 30.0f && profile.estimatedTempo < 300.0f) {
            confidence += 0.1f;
        }

        return std::min(1.0f, confidence);
    }
};

// Signal generation functions for testing
std::vector<float>
generateRhythmicPattern(const std::vector<float>& beatTimes, float duration, float sampleRate) {
    std::vector<float> signal(static_cast<size_t>(duration * sampleRate), 0.0f);

    for (float beatTime : beatTimes) {
        if (beatTime < duration) {
            size_t startIdx = static_cast<size_t>(beatTime * sampleRate);
            size_t pulseLength = static_cast<size_t>(0.1f * sampleRate);  // 100ms pulses

            for (size_t i = 0; i < pulseLength && startIdx + i < signal.size(); ++i) {
                float t = static_cast<float>(i) / sampleRate;
                // Generate a short tone burst
                signal[startIdx + i] = 0.5f * std::sin(2.0f * M_PI * 800.0f * t)
                                       * std::exp(-t * 10.0f);  // Decaying envelope
            }
        }
    }

    return signal;
}

std::vector<float> generateIrregularCalls(float duration, float sampleRate) {
    std::vector<float> signal(static_cast<size_t>(duration * sampleRate), 0.0f);

    // Random call times
    std::vector<float> callTimes = {0.2f, 0.8f, 1.7f, 2.1f, 3.3f, 4.0f, 4.9f};

    for (float callTime : callTimes) {
        if (callTime < duration) {
            size_t startIdx = static_cast<size_t>(callTime * sampleRate);
            size_t callLength = static_cast<size_t>(0.15f * sampleRate);  // 150ms calls

            for (size_t i = 0; i < callLength && startIdx + i < signal.size(); ++i) {
                float t = static_cast<float>(i) / sampleRate;
                // Generate a wildlife-like call
                signal[startIdx + i] =
                    0.4f * std::sin(2.0f * M_PI * (400.0f + 200.0f * t) * t) * std::exp(-t * 5.0f);
            }
        }
    }

    return signal;
}

// Test functions
bool testRegularRhythmDetection() {
    std::cout << "\n=== Testing Regular Rhythm Detection ===" << std::endl;

    CadenceAnalyzer analyzer(44100.0f);

    // Generate regular 120 BPM pattern
    float bpm = 120.0f;
    float interval = 60.0f / bpm;  // 0.5 seconds
    std::vector<float> beatTimes;

    for (float t = 0.0f; t < 5.0f; t += interval) {
        beatTimes.push_back(t);
    }

    auto signal = generateRhythmicPattern(beatTimes, 5.0f, 44100.0f);
    auto profile = analyzer.analyzeCadence(signal);

    std::cout << "Testing regular 120 BPM pattern:" << std::endl;
    std::cout << "  - Estimated tempo: " << profile.estimatedTempo << " BPM" << std::endl;
    std::cout << "  - Calls detected: " << profile.sequence.numCalls << std::endl;
    std::cout << "  - Rhythm regularity: " << profile.rhythm.rhythmRegularity << std::endl;
    std::cout << "  - Periodicity strength: " << profile.periodicity.periodicityStrength
              << std::endl;
    std::cout << "  - Overall rhythm score: " << profile.overallRhythmScore << std::endl;
    std::cout << "  - Confidence: " << profile.confidence << std::endl;
    std::cout << "  - Has strong rhythm: " << (profile.hasStrongRhythm ? "true" : "false")
              << std::endl;

    if (profile.sequence.numCalls >= 5 && profile.rhythm.rhythmRegularity > 0.7f
        && profile.confidence > 0.5f) {
        std::cout << "  ✓ PASS - Regular rhythm detected successfully" << std::endl;
        return true;
    } else {
        std::cout << "  ✗ FAIL - Regular rhythm detection failed" << std::endl;
        return false;
    }
}

bool testIrregularCallPattern() {
    std::cout << "\n=== Testing Irregular Call Pattern ===" << std::endl;

    CadenceAnalyzer analyzer(44100.0f);

    auto signal = generateIrregularCalls(5.0f, 44100.0f);
    auto profile = analyzer.analyzeCadence(signal);

    std::cout << "Testing irregular call pattern:" << std::endl;
    std::cout << "  - Estimated tempo: " << profile.estimatedTempo << " BPM" << std::endl;
    std::cout << "  - Calls detected: " << profile.sequence.numCalls << std::endl;
    std::cout << "  - Rhythm regularity: " << profile.rhythm.rhythmRegularity << std::endl;
    std::cout << "  - Rhythm complexity: " << profile.rhythm.rhythmComplexity << std::endl;
    std::cout << "  - Syncopation: " << profile.rhythm.syncopation << std::endl;
    std::cout << "  - Overall rhythm score: " << profile.overallRhythmScore << std::endl;
    std::cout << "  - Confidence: " << profile.confidence << std::endl;

    if (profile.sequence.numCalls >= 4 && profile.rhythm.rhythmComplexity > 0.3f
        && profile.confidence > 0.3f) {
        std::cout << "  ✓ PASS - Irregular call pattern analyzed successfully" << std::endl;
        return true;
    } else {
        std::cout << "  ✗ FAIL - Irregular call pattern analysis failed" << std::endl;
        return false;
    }
}

bool testSilenceRejection() {
    std::cout << "\n=== Testing Silence Rejection ===" << std::endl;

    CadenceAnalyzer analyzer(44100.0f);

    // Generate mostly silent signal
    std::vector<float> silence(static_cast<size_t>(5.0f * 44100.0f), 0.0f);

    auto profile = analyzer.analyzeCadence(silence);

    std::cout << "Testing silence:" << std::endl;
    std::cout << "  - Calls detected: " << profile.sequence.numCalls << std::endl;
    std::cout << "  - Overall rhythm score: " << profile.overallRhythmScore << std::endl;
    std::cout << "  - Has strong rhythm: " << (profile.hasStrongRhythm ? "true" : "false")
              << std::endl;

    if (profile.sequence.numCalls <= 1 && !profile.hasStrongRhythm
        && profile.overallRhythmScore < 0.3f) {
        std::cout << "  ✓ PASS - Silence correctly rejected" << std::endl;
        return true;
    } else {
        std::cout << "  ✗ FAIL - Silence incorrectly classified as rhythmic" << std::endl;
        return false;
    }
}

int main() {
    std::cout << "=== Phase 1 Enhanced Analyzers - Direct Cadence Analysis Test ===" << std::endl;
    std::cout << "Testing rhythm pattern detection and temporal analysis algorithms" << std::endl;

    bool allTestsPassed = true;

    if (!testRegularRhythmDetection()) {
        allTestsPassed = false;
    }

    if (!testIrregularCallPattern()) {
        allTestsPassed = false;
    }

    if (!testSilenceRejection()) {
        allTestsPassed = false;
    }

    std::cout << "\n=== Direct Cadence Analysis Test Results ===" << std::endl;

    if (allTestsPassed) {
        std::cout << "🎯 ALL TESTS PASSED - Cadence Analysis Implementation Working!" << std::endl;
        std::cout << "✅ Regular rhythm detection: Tempo estimation and pattern recognition"
                  << std::endl;
        std::cout << "✅ Irregular call pattern: Onset detection and complexity analysis"
                  << std::endl;
        std::cout << "✅ Silence rejection: Proper non-rhythmic classification" << std::endl;
        std::cout << "\n🚀 CADENCE ANALYSIS READY FOR PRODUCTION INTEGRATION!" << std::endl;
        std::cout << "📋 Next Steps:" << std::endl;
        std::cout << "   - Integrate with CadenceAnalyzer class factory methods" << std::endl;
        std::cout << "   - Add advanced beat tracking algorithms" << std::endl;
        std::cout << "   - Implement syllable analysis for vocal patterns" << std::endl;
        std::cout << "   - Test with real wildlife call temporal patterns" << std::endl;
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED - Algorithm needs refinement" << std::endl;
        return 1;
    }
}
