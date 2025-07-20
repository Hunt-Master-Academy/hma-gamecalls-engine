#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <span>
#include <string_view>  // Use string_view for consistency with the API
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/DebugConfig.h"
#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

// The 'using' declarations can be at the top level for clarity
using huntmaster::Component;
using huntmaster::DebugConfig;
using huntmaster::DebugLogger;
using huntmaster::LogLevel;
using huntmaster::UnifiedAudioEngine;

/**
 * @brief Command line argument parser for debugging control
 */
struct DebugOptions {
    LogLevel globalLevel = LogLevel::INFO;
    bool enableFileLogging = false;
    bool enableTimestamps = true;
    bool enableThreadIds = false;
    bool verbose = false;
    bool trace = false;

    void parseArgs(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--debug") {
                globalLevel = LogLevel::DEBUG;
            } else if (arg == "--trace") {
                globalLevel = LogLevel::TRACE;
                trace = true;
            } else if (arg == "--verbose" || arg == "-v") {
                verbose = true;
                globalLevel = LogLevel::INFO;
            } else if (arg == "--quiet" || arg == "-q") {
                globalLevel = LogLevel::WARN;
            } else if (arg == "--log-file") {
                enableFileLogging = true;
            } else if (arg == "--no-timestamps") {
                enableTimestamps = false;
            } else if (arg == "--thread-ids") {
                enableThreadIds = true;
            } else if (arg == "--help" || arg == "-h") {
                printUsage();
                exit(0);
            }
        }
    }

    void printUsage() {
        std::cout << "Huntmaster Recording Analyzer - Debug Options:\n"
                  << "  --debug         Enable debug logging\n"
                  << "  --trace         Enable trace logging (most verbose)\n"
                  << "  --verbose, -v   Enable verbose info logging\n"
                  << "  --quiet, -q     Only show warnings and errors\n"
                  << "  --log-file      Enable file logging\n"
                  << "  --no-timestamps Disable timestamps in output\n"
                  << "  --thread-ids    Show thread IDs in output\n"
                  << "  --help, -h      Show this help\n"
                  << "\nUsage: analyze_recording [options] [recording_path] [master_call_id]\n";
    }
};

/**
 * @brief Performance monitoring helper
 */
class PerformanceMonitor {
   private:
    std::chrono::high_resolution_clock::time_point startTime_;
    std::string operationName_;
    bool enabled_;

   public:
    PerformanceMonitor(const std::string& operationName, bool enabled = true)
        : operationName_(operationName), enabled_(enabled) {
        if (enabled_) {
            startTime_ = std::chrono::high_resolution_clock::now();
            LOG_DEBUG(Component::TOOLS, "Starting: " + operationName_);
        }
    }

    ~PerformanceMonitor() {
        if (enabled_) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_);
            LOG_INFO(Component::TOOLS, operationName_ + " completed in " +
                                           std::to_string(duration.count()) + " microseconds");
        }
    }
};

// Helper function to load an audio file into a float vector with enhanced debugging
std::vector<float> load_audio_file(const std::string& filePath, unsigned int& channels,
                                   unsigned int& sampleRate, const DebugOptions& debugOpts) {
    LOG_IF_TRACE(Component::TOOLS, "Attempting to load audio file: " + filePath);

    PerformanceMonitor perfMon("Audio file loading", debugOpts.verbose);

    drwav_uint64 totalPCMFrameCount = 0;
    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (pSampleData == nullptr) {
        LOG_ERROR(Component::TOOLS, "Could not load audio file: " + filePath);
        return {};
    }

    LOG_INFO(Component::TOOLS, "Audio file loaded successfully:");
    LOG_INFO(Component::TOOLS, "  - File: " + filePath);
    LOG_INFO(Component::TOOLS, "  - Frames: " + std::to_string(totalPCMFrameCount));
    LOG_INFO(Component::TOOLS, "  - Sample Rate: " + std::to_string(sampleRate) + " Hz");
    LOG_INFO(Component::TOOLS, "  - Channels: " + std::to_string(channels));
    LOG_INFO(Component::TOOLS,
             "  - Duration: " +
                 std::to_string(static_cast<float>(totalPCMFrameCount) / sampleRate) + " seconds");

    // Convert to mono with detailed logging
    std::vector<float> monoSamples(totalPCMFrameCount);
    if (channels > 1) {
        LOG_DEBUG(Component::TOOLS, "Converting " + std::to_string(channels) + " channels to mono");
        for (drwav_uint64 i = 0; i < totalPCMFrameCount; ++i) {
            float monoSample = 0.0f;
            for (unsigned int j = 0; j < channels; ++j) {
                monoSample += pSampleData[i * channels + j];
            }
            monoSamples[i] = monoSample / channels;
        }
        LOG_DEBUG(Component::TOOLS, "Multi-channel to mono conversion completed");
    } else {
        LOG_DEBUG(Component::TOOLS, "Audio is already mono, copying directly");
        monoSamples.assign(pSampleData, pSampleData + totalPCMFrameCount);
    }

    // Calculate and log audio statistics
    if (debugOpts.verbose) {
        float minValue = *std::min_element(monoSamples.begin(), monoSamples.end());
        float maxValue = *std::max_element(monoSamples.begin(), monoSamples.end());
        float rms = 0.0f;
        for (float sample : monoSamples) {
            rms += sample * sample;
        }
        rms = std::sqrt(rms / monoSamples.size());

        LOG_DEBUG(Component::TOOLS, "Audio statistics:");
        LOG_DEBUG(Component::TOOLS, "  - Min value: " + std::to_string(minValue));
        LOG_DEBUG(Component::TOOLS, "  - Max value: " + std::to_string(maxValue));
        LOG_DEBUG(Component::TOOLS, "  - RMS level: " + std::to_string(rms));
        LOG_DEBUG(Component::TOOLS, "  - Dynamic range: " + std::to_string(maxValue - minValue));
    }

    drwav_free(pSampleData, nullptr);
    return monoSamples;
}

/**
 * @brief Enhanced audio analysis with comprehensive debugging
 */
class AudioAnalyzer {
   private:
    std::unique_ptr<UnifiedAudioEngine> engine_;
    DebugOptions debugOpts_;

   public:
    AudioAnalyzer(const DebugOptions& debugOpts) : debugOpts_(debugOpts) {
        LOG_DEBUG(Component::TOOLS, "Initializing AudioAnalyzer");
    }

    bool initialize() {
        LOG_DEBUG(Component::TOOLS, "Creating UnifiedAudioEngine");
        auto engineResult = UnifiedAudioEngine::create();
        if (!engineResult.isOk()) {
            LOG_ERROR(Component::TOOLS, "Failed to create UnifiedAudioEngine");
            return false;
        }
        engine_ = std::move(engineResult.value);
        LOG_INFO(Component::TOOLS, "UnifiedAudioEngine created successfully");
        return true;
    }

    bool analyzeRecording(const std::string& recordingPath, const std::string& masterCallId) {
        LOG_INFO(Component::TOOLS, "=== Starting Audio Analysis ===");
        LOG_INFO(Component::TOOLS, "Recording: " + recordingPath);
        LOG_INFO(Component::TOOLS, "Master Call: " + masterCallId);

        // Step 1: Load recording
        {
            PerformanceMonitor perfMon("Recording loading", debugOpts_.verbose);
            LOG_INFO(Component::TOOLS, "1. Loading recording...");

            unsigned int channels, sampleRate;
            std::vector<float> audioData =
                load_audio_file(recordingPath, channels, sampleRate, debugOpts_);

            if (audioData.empty()) {
                LOG_ERROR(Component::TOOLS, "Failed to load recording");
                return false;
            }

            // Step 2: Create session
            LOG_INFO(Component::TOOLS, "2. Creating audio session...");
            auto sessionResult = engine_->createSession(static_cast<float>(sampleRate));
            if (!sessionResult.isOk()) {
                LOG_ERROR(Component::TOOLS, "Failed to create session");
                return false;
            }
            auto sessionId = sessionResult.value;
            LOG_DEBUG(Component::TOOLS, "Session created with ID: " + std::to_string(sessionId));

            // Step 3: Load master call
            LOG_INFO(Component::TOOLS, "3. Loading master call...");
            auto loadResult = engine_->loadMasterCall(sessionId, masterCallId);
            if (loadResult != UnifiedAudioEngine::Status::OK) {
                LOG_ERROR(Component::TOOLS, "Failed to load master call '" + masterCallId + "'");
                return false;
            }
            LOG_INFO(Component::TOOLS, "Master call loaded successfully");

            // Step 4: Process audio
            LOG_INFO(Component::TOOLS, "4. Processing audio chunks...");
            return processAudioChunks(sessionId, audioData, sampleRate);
        }
    }

   private:
    bool processAudioChunks(huntmaster::SessionId sessionId, const std::vector<float>& audioData,
                            unsigned int sampleRate) {
        PerformanceMonitor perfMon("Audio chunk processing", debugOpts_.verbose);

        const int chunkSize = 1024;
        int totalChunks = (audioData.size() + chunkSize - 1) / chunkSize;
        int processedChunks = 0;

        LOG_INFO(Component::TOOLS, "Processing " + std::to_string(totalChunks) + " audio chunks");
        LOG_DEBUG(Component::TOOLS, "Chunk size: " + std::to_string(chunkSize) + " samples");

        for (size_t i = 0; i < audioData.size(); i += chunkSize) {
            size_t remainingSamples = audioData.size() - i;
            size_t samplesToProcess = std::min(static_cast<size_t>(chunkSize), remainingSamples);

            LOG_IF_TRACE(Component::TOOLS, "Processing chunk " +
                                               std::to_string(processedChunks + 1) + "/" +
                                               std::to_string(totalChunks) + " (" +
                                               std::to_string(samplesToProcess) + " samples)");

            // Process audio chunk using span
            std::span<const float> audioChunk(audioData.data() + i, samplesToProcess);
            auto processStatus = engine_->processAudioChunk(sessionId, audioChunk);

            if (processStatus != UnifiedAudioEngine::Status::OK) {
                LOG_ERROR(Component::TOOLS,
                          "Error processing audio chunk " + std::to_string(processedChunks + 1));
                return false;
            }

            processedChunks++;

            // Progress indicator
            if (debugOpts_.verbose || processedChunks % 10 == 0) {
                if (debugOpts_.verbose) {
                    float progress = (float)processedChunks / totalChunks * 100.0f;
                    LOG_DEBUG(Component::TOOLS, "Progress: " + std::to_string(progress) + "%");
                } else {
                    std::cout << ".";
                    std::cout.flush();
                }
            }
        }

        if (!debugOpts_.verbose) {
            std::cout << std::endl;
        }

        LOG_INFO(Component::TOOLS, "Audio processing completed successfully");
        LOG_INFO(Component::TOOLS, "Total chunks processed: " + std::to_string(processedChunks));

        // Step 5: Calculate similarity score
        return calculateSimilarityScore(sessionId, audioData, sampleRate);
    }

    bool calculateSimilarityScore(huntmaster::SessionId sessionId,
                                  const std::vector<float>& audioData, unsigned int sampleRate) {
        PerformanceMonitor perfMon("Similarity score calculation", debugOpts_.verbose);

        LOG_INFO(Component::TOOLS, "5. Calculating similarity score...");

        auto scoreResult = engine_->getSimilarityScore(sessionId);
        if (!scoreResult.isOk()) {
            LOG_ERROR(Component::TOOLS, "Could not calculate similarity score");
            return false;
        }

        float score = scoreResult.value;
        float duration = static_cast<float>(audioData.size()) / sampleRate;

        // Display results
        displayResults(score, duration, audioData.size(), sampleRate);

        return true;
    }

    void displayResults(float score, float duration, size_t sampleCount, unsigned int sampleRate) {
        LOG_INFO(Component::TOOLS, "=== Analysis Results ===");

        std::cout << "\n========================================" << std::endl;
        std::cout << "Recording Analysis Results" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Duration: " << std::fixed << std::setprecision(2) << duration << " seconds"
                  << std::endl;
        std::cout << "Sample Count: " << sampleCount << std::endl;
        std::cout << "Sample Rate: " << sampleRate << " Hz" << std::endl;
        std::cout << "Similarity Score: " << std::fixed << std::setprecision(4) << score
                  << std::endl;
        std::cout << "========================================" << std::endl;

        // Enhanced interpretation with detailed feedback
        const float EXCELLENT_THRESHOLD = 0.8f;
        const float GOOD_THRESHOLD = 0.6f;
        const float FAIR_THRESHOLD = 0.4f;
        const float SOME_SIMILARITY_THRESHOLD = 0.2f;

        std::string interpretation;
        std::string recommendation;

        if (score > EXCELLENT_THRESHOLD) {
            interpretation = "EXCELLENT match to master call!";
            recommendation = "This is a high-quality reproduction of the master call.";
            LOG_INFO(Component::TOOLS, "Score interpretation: EXCELLENT");
        } else if (score > GOOD_THRESHOLD) {
            interpretation = "Good match to master call";
            recommendation = "This is a solid attempt with room for minor improvements.";
            LOG_INFO(Component::TOOLS, "Score interpretation: GOOD");
        } else if (score > FAIR_THRESHOLD) {
            interpretation = "Fair match to master call";
            recommendation = "Consider practicing timing and pitch accuracy.";
            LOG_INFO(Component::TOOLS, "Score interpretation: FAIR");
        } else if (score > SOME_SIMILARITY_THRESHOLD) {
            interpretation = "Some similarity to master call";
            recommendation = "Significant practice needed to improve similarity.";
            LOG_INFO(Component::TOOLS, "Score interpretation: SOME_SIMILARITY");
        } else {
            interpretation = "Different from master call";
            recommendation = "This sounds quite different from the target call.";
            LOG_INFO(Component::TOOLS, "Score interpretation: DIFFERENT");
        }

        std::cout << "Interpretation: " << interpretation << std::endl;
        std::cout << "Recommendation: " << recommendation << std::endl;

        // Additional debug information
        if (debugOpts_.verbose) {
            std::cout << "\nDetailed Analysis:" << std::endl;
            std::cout << "  - Score range: 0.0 (no similarity) to 1.0 (perfect match)" << std::endl;
            std::cout << "  - Algorithm: DTW (Dynamic Time Warping) with MFCC features"
                      << std::endl;
            std::cout << "  - Processing chunks: " << ((sampleCount + 1023) / 1024) << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    // Parse debug options first
    DebugOptions debugOpts;
    debugOpts.parseArgs(argc, argv);

    // Configure logging based on options
    auto& logger = DebugLogger::getInstance();
    logger.setGlobalLogLevel(debugOpts.globalLevel);
    logger.enableConsoleOutput(true);
    logger.enableTimestamps(debugOpts.enableTimestamps);
    logger.enableThreadIds(debugOpts.enableThreadIds);

    if (debugOpts.enableFileLogging) {
        logger.enableFileLogging("analyze_recording_debug.log");
    }

    // Enhanced tool debugging
    if (debugOpts.trace) {
        DebugConfig::setupFullDebug();
    } else if (debugOpts.verbose) {
        DebugConfig::setupToolsDebug();
    }

    LOG_INFO(Component::TOOLS, "=== Huntmaster Recording Analyzer (Debug Enhanced) ===");
    LOG_DEBUG(Component::TOOLS,
              "Debug level: " + std::to_string(static_cast<int>(debugOpts.globalLevel)));

    // Parse file arguments
    std::string recordingPath = "../data/recordings/user_attempt_buck_grunt.wav";
    std::string masterCallId = "buck_grunt";

    // Extract non-debug arguments
    std::vector<std::string> fileArgs;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--") != 0 && arg.find("-") != 0) {
            fileArgs.push_back(arg);
        }
    }

    if (fileArgs.size() > 0) {
        recordingPath = fileArgs[0];
    }
    if (fileArgs.size() > 1) {
        masterCallId = fileArgs[1];
    }

    // Initialize and run analyzer
    AudioAnalyzer analyzer(debugOpts);

    if (!analyzer.initialize()) {
        LOG_ERROR(Component::TOOLS, "Failed to initialize analyzer");
        return 1;
    }

    if (!analyzer.analyzeRecording(recordingPath, masterCallId)) {
        LOG_ERROR(Component::TOOLS, "Analysis failed");
        return 1;
    }

    LOG_INFO(Component::TOOLS, "Analysis completed successfully");
    return 0;
}