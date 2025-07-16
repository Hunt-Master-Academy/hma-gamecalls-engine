#include <huntmaster/core/DebugConfig.h>
#include <huntmaster/core/DebugLogger.h>
#include <huntmaster/core/HuntmasterAudioEngine.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

using huntmaster::HuntmasterAudioEngine;

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
            huntmaster::DebugLogger::getInstance().log(huntmaster::Component::PERFORMANCE,
                                                       huntmaster::LogLevel::INFO,
                                                       "Starting: " + name);
        }
    }

    ~PerformanceMonitor() {
        if (enabled) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::PERFORMANCE, huntmaster::LogLevel::INFO,
                "Completed: " + name + " in " + std::to_string(duration.count()) + " μs");
        }
    }

    void checkpoint(const std::string& message) {
        if (enabled) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime);
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::PERFORMANCE, huntmaster::LogLevel::DEBUG,
                name + " checkpoint: " + message + " at " + std::to_string(duration.count()) +
                    " μs");
        }
    }
};

// Global debug options
DebugOptions g_debugOptions;

void printLevel(float level) {
    if (g_debugOptions.enableRecordingDebug) {
        huntmaster::DebugLogger::getInstance().log(huntmaster::Component::TOOLS,
                                                   huntmaster::LogLevel::TRACE,
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
    HuntmasterAudioEngine& engine;
    int lastRecordingId;
    std::string lastRecordingFile;

   public:
    InteractiveRecorder(HuntmasterAudioEngine& eng) : engine(eng), lastRecordingId(-1) {
        huntmaster::DebugLogger::getInstance().log(huntmaster::Component::TOOLS,
                                                   huntmaster::LogLevel::INFO,
                                                   "Interactive recorder initialized");
    }

    void recordAudio() {
        PerformanceMonitor monitor("Record audio", g_debugOptions.enablePerformanceMetrics);

        std::cout << "\nHow many seconds to record? ";
        int seconds;
        std::cin >> seconds;
        std::cin.ignore();

        if (g_debugOptions.enableRecordingDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::TOOLS, huntmaster::LogLevel::DEBUG,
                "Starting audio recording for " + std::to_string(seconds) + " seconds");
        }

        std::cout << "Starting recording in 3... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "2... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "1... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "GO!" << std::endl;

        lastRecordingId = engine.startRecording(44100.0);
        monitor.checkpoint("Recording started");

        if (g_debugOptions.enableRecordingDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::INFO,
                "Recording started with ID: " + std::to_string(lastRecordingId));
        }

        // Monitor levels
        auto start = std::chrono::steady_clock::now();
        int levelUpdates = 0;
        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(seconds)) {
            float level = engine.getRecordingLevel();
            printLevel(level);
            levelUpdates++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cout << std::endl;

        monitor.checkpoint("Level monitoring completed");

        engine.stopRecording(lastRecordingId);

        if (g_debugOptions.enableRecordingDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::INFO,
                "Recording stopped after " + std::to_string(levelUpdates) + " level updates");
        }

        std::cout << "Enter filename (without .wav): ";
        std::string filename;
        std::getline(std::cin, filename);

        lastRecordingFile = engine.saveRecording(lastRecordingId, filename);
        std::cout << "Saved to: " << lastRecordingFile << std::endl;

        monitor.checkpoint("Recording saved");

        if (g_debugOptions.enableRecordingDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::Component::AUDIO_ENGINE,
                                                       huntmaster::LogLevel::INFO,
                                                       "Recording saved to: " + lastRecordingFile);
        }
    }

    void playLastRecording() {
        PerformanceMonitor monitor("Play last recording", g_debugOptions.enablePerformanceMetrics);

        if (lastRecordingFile.empty()) {
            std::cout << "No recording available!" << std::endl;

            if (g_debugOptions.enablePlaybackDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::Component::TOOLS, huntmaster::LogLevel::WARN,
                    "Attempted to play recording but none available");
            }
            return;
        }

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::Component::TOOLS,
                                                       huntmaster::LogLevel::DEBUG,
                                                       "Playing recording: " + lastRecordingFile);
        }

        std::cout << "Playing: " << lastRecordingFile << std::endl;
        auto status = engine.playRecording(lastRecordingFile);

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::DEBUG,
                "Playback status: " + std::to_string(static_cast<int>(status)));
        }

        monitor.checkpoint("Playback started");

        // Wait for playback to finish (simple approach)
        std::this_thread::sleep_for(std::chrono::seconds(5));
        engine.stopPlayback();

        monitor.checkpoint("Playback stopped");

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::INFO,
                "Playback completed for: " + lastRecordingFile);
        }
    }

    void loadAndPlayMaster() {
        PerformanceMonitor monitor("Load and play master", g_debugOptions.enablePerformanceMetrics);

        std::cout << "Enter master call ID (e.g., 'buck_grunt'): ";
        std::string callId;
        std::getline(std::cin, callId);

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::Component::TOOLS,
                                                       huntmaster::LogLevel::DEBUG,
                                                       "Loading master call: " + callId);
        }

        auto loadStatus = engine.loadMasterCall(callId);
        monitor.checkpoint("Master call loaded");

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::INFO,
                "Master call loaded: " + callId +
                    ", Status: " + std::to_string(static_cast<int>(loadStatus)));
        }

        auto playStatus = engine.playMasterCall(callId);
        monitor.checkpoint("Master call playback started");

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::INFO,
                "Master call playback started: " + callId +
                    ", Status: " + std::to_string(static_cast<int>(playStatus)));
        }
        std::this_thread::sleep_for(std::chrono::seconds(3));

        if (g_debugOptions.enablePlaybackDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::Component::AUDIO_ENGINE,
                                                       huntmaster::LogLevel::INFO,
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
                huntmaster::Component::TOOLS, huntmaster::LogLevel::DEBUG,
                "Starting record and compare with master: " + callId);
        }

        auto loadStatus = engine.loadMasterCall(callId);
        monitor.checkpoint("Master call loaded for comparison");

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::INFO,
                "Master call loaded for comparison: " + callId +
                    ", Status: " + std::to_string(static_cast<int>(loadStatus)));
        }

        // Play master first
        std::cout << "\nPlaying master call..." << std::endl;
        auto playStatus = engine.playMasterCall(callId);

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::INFO,
                "Master call play status: " + std::to_string(static_cast<int>(playStatus)));
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));

        monitor.checkpoint("Master call played");

        // Record user attempt
        std::cout << "\nNow imitate the call! Recording in 3... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "2... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "1... ";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "GO!" << std::endl;

        int sessionId = engine.startRealtimeSession(44100.0, 1024);
        lastRecordingId = engine.startRecording(44100.0);

        monitor.checkpoint("Real-time session and recording started");

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::INFO,
                "Real-time session started with ID: " + std::to_string(sessionId) +
                    ", Recording ID: " + std::to_string(lastRecordingId));
        }

        // Record for 3 seconds with real-time analysis
        auto start = std::chrono::steady_clock::now();
        int analysisUpdates = 0;
        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
            float level = engine.getRecordingLevel();
            printLevel(level);
            analysisUpdates++;

            // In a real implementation, you'd feed audio chunks here
            // float score = engine.getSimilarityScore(sessionId);

            if (g_debugOptions.enableAnalysisDebug && analysisUpdates % 20 == 0) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::TRACE,
                    "Real-time analysis update " + std::to_string(analysisUpdates) +
                        ", Level: " + std::to_string(level));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        std::cout << std::endl;

        engine.stopRecording(lastRecordingId);
        engine.endRealtimeSession(sessionId);

        monitor.checkpoint("Recording and analysis completed");

        lastRecordingFile = engine.saveRecording(lastRecordingId, "comparison_attempt");
        std::cout << "Recording saved. Analysis complete!" << std::endl;

        if (g_debugOptions.enableAnalysisDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::INFO,
                "Comparison recording saved: " + lastRecordingFile + " with " +
                    std::to_string(analysisUpdates) + " analysis updates");
        }

        // TODO: Display similarity score when real-time analysis is fully implemented
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
                    huntmaster::Component::TOOLS, huntmaster::LogLevel::DEBUG,
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
                        huntmaster::Component::TOOLS, huntmaster::LogLevel::INFO,
                        "User selected exit after " + std::to_string(menuSelections) +
                            " menu interactions");
                    break;

                default:
                    std::cout << "Invalid choice!" << std::endl;
                    huntmaster::DebugLogger::getInstance().log(
                        huntmaster::Component::TOOLS, huntmaster::LogLevel::WARN,
                        "Invalid menu choice: " + std::to_string(choice));
                    break;
            }
        }

        monitor.checkpoint("Interactive session completed");
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
    if (g_debugOptions.enableRecordingDebug) {
        logger.setComponentLogLevel(huntmaster::Component::AUDIO_ENGINE,
                                    huntmaster::LogLevel::TRACE);
    }
    if (g_debugOptions.enablePlaybackDebug) {
        logger.setComponentLogLevel(huntmaster::Component::AUDIO_ENGINE,
                                    huntmaster::LogLevel::DEBUG);
    }
    if (g_debugOptions.enableAnalysisDebug) {
        logger.setComponentLogLevel(huntmaster::Component::FEATURE_EXTRACTION,
                                    huntmaster::LogLevel::DEBUG);
        logger.setComponentLogLevel(huntmaster::Component::SIMILARITY_ANALYSIS,
                                    huntmaster::LogLevel::DEBUG);
    }
    if (g_debugOptions.enablePerformanceMetrics) {
        logger.setComponentLogLevel(huntmaster::Component::PERFORMANCE,
                                    huntmaster::LogLevel::DEBUG);
    }

    huntmaster::DebugLogger::getInstance().log(huntmaster::Component::TOOLS,
                                               huntmaster::LogLevel::INFO,
                                               "=== Interactive Recorder Tool Started ===");

    try {
        PerformanceMonitor totalMonitor("Complete interactive recorder session",
                                        g_debugOptions.enablePerformanceMetrics);

        HuntmasterAudioEngine& engine = HuntmasterAudioEngine::getInstance();

        if (g_debugOptions.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::Component::AUDIO_ENGINE,
                                                       huntmaster::LogLevel::DEBUG,
                                                       "Initializing HuntmasterAudioEngine");
        }

        engine.initialize();
        totalMonitor.checkpoint("Engine initialized");

        if (g_debugOptions.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::Component::AUDIO_ENGINE, huntmaster::LogLevel::INFO,
                "HuntmasterAudioEngine initialized successfully");
        }

        InteractiveRecorder recorder(engine);
        recorder.run();

        totalMonitor.checkpoint("Interactive recorder session completed");

        if (g_debugOptions.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::Component::AUDIO_ENGINE,
                                                       huntmaster::LogLevel::DEBUG,
                                                       "Shutting down HuntmasterAudioEngine");
        }

        engine.shutdown();

        if (g_debugOptions.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::Component::AUDIO_ENGINE,
                                                       huntmaster::LogLevel::INFO,
                                                       "HuntmasterAudioEngine shutdown completed");
        }

        huntmaster::DebugLogger::getInstance().log(
            huntmaster::Component::TOOLS, huntmaster::LogLevel::INFO,
            "=== Interactive Recorder Tool Completed Successfully ===");

    } catch (const std::exception& e) {
        std::cerr << "❌ An unexpected error occurred: " << e.what() << std::endl;
        huntmaster::DebugLogger::getInstance().log(huntmaster::Component::TOOLS,
                                                   huntmaster::LogLevel::ERROR,
                                                   "Exception occurred: " + std::string(e.what()));
        return 1;
    }

    return 0;
}