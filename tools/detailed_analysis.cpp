#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// These headers are needed for the tool's functionality
#include <huntmaster/core/DebugConfig.h>
#include <huntmaster/core/DebugLogger.h>

#include "dr_wav.h"
#include "huntmaster/core/HuntmasterAudioEngine.h"

// Debug options for detailed analysis
struct DebugOptions {
    bool enableDebug = false;
    bool enableTrace = false;
    bool enableVerbose = false;
    bool enableEngineDebug = false;
    bool enableAnalysisDebug = false;
    bool enableAudioDebug = false;
    bool enableScoreDebug = false;
    bool enablePerformanceMetrics = false;
    bool printHelp = false;

    void parseArgs(int argc, char* argv[]) {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "--debug" || arg == "-d") {
                enableDebug = true;
            } else if (arg == "--trace" || arg == "-t") {
                enableTrace = true;
            } else if (arg == "--verbose" || arg == "-v") {
                enableVerbose = true;
            } else if (arg == "--engine-debug") {
                enableEngineDebug = true;
            } else if (arg == "--analysis-debug") {
                enableAnalysisDebug = true;
            } else if (arg == "--audio-debug") {
                enableAudioDebug = true;
            } else if (arg == "--score-debug") {
                enableScoreDebug = true;
            } else if (arg == "--performance" || arg == "-p") {
                enablePerformanceMetrics = true;
            } else if (arg == "--help" || arg == "-h") {
                printHelp = true;
            }
        }
    }

    void printUsage(const char* programName) {
        std::cout << "Usage: " << programName << " [OPTIONS] <path_to_your_recording.wav>\n"
                  << "Detailed analysis tool for comparing recordings against master calls\n\n"
                  << "Options:\n"
                  << "  --debug, -d          Enable debug logging\n"
                  << "  --trace, -t          Enable trace logging (most verbose)\n"
                  << "  --verbose, -v        Enable verbose output\n"
                  << "  --engine-debug       Enable engine-specific debugging\n"
                  << "  --analysis-debug     Enable analysis-specific debugging\n"
                  << "  --audio-debug        Enable audio-specific debugging\n"
                  << "  --score-debug        Enable score calculation debugging\n"
                  << "  --performance, -p    Enable performance metrics\n"
                  << "  --help, -h           Show this help message\n"
                  << std::endl;
    }
};

// Performance monitoring class
class PerformanceMonitor {
   private:
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
    bool enabled;

   public:
    PerformanceMonitor(const std::string& testName, bool enable = true)
        : name(testName), enabled(enable) {
        if (enabled) {
            startTime = std::chrono::high_resolution_clock::now();
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::PERFORMANCE,
                                                       huntmaster::DebugLevel::INFO,
                                                       "Starting: " + name);
        }
    }

    ~PerformanceMonitor() {
        if (enabled) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::PERFORMANCE, huntmaster::DebugLevel::INFO,
                "Completed: " + name + " in " + std::to_string(duration.count()) + " μs");
        }
    }

    void checkpoint(const std::string& message) {
        if (enabled) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::PERFORMANCE, huntmaster::DebugLevel::DEBUG,
                name + " checkpoint: " + message + " at " + std::to_string(duration.count()) +
                    " μs");
        }
    }
};

// Global debug options
DebugOptions g_debugOptions;

// For now, we'll include the function directly until we make the shared header
namespace AudioUtils {
std::vector<float> load_audio_file(const std::string& filePath, unsigned int& channels,
                                   unsigned int& sampleRate) {
    PerformanceMonitor monitor("Loading audio file: " + filePath,
                               g_debugOptions.enablePerformanceMetrics);

    if (g_debugOptions.enableAudioDebug) {
        huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                   huntmaster::DebugLevel::DEBUG,
                                                   "Loading audio file: " + filePath);
    }

    drwav_uint64 totalPCMFrameCount = 0;
    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);
    if (!pSampleData) {
        std::cerr << "Error: Could not load audio file: " << filePath << std::endl;

        if (g_debugOptions.enableAudioDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::ERROR,
                                                       "Failed to load audio file: " + filePath);
        }
        return {};
    }

    monitor.checkpoint("Raw audio data loaded");

    std::cout << "  - Loaded: " << filePath << std::endl;

    if (g_debugOptions.enableAudioDebug) {
        huntmaster::DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
            "Audio file loaded - Channels: " + std::to_string(channels) + ", Sample Rate: " +
                std::to_string(sampleRate) + ", Frames: " + std::to_string(totalPCMFrameCount));
    }

    std::vector<float> monoSamples(totalPCMFrameCount);
    if (channels > 1) {
        if (g_debugOptions.enableAudioDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Converting multi-channel audio to mono");
        }

        for (drwav_uint64 i = 0; i < totalPCMFrameCount; ++i) {
            float monoSample = 0.0f;
            for (unsigned int j = 0; j < channels; ++j) {
                monoSample += pSampleData[i * channels + j];
            }
            monoSamples[i] = monoSample / channels;
        }
    } else {
        monoSamples.assign(pSampleData, pSampleData + totalPCMFrameCount);
    }

    drwav_free(pSampleData, nullptr);
    monitor.checkpoint("Audio conversion completed");

    if (g_debugOptions.enableAudioDebug) {
        // Calculate some basic audio statistics
        float minSample = *std::min_element(monoSamples.begin(), monoSamples.end());
        float maxSample = *std::max_element(monoSamples.begin(), monoSamples.end());
        float avgSample = 0.0f;
        for (float sample : monoSamples) {
            avgSample += std::abs(sample);
        }
        avgSample /= monoSamples.size();

        huntmaster::DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Audio statistics - Min: " + std::to_string(minSample) + ", Max: " +
                std::to_string(maxSample) + ", Avg Magnitude: " + std::to_string(avgSample));
    }

    return monoSamples;
}
}  // namespace AudioUtils

using huntmaster::HuntmasterAudioEngine;

// Enhanced detailed analysis class
class DetailedAnalyzer {
   private:
    HuntmasterAudioEngine& engine;

    struct AnalysisResult {
        std::string masterCallId;
        float score;
        bool success;
        std::string errorMessage;

        AnalysisResult(const std::string& id, float s, bool suc = true, const std::string& err = "")
            : masterCallId(id), score(s), success(suc), errorMessage(err) {}
    };

   public:
    DetailedAnalyzer(HuntmasterAudioEngine& eng) : engine(eng) {}

    std::vector<AnalysisResult> analyzeRecording(const std::string& recordingPath,
                                                 const std::vector<std::string>& masterCalls) {
        PerformanceMonitor monitor("Complete recording analysis",
                                   g_debugOptions.enablePerformanceMetrics);

        huntmaster::DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
            "Starting detailed analysis of: " + recordingPath);

        std::vector<AnalysisResult> results;

        // --- 1. Load the User's Recording ---
        unsigned int channels, sampleRate;
        std::vector<float> recordingAudio =
            AudioUtils::load_audio_file(recordingPath, channels, sampleRate);
        if (recordingAudio.empty()) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::ERROR,
                "Failed to load recording: " + recordingPath);
            return results;
        }

        monitor.checkpoint("User recording loaded");

        if (g_debugOptions.enableVerbose) {
            std::cout << "  - Duration: " << std::fixed << std::setprecision(2)
                      << static_cast<float>(recordingAudio.size()) / sampleRate << " seconds"
                      << std::endl;
            std::cout << "  - Sample Rate: " << sampleRate << " Hz" << std::endl;
            std::cout << "  - Channels: " << channels << std::endl;
        }

        // --- 2. Iterate and Compare Against All Master Calls ---
        std::cout << "\nComparing against all master calls...\n"
                  << std::string(40, '-') << std::endl;

        for (size_t i = 0; i < masterCalls.size(); ++i) {
            const auto& masterId = masterCalls[i];

            if (g_debugOptions.enableVerbose) {
                std::cout << "Processing " << (i + 1) << "/" << masterCalls.size() << ": "
                          << masterId << std::endl;
            }

            AnalysisResult result = analyzeSingleMasterCall(masterId, recordingAudio, sampleRate,
                                                            i + 1, masterCalls.size());
            results.push_back(result);
        }

        monitor.checkpoint("All master calls analyzed");

        huntmaster::DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
            "Analysis completed for " + std::to_string(results.size()) + " master calls");

        return results;
    }

   private:
    AnalysisResult analyzeSingleMasterCall(const std::string& masterId,
                                           const std::vector<float>& recordingAudio,
                                           unsigned int sampleRate, size_t currentIndex,
                                           size_t totalCount) {
        PerformanceMonitor monitor("Analysis of " + masterId,
                                   g_debugOptions.enablePerformanceMetrics);

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                "Starting analysis of master call: " + masterId);
        }

        // A. Load the master call
        if (engine.loadMasterCall(masterId) != HuntmasterAudioEngine::EngineStatus::OK) {
            std::cerr << "Could not load master call: " << masterId << ". Skipping." << std::endl;

            if (g_debugOptions.enableAnalysisDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Failed to load master call: " + masterId);
            }

            return AnalysisResult(masterId, 0.0f, false, "Failed to load master call");
        }

        monitor.checkpoint("Master call loaded");

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "Master call loaded successfully: " + masterId);
        }

        // B. Start a session for this comparison
        auto sessionResult = engine.startRealtimeSession(static_cast<float>(sampleRate), 4096);
        if (!sessionResult.isOk()) {
            std::cerr << "Could not start session for " << masterId << ". Skipping." << std::endl;

            if (g_debugOptions.enableAnalysisDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Failed to start session for " + masterId);
            }

            return AnalysisResult(masterId, 0.0f, false, "Failed to start session");
        }

        int sessionId = sessionResult.value;
        monitor.checkpoint("Session started");

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "Session started for " + masterId + " with ID: " + std::to_string(sessionId));
        }

        // C. Process the entire user recording
        if (engine.processAudioChunk(sessionId, recordingAudio.data(), recordingAudio.size()) !=
            HuntmasterAudioEngine::EngineStatus::OK) {
            std::cerr << "Could not process audio for " << masterId << ". Skipping." << std::endl;

            if (g_debugOptions.enableAnalysisDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Failed to process audio for " + masterId);
            }

            engine.endRealtimeSession(sessionId);
            return AnalysisResult(masterId, 0.0f, false, "Failed to process audio");
        }

        monitor.checkpoint("Audio processed");

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::DEBUG,
                "Audio processing completed for " + masterId + " (" +
                    std::to_string(recordingAudio.size()) + " samples)");
        }

        // D. Get the score
        auto scoreResult = engine.getSimilarityScore(sessionId);
        float currentScore = 0.0f;
        bool success = false;
        std::string errorMessage;

        if (scoreResult.isOk()) {
            currentScore = scoreResult.value;
            success = true;

            std::cout << "  - vs " << std::setw(20) << std::left << masterId
                      << " -> Score: " << std::fixed << std::setprecision(5) << currentScore
                      << std::endl;

            if (g_debugOptions.enableScoreDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::SIMILARITY_ANALYSIS, huntmaster::DebugLevel::INFO,
                    "Score calculated for " + masterId + ": " + std::to_string(currentScore));
            }
        } else {
            std::cout << "  - vs " << std::setw(20) << std::left << masterId
                      << " -> Error calculating score." << std::endl;
            errorMessage = "Error calculating score";

            if (g_debugOptions.enableScoreDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::SIMILARITY_ANALYSIS, huntmaster::DebugLevel::ERROR,
                    "Failed to calculate score for " + masterId);
            }
        }

        monitor.checkpoint("Score calculated");

        // E. Clean up the session for the next loop
        engine.endRealtimeSession(sessionId);

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::AUDIO_ENGINE,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Session ended for " + masterId);
        }

        return AnalysisResult(masterId, currentScore, success, errorMessage);
    }
};

int main(int argc, char* argv[]) {
    // Parse debug options
    g_debugOptions.parseArgs(argc, argv);

    if (g_debugOptions.printHelp) {
        g_debugOptions.printUsage(argv[0]);
        return 0;
    }

    // Set up debugging based on options
    if (g_debugOptions.enableTrace) {
        huntmaster::DebugConfig::setupFullDebug();
    } else if (g_debugOptions.enableDebug) {
        huntmaster::DebugConfig::setupToolsDebug();
    }

    // Configure component-specific debug levels
    auto& logger = huntmaster::DebugLogger::getInstance();
    if (g_debugOptions.enableEngineDebug) {
        logger.setComponentLogLevel(huntmaster::Component::AUDIO_ENGINE,
                                    huntmaster::LogLevel::DEBUG);
    }
    if (g_debugOptions.enableAnalysisDebug) {
        logger.setComponentLogLevel(huntmaster::Component::SIMILARITY_ANALYSIS,
                                    huntmaster::LogLevel::DEBUG);
        logger.setComponentLogLevel(huntmaster::Component::FEATURE_EXTRACTION,
                                    huntmaster::LogLevel::DEBUG);
    }
    if (g_debugOptions.enableAudioDebug) {
        logger.setComponentLogLevel(huntmaster::Component::TOOLS, huntmaster::LogLevel::DEBUG);
    }
    if (g_debugOptions.enableScoreDebug) {
        logger.setComponentLogLevel(huntmaster::Component::SIMILARITY_ANALYSIS,
                                    huntmaster::LogLevel::TRACE);
    }
    if (g_debugOptions.enablePerformanceMetrics) {
        logger.setComponentLogLevel(huntmaster::Component::PERFORMANCE,
                                    huntmaster::LogLevel::DEBUG);
    }

    huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::INFO,
                                               "=== Detailed Analysis Tool Started ===");

    std::cout << "=== Detailed Recording Analysis ===" << std::endl;

    // Find the recording file argument (not a debug option)
    std::string recordingPath;
    bool foundRecordingPath = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg[0] != '-') {  // Not a debug option
            recordingPath = arg;
            foundRecordingPath = true;
            break;
        }
    }

    if (!foundRecordingPath) {
        std::cout << "\nUsage: " << argv[0] << " [OPTIONS] <path_to_your_recording.wav>"
                  << std::endl;
        std::cout << "Use --help for more information." << std::endl;
        return 1;
    }

    std::cout << "\nAnalyzing recording: " << recordingPath << std::endl;

    if (g_debugOptions.enableVerbose) {
        huntmaster::DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
            "Starting detailed analysis of: " + recordingPath);
    }

    try {
        PerformanceMonitor totalMonitor("Complete detailed analysis",
                                        g_debugOptions.enablePerformanceMetrics);

        // --- 2. Initialize the Engine ---
        HuntmasterAudioEngine& engine = HuntmasterAudioEngine::getInstance();

        if (g_debugOptions.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::AUDIO_ENGINE,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Initializing HuntmasterAudioEngine");
        }

        engine.initialize();
        totalMonitor.checkpoint("Engine initialized");

        if (g_debugOptions.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "HuntmasterAudioEngine initialized successfully");
        }

        // --- 3. Define Master Calls ---
        std::vector<std::string> masterCalls = {
            "buck_grunt",     "doe-grunt",    "buck-bawl", "breeding_bellow",
            "contact-bleatr", "estrus_bleat", "fawn-bleat"
            // Add all your master call IDs here
        };

        if (g_debugOptions.enableVerbose) {
            std::cout << "Will compare against " << masterCalls.size() << " master calls"
                      << std::endl;
        }

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                "Configured " + std::to_string(masterCalls.size()) +
                    " master calls for comparison");
        }

        // --- 4. Perform Analysis ---
        DetailedAnalyzer analyzer(engine);
        auto results = analyzer.analyzeRecording(recordingPath, masterCalls);

        totalMonitor.checkpoint("Analysis completed");

        // --- 5. Find Best Match ---
        float bestScore = -1.0f;
        std::string bestMatchName = "None";
        int successfulAnalyses = 0;

        for (const auto& result : results) {
            if (result.success) {
                successfulAnalyses++;
                if (result.score > bestScore) {
                    bestScore = result.score;
                    bestMatchName = result.masterCallId;
                }
            }
        }

        // --- 6. Report Results ---
        std::cout << "\n========================================" << std::endl;
        std::cout << "           ANALYSIS COMPLETE" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Recording '" << recordingPath << "'\nmost closely matches:\n" << std::endl;
        std::cout << "  -> Master Call: " << bestMatchName << std::endl;
        std::cout << "  -> Similarity Score: " << std::fixed << std::setprecision(5) << bestScore
                  << std::endl;
        std::cout << "  -> Successful Analyses: " << successfulAnalyses << "/" << results.size()
                  << std::endl;
        std::cout << "========================================\n" << std::endl;

        if (g_debugOptions.enableVerbose) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                "Analysis complete - Best match: " + bestMatchName + " with score: " +
                    std::to_string(bestScore) + " (" + std::to_string(successfulAnalyses) + "/" +
                    std::to_string(results.size()) + " successful)");
        }

        // Optional: Show detailed breakdown if verbose
        if (g_debugOptions.enableVerbose) {
            std::cout << "\nDetailed Results:" << std::endl;
            for (const auto& result : results) {
                std::cout << "  " << std::setw(20) << std::left << result.masterCallId;
                if (result.success) {
                    std::cout << " -> " << std::fixed << std::setprecision(5) << result.score;
                } else {
                    std::cout << " -> FAILED (" << result.errorMessage << ")";
                }
                std::cout << std::endl;
            }
        }

        totalMonitor.checkpoint("Results reported");

        if (g_debugOptions.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::AUDIO_ENGINE,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Shutting down HuntmasterAudioEngine");
        }

        engine.shutdown();

        if (g_debugOptions.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::AUDIO_ENGINE,
                                                       huntmaster::DebugLevel::INFO,
                                                       "HuntmasterAudioEngine shutdown completed");
        }

        huntmaster::DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
            "=== Detailed Analysis Tool Completed Successfully ===");

    } catch (const std::exception& e) {
        std::cerr << "❌ An unexpected error occurred: " << e.what() << std::endl;
        huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                   huntmaster::DebugLevel::ERROR,
                                                   "Exception occurred: " + std::string(e.what()));
        return 1;
    }

    return 0;
}