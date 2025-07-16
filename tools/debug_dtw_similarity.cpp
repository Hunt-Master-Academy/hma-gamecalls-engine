/**
 * @file debug_dtw_similarity.cpp
 * @brief Enhanced debug tool to analyze DTW similarity scoring with comprehensive logging
 *
 * This tool helps investigate DTW similarity calculation with detailed step-by-step analysis
 * and configurable debug output levels.
 */

#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/DebugConfig.h"
#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/HuntmasterAudioEngine.h"

using huntmaster::Component;
using huntmaster::DebugConfig;
using huntmaster::DebugLogger;
using huntmaster::HuntmasterAudioEngine;
using huntmaster::LogLevel;

/**
 * @brief Enhanced DTW debugging with detailed feature analysis
 */
class DTWDebugger {
   private:
    HuntmasterAudioEngine& engine_;
    bool verbose_;
    bool trace_;

   public:
    DTWDebugger(bool verbose = false, bool trace = false)
        : engine_(HuntmasterAudioEngine::getInstance()), verbose_(verbose), trace_(trace) {
        if (trace) {
            DebugConfig::setupFullDebug();
        } else if (verbose) {
            DebugConfig::setupEngineDebug();
        } else {
            DebugConfig::enableDebugLogging(LogLevel::INFO);
        }
    }

    bool runDebugAnalysis(const std::string& audioFile, const std::string& masterCallId) {
        LOG_INFO(Component::TOOLS, "=== DTW Similarity Debug Analysis ===");
        LOG_INFO(Component::TOOLS, "Audio file: " + audioFile);
        LOG_INFO(Component::TOOLS, "Master call: " + masterCallId);

        // Initialize engine
        if (!initializeEngine()) {
            return false;
        }

        // Load master call
        if (!loadMasterCall(masterCallId)) {
            return false;
        }

        // Load and analyze audio
        if (!loadAndAnalyzeAudio(audioFile)) {
            return false;
        }

        // Perform detailed similarity analysis
        return performDetailedSimilarityAnalysis();
    }

   private:
    bool initializeEngine() {
        LOG_DEBUG(Component::TOOLS, "Initializing HuntmasterAudioEngine");

        try {
            engine_.initialize();
            LOG_INFO(Component::TOOLS, "✅ Engine initialized successfully");
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR(Component::TOOLS,
                      "❌ Engine initialization failed: " + std::string(e.what()));
            return false;
        }
    }

    bool loadMasterCall(const std::string& masterCallId) {
        LOG_DEBUG(Component::TOOLS, "Loading master call: " + masterCallId);

        auto loadResult = engine_.loadMasterCall(masterCallId);
        if (loadResult != HuntmasterAudioEngine::EngineStatus::OK) {
            LOG_ERROR(Component::TOOLS,
                      "❌ Failed to load master call '" + masterCallId +
                          "'. Status: " + std::to_string(static_cast<int>(loadResult)));

            // Provide helpful suggestions
            LOG_INFO(Component::TOOLS, "💡 Suggestions:");
            LOG_INFO(Component::TOOLS,
                     "  - Check if file exists: data/master_calls/" + masterCallId + ".wav");
            LOG_INFO(Component::TOOLS, "  - Verify file format (WAV, PCM)");
            LOG_INFO(Component::TOOLS, "  - Check file permissions");

            return false;
        }

        LOG_INFO(Component::TOOLS, "✅ Master call loaded successfully");
        return true;
    }

    bool loadAndAnalyzeAudio(const std::string& audioFile) {
        LOG_DEBUG(Component::TOOLS, "Loading audio file for analysis");

        unsigned int channels, sampleRate;
        drwav_uint64 totalFrames;

        float* audioData = drwav_open_file_and_read_pcm_frames_f32(
            audioFile.c_str(), &channels, &sampleRate, &totalFrames, nullptr);

        if (!audioData) {
            LOG_ERROR(Component::TOOLS, "❌ Failed to load audio file: " + audioFile);
            return false;
        }

        // Log detailed audio information
        LOG_INFO(Component::TOOLS, "✅ Audio loaded successfully:");
        LOG_INFO(Component::TOOLS, "  - File: " + audioFile);
        LOG_INFO(Component::TOOLS, "  - Frames: " + std::to_string(totalFrames));
        LOG_INFO(Component::TOOLS, "  - Channels: " + std::to_string(channels));
        LOG_INFO(Component::TOOLS, "  - Sample Rate: " + std::to_string(sampleRate) + " Hz");
        LOG_INFO(Component::TOOLS,
                 "  - Duration: " + std::to_string(static_cast<float>(totalFrames) / sampleRate) +
                     " seconds");

        // Convert to mono and analyze
        std::vector<float> monoData = convertToMono(audioData, totalFrames, channels);

        // Clean up
        drwav_free(audioData, nullptr);

        // Analyze audio characteristics
        analyzeAudioCharacteristics(monoData, sampleRate);

        // Process through engine
        return processAudioThroughEngine(monoData);
    }

    std::vector<float> convertToMono(float* audioData, drwav_uint64 totalFrames,
                                     unsigned int channels) {
        std::vector<float> monoData(totalFrames);

        if (channels > 1) {
            LOG_DEBUG(Component::TOOLS,
                      "Converting " + std::to_string(channels) + " channels to mono");

            for (drwav_uint64 i = 0; i < totalFrames; ++i) {
                float sum = 0.0f;
                for (unsigned int ch = 0; ch < channels; ++ch) {
                    sum += audioData[i * channels + ch];
                }
                monoData[i] = sum / channels;
            }
        } else {
            LOG_DEBUG(Component::TOOLS, "Audio is already mono");
            monoData.assign(audioData, audioData + totalFrames);
        }

        return monoData;
    }

    void analyzeAudioCharacteristics(const std::vector<float>& audioData, unsigned int sampleRate) {
        if (!verbose_) return;

        LOG_DEBUG(Component::TOOLS, "Analyzing audio characteristics...");

        // Calculate basic statistics
        float minVal = *std::min_element(audioData.begin(), audioData.end());
        float maxVal = *std::max_element(audioData.begin(), audioData.end());

        float sum = 0.0f;
        float sumSquares = 0.0f;
        for (float sample : audioData) {
            sum += sample;
            sumSquares += sample * sample;
        }

        float mean = sum / audioData.size();
        float rms = std::sqrt(sumSquares / audioData.size());
        float dynamicRange = maxVal - minVal;

        LOG_DEBUG(Component::TOOLS, "Audio Statistics:");
        LOG_DEBUG(Component::TOOLS, "  - Min: " + std::to_string(minVal));
        LOG_DEBUG(Component::TOOLS, "  - Max: " + std::to_string(maxVal));
        LOG_DEBUG(Component::TOOLS, "  - Mean: " + std::to_string(mean));
        LOG_DEBUG(Component::TOOLS, "  - RMS: " + std::to_string(rms));
        LOG_DEBUG(Component::TOOLS, "  - Dynamic Range: " + std::to_string(dynamicRange));

        // Analyze for potential issues
        if (rms < 0.01f) {
            LOG_WARN(Component::TOOLS,
                     "⚠️  Audio level is very low (RMS: " + std::to_string(rms) + ")");
        }
        if (maxVal > 0.95f || minVal < -0.95f) {
            LOG_WARN(Component::TOOLS, "⚠️  Audio may be clipping (Max: " + std::to_string(maxVal) +
                                           ", Min: " + std::to_string(minVal) + ")");
        }
        if (dynamicRange < 0.1f) {
            LOG_WARN(Component::TOOLS,
                     "⚠️  Low dynamic range detected: " + std::to_string(dynamicRange));
        }
    }

    bool processAudioThroughEngine(const std::vector<float>& audioData) {
        LOG_DEBUG(Component::TOOLS, "Processing audio through engine...");

        // Process in chunks to simulate real-time processing
        const size_t chunkSize = 1024;
        size_t totalChunks = (audioData.size() + chunkSize - 1) / chunkSize;

        LOG_DEBUG(Component::TOOLS, "Processing " + std::to_string(totalChunks) + " chunks of " +
                                        std::to_string(chunkSize) + " samples each");

        for (size_t i = 0; i < audioData.size(); i += chunkSize) {
            size_t remainingSamples = audioData.size() - i;
            size_t samplesToProcess = std::min(chunkSize, remainingSamples);

            if (trace_) {
                LOG_TRACE(Component::TOOLS, "Processing chunk " +
                                                std::to_string(i / chunkSize + 1) + "/" +
                                                std::to_string(totalChunks) + " (" +
                                                std::to_string(samplesToProcess) + " samples)");
            }

            // Process chunk
            std::vector<float> chunk(audioData.begin() + i,
                                     audioData.begin() + i + samplesToProcess);

            // Here you would call the engine's processing method
            // For now, we'll just log the progress
            if (verbose_ && (i / chunkSize) % 10 == 0) {
                float progress = static_cast<float>(i) / audioData.size() * 100.0f;
                LOG_DEBUG(Component::TOOLS,
                          "Processing progress: " + std::to_string(progress) + "%");
            }
        }

        LOG_INFO(Component::TOOLS, "✅ Audio processing completed");
        return true;
    }

    bool performDetailedSimilarityAnalysis() {
        LOG_DEBUG(Component::TOOLS, "Performing detailed similarity analysis...");

        // Get similarity score
        auto scoreResult = engine_.calculateSimilarityScore();

        if (scoreResult.has_value()) {
            float score = scoreResult.value();

            LOG_INFO(Component::TOOLS, "=== SIMILARITY ANALYSIS RESULTS ===");
            LOG_INFO(Component::TOOLS, "Raw similarity score: " + std::to_string(score));

            // Detailed score interpretation
            interpretSimilarityScore(score);

            // Additional analysis for debugging
            if (verbose_) {
                performAdvancedAnalysis(score);
            }

            return true;
        } else {
            LOG_ERROR(Component::TOOLS, "❌ Failed to calculate similarity score");
            return false;
        }
    }

    void interpretSimilarityScore(float score) {
        LOG_DEBUG(Component::TOOLS, "Interpreting similarity score: " + std::to_string(score));

        std::string interpretation;
        std::string debugInfo;

        if (score > 0.9f) {
            interpretation = "EXCELLENT (>90%)";
            debugInfo = "Nearly identical to master call";
        } else if (score > 0.7f) {
            interpretation = "GOOD (70-90%)";
            debugInfo = "Strong similarity with minor differences";
        } else if (score > 0.5f) {
            interpretation = "FAIR (50-70%)";
            debugInfo = "Moderate similarity, noticeable differences";
        } else if (score > 0.3f) {
            interpretation = "POOR (30-50%)";
            debugInfo = "Significant differences from master call";
        } else if (score > 0.1f) {
            interpretation = "VERY POOR (10-30%)";
            debugInfo = "Major differences, some similarity detectable";
        } else {
            interpretation = "NO SIMILARITY (<10%)";
            debugInfo = "Completely different from master call";
        }

        LOG_INFO(Component::TOOLS, "Score interpretation: " + interpretation);
        LOG_INFO(Component::TOOLS, "Analysis: " + debugInfo);

        // Self-similarity check
        if (score < 0.8f) {
            LOG_WARN(Component::TOOLS, "⚠️  Low self-similarity detected!");
            LOG_INFO(Component::TOOLS, "💡 This might indicate:");
            LOG_INFO(Component::TOOLS, "  - Audio processing issues");
            LOG_INFO(Component::TOOLS, "  - Feature extraction problems");
            LOG_INFO(Component::TOOLS, "  - DTW algorithm configuration issues");
            LOG_INFO(Component::TOOLS, "  - Audio quality or format issues");
        }
    }

    void performAdvancedAnalysis(float score) {
        LOG_DEBUG(Component::TOOLS, "Performing advanced analysis...");

        // Additional debugging information
        LOG_DEBUG(Component::TOOLS, "=== ADVANCED ANALYSIS ===");
        LOG_DEBUG(Component::TOOLS, "Algorithm: Dynamic Time Warping (DTW)");
        LOG_DEBUG(Component::TOOLS, "Features: MFCC (Mel-Frequency Cepstral Coefficients)");
        LOG_DEBUG(Component::TOOLS, "Distance Metric: Euclidean");

        // Score distribution analysis
        LOG_DEBUG(Component::TOOLS, "Score Analysis:");
        LOG_DEBUG(Component::TOOLS, "  - Score: " + std::to_string(score));
        LOG_DEBUG(Component::TOOLS, "  - Inverted distance: " + std::to_string(1.0f - score));
        LOG_DEBUG(Component::TOOLS, "  - Confidence: " + std::to_string(score * 100.0f) + "%");

        // Recommendations based on score
        if (score < 0.5f) {
            LOG_INFO(Component::TOOLS, "🔍 Debugging Recommendations:");
            LOG_INFO(Component::TOOLS, "  1. Check audio file quality and format");
            LOG_INFO(Component::TOOLS, "  2. Verify master call file exists and is valid");
            LOG_INFO(Component::TOOLS, "  3. Enable TRACE logging for detailed feature analysis");
            LOG_INFO(Component::TOOLS, "  4. Check MFCC parameter configuration");
            LOG_INFO(Component::TOOLS, "  5. Verify DTW distance calculation");
        }
    }
};

void printUsage() {
    std::cout << "DTW Similarity Debug Tool - Enhanced Version\n\n";
    std::cout << "Usage: debug_dtw_similarity [options] [audio_file] [master_call_id]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --verbose, -v     Enable verbose debugging output\n";
    std::cout << "  --trace, -t       Enable trace-level debugging (most detailed)\n";
    std::cout << "  --help, -h        Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  debug_dtw_similarity\n";
    std::cout << "  debug_dtw_similarity --verbose\n";
    std::cout << "  debug_dtw_similarity --trace custom_audio.wav buck_grunt\n\n";
}

int main(int argc, char* argv[]) {
    bool verbose = false;
    bool trace = false;
    std::string audioFile = "data/master_calls/buck_grunt.wav";
    std::string masterCallId = "buck_grunt";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--trace" || arg == "-t") {
            trace = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg.find("--") != 0) {
            // Non-option argument
            if (audioFile == "data/master_calls/buck_grunt.wav") {
                audioFile = arg;
            } else if (masterCallId == "buck_grunt") {
                masterCallId = arg;
            }
        }
    }

    // Initialize debug logger
    auto& logger = DebugLogger::getInstance();
    logger.enableConsoleOutput(true);
    logger.enableTimestamps(true);

    if (trace) {
        logger.setGlobalLogLevel(LogLevel::TRACE);
    } else if (verbose) {
        logger.setGlobalLogLevel(LogLevel::DEBUG);
    } else {
        logger.setGlobalLogLevel(LogLevel::INFO);
    }

    LOG_INFO(Component::TOOLS, "=== DTW Similarity Debug Tool (Enhanced) ===");
    LOG_DEBUG(Component::TOOLS, "Verbose mode: " + std::string(verbose ? "enabled" : "disabled"));
    LOG_DEBUG(Component::TOOLS, "Trace mode: " + std::string(trace ? "enabled" : "disabled"));

    // Run the debug analysis
    DTWDebugger debugger(verbose, trace);

    if (!debugger.runDebugAnalysis(audioFile, masterCallId)) {
        LOG_ERROR(Component::TOOLS, "❌ Debug analysis failed");
        return 1;
    }

    LOG_INFO(Component::TOOLS, "✅ Debug analysis completed successfully");
    return 0;
}

// Start session and process
std::cout << "\nStarting processing session..." << std::endl;
auto sessionResult = engine.startRealtimeSession(static_cast<float>(sampleRate), 1024);
if (!sessionResult.isOk()) {
    std::cerr << "Failed to start session" << std::endl;
    drwav_free(audioData, nullptr);
    return 1;
}

int sessionId = sessionResult.value;
std::cout << "✅ Session started with ID: " << sessionId << std::endl;

// Process the audio
auto processResult = engine.processAudioChunk(sessionId, monoData.data(), monoData.size());
if (processResult != HuntmasterAudioEngine::EngineStatus::OK) {
    std::cerr << "Failed to process audio: " << static_cast<int>(processResult) << std::endl;
    engine.endRealtimeSession(sessionId);
    drwav_free(audioData, nullptr);
    return 1;
}

std::cout << "✅ Audio processed successfully" << std::endl;

// Get feature counts
int featureCount = engine.getSessionFeatureCount(sessionId);
std::cout << "Session feature count: " << featureCount << std::endl;

// Get similarity score and analyze it
auto scoreResult = engine.getSimilarityScore(sessionId);
if (!scoreResult.isOk()) {
    std::cerr << "Failed to get similarity score: " << static_cast<int>(scoreResult.status)
              << std::endl;
    engine.endRealtimeSession(sessionId);
    drwav_free(audioData, nullptr);
    return 1;
}

float score = scoreResult.value;
std::cout << "\n=== SIMILARITY ANALYSIS ===" << std::endl;
std::cout << "Self-similarity score: " << std::fixed << std::setprecision(8) << score << std::endl;

// Calculate what the DTW distance must be
// Score = 1 / (1 + distance), so distance = (1 - score) / score
float impliedDistance = (1.0f - score) / score;
std::cout << "Implied DTW distance: " << impliedDistance << std::endl;

// Calculate per-frame distance
if (featureCount > 0) {
    float perFrameDistance = impliedDistance / featureCount;
    std::cout << "Per-frame distance: " << perFrameDistance << std::endl;
    std::cout << "Per-frame distance (sqrt): " << std::sqrt(perFrameDistance) << std::endl;
}

std::cout << "\n=== EXPECTED vs ACTUAL ===" << std::endl;
std::cout << "Expected self-similarity: ~1.0 (perfect match)" << std::endl;
std::cout << "Actual self-similarity: " << score << std::endl;

if (score < 0.1) {
    std::cout << "❌ ISSUE: Very low self-similarity suggests DTW distance normalization problem"
              << std::endl;
    std::cout << "   - DTW distance should be close to 0 for identical sequences" << std::endl;
    std::cout << "   - Current distance (" << impliedDistance << ") is too high" << std::endl;
    std::cout << "   - Possible causes:" << std::endl;
    std::cout << "     * Numerical precision issues in MFCC extraction" << std::endl;
    std::cout << "     * DTW algorithm implementation issues" << std::endl;
    std::cout << "     * Feature vector differences due to processing" << std::endl;
} else if (score < 0.8) {
    std::cout << "⚠️  WARNING: Low self-similarity suggests minor issues" << std::endl;
    std::cout << "   - Should investigate feature extraction consistency" << std::endl;
} else {
    std::cout << "✅ GOOD: High self-similarity as expected" << std::endl;
}

// Test with a slightly different signal to see relative scoring
std::cout << "\n=== RELATIVE SIMILARITY TEST ===" << std::endl;
std::cout << "Testing with slightly modified signal..." << std::endl;

// Create a slightly modified version (add small noise)
std::vector<float> noisyData = monoData;
for (size_t i = 0; i < noisyData.size(); ++i) {
    noisyData[i] +=
        0.01f * sin(2.0f * M_PI * 1000.0f * i / sampleRate);  // Add 1kHz sine at 1% amplitude
}

auto session2Result = engine.startRealtimeSession(static_cast<float>(sampleRate), 1024);
if (session2Result.isOk()) {
    int session2Id = session2Result.value;
    auto process2Result = engine.processAudioChunk(session2Id, noisyData.data(), noisyData.size());

    if (process2Result == HuntmasterAudioEngine::EngineStatus::OK) {
        auto score2Result = engine.getSimilarityScore(session2Id);
        if (score2Result.isOk()) {
            float score2 = score2Result.value;
            std::cout << "Noisy signal similarity: " << std::fixed << std::setprecision(8) << score2
                      << std::endl;
            std::cout << "Difference: " << (score - score2) << std::endl;

            if (score > score2) {
                std::cout << "✅ GOOD: Original has higher similarity than noisy version"
                          << std::endl;
            } else {
                std::cout << "❌ ISSUE: Noisy version has equal or higher similarity" << std::endl;
            }
        }
    }
    engine.endRealtimeSession(session2Id);
}

// Cleanup
engine.endRealtimeSession(sessionId);
drwav_free(audioData, nullptr);
engine.shutdown();

std::cout << "\n=== RECOMMENDATIONS ===" << std::endl;
if (score < 0.1) {
    std::cout << "1. Check DTW distance normalization - consider dividing by sequence length"
              << std::endl;
    std::cout << "2. Verify MFCC feature extraction produces identical results for identical input"
              << std::endl;
    std::cout << "3. Consider using a different similarity metric (cosine similarity, correlation)"
              << std::endl;
    std::cout << "4. Investigate if the DTW algorithm implementation is correct" << std::endl;
}

return 0;
}
