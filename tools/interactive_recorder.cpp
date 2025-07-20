#include <huntmaster/core/DebugConfig.h>
#include <huntmaster/core/DebugLogger.h>
#include <huntmaster/core/UnifiedAudioEngine.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

using huntmaster::UnifiedAudioEngine;

// Helper function to convert Status to string
std::string statusToString(UnifiedAudioEngine::Status status) {
    switch (status) {
        case UnifiedAudioEngine::Status::OK:
            return "OK";
        case UnifiedAudioEngine::Status::INVALID_PARAMS:
            return "Invalid parameters";
        case UnifiedAudioEngine::Status::SESSION_NOT_FOUND:
            return "Session not found";
        case UnifiedAudioEngine::Status::FILE_NOT_FOUND:
            return "File not found";
        case UnifiedAudioEngine::Status::PROCESSING_ERROR:
            return "Processing error";
        case UnifiedAudioEngine::Status::INSUFFICIENT_DATA:
            return "Insufficient data";
        case UnifiedAudioEngine::Status::OUT_OF_MEMORY:
            return "Out of memory";
        case UnifiedAudioEngine::Status::INIT_FAILED:
            return "Initialization failed";
        default:
            return "Unknown error";
    }
}

// Debug options for interactive recorder
struct DebugOptions {
    bool enableDebug = false;
    bool enableTrace = false;
    bool enableVerbose = false;
    bool enableEngineDebug = false;
    bool enableRecordingDebug = false;
    bool enablePlaybackDebug = false;
    bool enableAnalysisDebug = false;
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
            } else if (arg == "--recording-debug") {
                enableRecordingDebug = true;
            } else if (arg == "--playback-debug") {
                enablePlaybackDebug = true;
            } else if (arg == "--analysis-debug") {
                enableAnalysisDebug = true;
            } else if (arg == "--performance" || arg == "-p") {
                enablePerformanceMetrics = true;
            } else if (arg == "--help" || arg == "-h") {
                printHelp = true;
            }
        }
    }

    void printUsage(const char* programName) {
        std::cout << "Usage: " << programName << " [OPTIONS]\n"
                  << "Interactive audio recorder with live monitoring and analysis\n\n"
                  << "Options:\n"
                  << "  --debug, -d          Enable debug logging\n"
                  << "  --trace, -t          Enable trace logging (most verbose)\n"
                  << "  --verbose, -v        Enable verbose output\n"
                  << "  --engine-debug       Enable engine-specific debugging\n"
                  << "  --recording-debug    Enable recording-specific debugging\n"
                  << "  --playback-debug     Enable playback-specific debugging\n"
                  << "  --analysis-debug     Enable analysis-specific debugging\n"
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

void printLevel(float level) {
    if (g_debugOptions.enableRecordingDebug) {
        huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                   huntmaster::DebugLevel::TRACE,
                                                   "Audio level: " + std::to_string(level));
    }

    int bars = static_cast<int>(level * 50);
    std::cout << "\rLevel: [";
    for (int j = 0; j < bars; ++j) std::cout << "=";
    for (int j = bars; j < 50; ++j) std::cout << " ";
    std::cout << "] " << std::fixed << std::setprecision(2) << level;
    std::cout.flush();
}

void showMenu() {
    std::cout << "\n=== Huntmaster Interactive Recorder ===" << std::endl;
    std::cout << "1. Record audio (with live monitoring)" << std::endl;
    std::cout << "2. Play last recording" << std::endl;
    std::cout << "3. Load and play master call" << std::endl;
    std::cout << "4. Record and compare to master" << std::endl;
    std::cout << "5. Exit" << std::endl;
    std::cout << "Choice: ";
}

// Enhanced interactive recorder class
class InteractiveRecorder {
   private:
    std::unique_ptr<UnifiedAudioEngine> engine;
    uint32_t currentSessionId;
    std::string lastRecordingFile;
    bool sessionActive;

   public:
    InteractiveRecorder() : currentSessionId(huntmaster::INVALID_SESSION_ID), sessionActive(false) {
        auto engineResult = UnifiedAudioEngine::create();
        if (!engineResult.isOk()) {
            throw std::runtime_error("Failed to create UnifiedAudioEngine");
        }
        engine = std::move(engineResult.value);

        huntmaster::DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
            "InteractiveRecorder initialized with UnifiedAudioEngine");
    }
    void recordAudio() {
        PerformanceMonitor monitor("Record audio", g_debugOptions.enablePerformanceMetrics);

        std::cout << "\nHow many seconds to record? ";
        int seconds;
        std::cin >> seconds;
        std::cin.ignore();

        if (g_debugOptions.enableRecordingDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                "Starting audio recording for " + std::to_string(seconds) + " seconds");
        }

        // Create real-time session for interactive recording
        if (!sessionActive) {
            auto sessionResult = engine->startRealtimeSession(44100.0f, 512);
            if (!sessionResult.isOk()) {
                std::cout << "Failed to create real-time session for recording!" << std::endl;
                return;
            }
            currentSessionId = sessionResult.value;
            sessionActive = true;
        }

        std::cout << "Starting recording in 3... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "2... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "1... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "GO!" << std::endl;

        auto startResult = engine->startRecording(currentSessionId);
        if (startResult != UnifiedAudioEngine::Status::OK) {
            std::cout << "Failed to start recording!" << std::endl;
            return;
        }
        monitor.checkpoint("Recording started");

        if (g_debugOptions.enableRecordingDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "Recording started for session: " + std::to_string(currentSessionId));
        }

        // Monitor for the specified duration with live level monitoring
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(seconds)) {
            // Get current recording level for visual feedback
            auto levelResult = engine->getRecordingLevel(currentSessionId);
            if (levelResult.isOk()) {
                printLevel(levelResult.value);
            } else {
                std::cout << "●" << std::flush;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << std::endl;

        monitor.checkpoint("Recording duration completed");

        auto stopResult = engine->stopRecording(currentSessionId);
        if (stopResult != UnifiedAudioEngine::Status::OK) {
            std::cout << "Failed to stop recording!" << std::endl;
            return;
        }

        if (g_debugOptions.enableRecordingDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "Recording stopped for session: " + std::to_string(currentSessionId));
        }

        std::cout << "Enter filename (without .wav): ";
        std::string filename;
        std::getline(std::cin, filename);
        filename += ".wav";

        auto saveResult = engine->saveRecording(currentSessionId, filename);
        if (!saveResult.isOk()) {
            std::cout << "Failed to save recording!" << std::endl;
            return;
        }

        lastRecordingFile = filename;
        std::cout << "Saved to: " << lastRecordingFile << std::endl;

        monitor.checkpoint("Recording saved");

        if (g_debugOptions.enableRecordingDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::AUDIO_ENGINE,
                                                       huntmaster::DebugLevel::INFO,
                                                       "Recording saved to: " + lastRecordingFile);
        }
    }

    void playLastRecording() {
        PerformanceMonitor monitor("Play last recording", g_debugOptions.enablePerformanceMetrics);

        if (lastRecordingFile.empty()) {
            std::cout << "No recording available!" << std::endl;

            if (g_debugOptions.enablePlaybackDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::WARN,
                    "Attempted to play recording but none available");
            }
            return;
        }

        // Create session if needed
        if (!sessionActive) {
            auto sessionResult = engine->startRealtimeSession(44100.0f, 512);
            if (!sessionResult.isOk()) {
                std::cout << "Failed to create session for playback!" << std::endl;
                return;
            }
            currentSessionId = sessionResult.value;
            sessionActive = true;
        }

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Playing recording: " + lastRecordingFile);
        }

        std::cout << "Playing: " << lastRecordingFile << std::endl;
        auto status = engine->playRecording(currentSessionId, lastRecordingFile);

        if (status != UnifiedAudioEngine::Status::OK) {
            std::cout << "Failed to start playback!" << std::endl;
            return;
        }

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::DEBUG,
                "Playback status: " + std::to_string(static_cast<int>(status)));
        }

        monitor.checkpoint("Playback started");

        // Wait for playback to finish or user interruption
        std::cout << "Playing... (press Enter to stop)" << std::endl;
        std::cout << "Status: ";
        while (engine->isPlaying(currentSessionId)) {
            std::cout << "♪ " << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        std::cout << "\nPlayback finished." << std::endl;
        monitor.checkpoint("Playback completed");

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "Playback completed for: " + lastRecordingFile);
        }
    }

    void loadAndPlayMaster() {
        PerformanceMonitor monitor("Load and play master", g_debugOptions.enablePerformanceMetrics);

        std::cout << "Enter master call ID (e.g., 'buck_grunt'): ";
        std::string callId;
        std::getline(std::cin, callId);

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Loading master call: " + callId);
        }

        // Create session if needed
        if (!sessionActive) {
            auto sessionResult = engine->startRealtimeSession(44100.0f, 512);
            if (!sessionResult.isOk()) {
                std::cout << "Failed to create session!" << std::endl;
                return;
            }
            currentSessionId = sessionResult.value;
            sessionActive = true;
        }

        auto loadResult = engine->loadMasterCall(currentSessionId, callId);
        if (loadResult != UnifiedAudioEngine::Status::OK) {
            std::cout << "Failed to load master call: " << callId << std::endl;
            return;
        }
        monitor.checkpoint("Master call loaded");

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "Master call loaded: " + callId +
                    " for session: " + std::to_string(currentSessionId));
        }

        std::cout << "Master call '" << callId << "' loaded successfully!" << std::endl;
        std::cout << "Now playing master call..." << std::endl;

        // Play the master call
        auto playStatus = engine->playMasterCall(currentSessionId, callId);
        if (playStatus != UnifiedAudioEngine::Status::OK) {
            std::cout << "Failed to play master call!" << std::endl;
            return;
        }

        monitor.checkpoint("Master call playback started");

        // Wait for playback to finish
        std::cout << "Playing master call: ";
        while (engine->isPlaying(currentSessionId)) {
            std::cout << "♪ " << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << "\nMaster call playback completed." << std::endl;

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::AUDIO_ENGINE,
                                                       huntmaster::DebugLevel::INFO,
                                                       "Master call playback completed: " + callId);
        }
    }

    void recordAndCompare() {
        PerformanceMonitor monitor("Record and compare", g_debugOptions.enablePerformanceMetrics);

        std::cout << "Enter master call ID to compare against: ";
        std::string callId;
        std::getline(std::cin, callId);

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                "Starting record and compare with master: " + callId);
        }

        // Create session if needed
        if (!sessionActive) {
            auto sessionResult = engine->startRealtimeSession(44100.0f, 512);
            if (!sessionResult.isOk()) {
                std::cout << "Failed to create real-time session!" << std::endl;
                return;
            }
            currentSessionId = sessionResult.value;
            sessionActive = true;
        }

        auto loadStatus = engine->loadMasterCall(currentSessionId, callId);
        if (loadStatus != UnifiedAudioEngine::Status::OK) {
            std::cout << "Failed to load master call: " << callId << std::endl;
            return;
        }
        monitor.checkpoint("Master call loaded for comparison");

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "Master call loaded for comparison: " + callId +
                    " for session: " + std::to_string(currentSessionId));
        }

        // Play master first
        std::cout << "\nPlaying master call..." << std::endl;
        auto playStatus = engine->playMasterCall(currentSessionId, callId);
        if (playStatus == UnifiedAudioEngine::Status::OK) {
            while (engine->isPlaying(currentSessionId)) {
                std::cout << "♪ " << std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            std::cout << std::endl;
        } else {
            std::cout << "Failed to play master call, but continuing with recording..."
                      << std::endl;
        }

        monitor.checkpoint("Master call played");

        // Record user attempt
        std::cout << "\nNow imitate the call! Recording in 3... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "2... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "1... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "GO!" << std::endl;

        auto startRecResult = engine->startRecording(currentSessionId);
        if (startRecResult != UnifiedAudioEngine::Status::OK) {
            std::cout << "Failed to start recording!" << std::endl;
            return;
        }

        monitor.checkpoint("Real-time recording started");

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "Real-time recording started for session: " + std::to_string(currentSessionId));
        }

        // Record for 3 seconds with real-time level monitoring
        auto start = std::chrono::steady_clock::now();
        int analysisUpdates = 0;
        std::cout << "Recording with live monitoring:" << std::endl;
        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
            auto levelResult = engine->getRecordingLevel(currentSessionId);
            if (levelResult.isOk()) {
                printLevel(levelResult.value);
            } else {
                std::cout << "●" << std::flush;
            }
            analysisUpdates++;

            if (g_debugOptions.enableAnalysisDebug && analysisUpdates % 20 == 0) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::TRACE,
                    "Real-time analysis update " + std::to_string(analysisUpdates));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cout << std::endl;

        auto stopResult = engine->stopRecording(currentSessionId);
        if (stopResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Warning: Failed to stop recording: " << statusToString(stopResult)
                      << std::endl;
        }

        monitor.checkpoint("Recording completed");

        // Save the recording
        std::string comparisonFilename = "comparison_attempt_" + callId + ".wav";
        auto saveResult = engine->saveRecording(currentSessionId, comparisonFilename);
        if (saveResult.isOk()) {
            lastRecordingFile = comparisonFilename;
            std::cout << "Recording saved as: " << comparisonFilename << std::endl;

            // Get similarity score for the comparison
            auto scoreResult = engine->getSimilarityScore(currentSessionId);
            if (scoreResult.isOk()) {
                std::cout << "Similarity Score: " << std::fixed << std::setprecision(5)
                          << scoreResult.value << std::endl;

                if (scoreResult.value > 0.01f) {
                    std::cout << "Great job! Good similarity to the master call." << std::endl;
                } else if (scoreResult.value > 0.005f) {
                    std::cout << "Not bad! Keep practicing to improve similarity." << std::endl;
                } else {
                    std::cout << "Keep practicing! Try listening to the master call again."
                              << std::endl;
                }
            } else {
                std::cout << "Recording saved, but similarity analysis failed." << std::endl;
            }
        } else {
            std::cout << "Recording completed but save failed!" << std::endl;
        }

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "Comparison analysis completed with " + std::to_string(analysisUpdates) +
                    " updates");
        }

        monitor.checkpoint("Comparison analysis completed");
    }

    void run() {
        PerformanceMonitor monitor("Interactive recorder session",
                                   g_debugOptions.enablePerformanceMetrics);

        bool running = true;
        int menuSelections = 0;

        while (running) {
            showMenu();

            int choice;
            std::cin >> choice;
            std::cin.ignore();  // Clear newline

            menuSelections++;

            if (g_debugOptions.enableVerbose) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                    "Menu selection " + std::to_string(menuSelections) + ": " +
                        std::to_string(choice));
            }

            switch (choice) {
                case 1:
                    recordAudio();
                    break;

                case 2:
                    playLastRecording();
                    break;

                case 3:
                    loadAndPlayMaster();
                    break;

                case 4:
                    recordAndCompare();
                    break;

                case 5:
                    running = false;
                    huntmaster::DebugLogger::getInstance().log(
                        huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                        "User selected exit after " + std::to_string(menuSelections) +
                            " menu interactions");
                    break;

                default:
                    std::cout << "Invalid choice!" << std::endl;
                    huntmaster::DebugLogger::getInstance().log(
                        huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::WARN,
                        "Invalid menu choice: " + std::to_string(choice));
                    break;
            }
        }

        monitor.checkpoint("Interactive session completed");
    }

    void cleanup() {
        if (sessionActive) {
            if (engine->isRecording(currentSessionId)) {
                auto stopRecordingResult = engine->stopRecording(currentSessionId);
                if (stopRecordingResult != UnifiedAudioEngine::Status::OK) {
                    std::cerr << "Warning: Failed to stop recording during cleanup: "
                              << statusToString(stopRecordingResult) << std::endl;
                }
            }
            if (engine->isPlaying(currentSessionId)) {
                auto stopPlaybackResult = engine->stopPlayback(currentSessionId);
                if (stopPlaybackResult != UnifiedAudioEngine::Status::OK) {
                    std::cerr << "Warning: Failed to stop playback during cleanup: "
                              << statusToString(stopPlaybackResult) << std::endl;
                }
            }
            if (engine->isRealtimeSession(currentSessionId)) {
                auto endSessionResult = engine->endRealtimeSession(currentSessionId);
                if (endSessionResult != UnifiedAudioEngine::Status::OK) {
                    std::cerr << "Warning: Failed to end realtime session during cleanup: "
                              << statusToString(endSessionResult) << std::endl;
                }
            } else {
                auto destroySessionResult = engine->destroySession(currentSessionId);
                if (destroySessionResult != UnifiedAudioEngine::Status::OK) {
                    std::cerr << "Warning: Failed to destroy session during cleanup: "
                              << statusToString(destroySessionResult) << std::endl;
                }
            }
            sessionActive = false;
        }

        huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                   huntmaster::DebugLevel::INFO,
                                                   "InteractiveRecorder cleanup completed");
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
        logger.setComponentLogLevel(huntmaster::DebugComponent::AUDIO_ENGINE,
                                    huntmaster::DebugLevel::DEBUG);
    }
    if (g_debugOptions.enableRecordingDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::AUDIO_ENGINE,
                                    huntmaster::DebugLevel::TRACE);
    }
    if (g_debugOptions.enablePlaybackDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::AUDIO_ENGINE,
                                    huntmaster::DebugLevel::DEBUG);
    }
    if (g_debugOptions.enableAnalysisDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::FEATURE_EXTRACTION,
                                    huntmaster::DebugLevel::DEBUG);
        logger.setComponentLogLevel(huntmaster::DebugComponent::SIMILARITY_ANALYSIS,
                                    huntmaster::DebugLevel::DEBUG);
    }
    if (g_debugOptions.enablePerformanceMetrics) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::PERFORMANCE,
                                    huntmaster::DebugLevel::DEBUG);
    }

    huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::INFO,
                                               "=== Interactive Recorder Tool Started ===");

    try {
        PerformanceMonitor totalMonitor("Complete interactive recorder session",
                                        g_debugOptions.enablePerformanceMetrics);

        if (g_debugOptions.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::AUDIO_ENGINE,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Creating UnifiedAudioEngine");
        }

        InteractiveRecorder recorder;
        totalMonitor.checkpoint("Engine initialized");

        if (g_debugOptions.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::AUDIO_ENGINE,
                                                       huntmaster::DebugLevel::INFO,
                                                       "UnifiedAudioEngine created successfully");
        }

        recorder.run();
        recorder.cleanup();

        totalMonitor.checkpoint("Interactive recorder session completed");

        huntmaster::DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
            "=== Interactive Recorder Tool Completed Successfully ===");

    } catch (const std::exception& e) {
        std::cerr << "❌ An unexpected error occurred: " << e.what() << std::endl;
        huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                   huntmaster::DebugLevel::ERROR,
                                                   "Exception occurred: " + std::string(e.what()));
        return 1;
    }

    return 0;
}