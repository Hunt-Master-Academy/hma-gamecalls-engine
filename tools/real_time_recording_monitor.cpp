#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "huntmaster/core/DebugConfig.h"
#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/HuntmasterAudioEngine.h"

using namespace huntmaster;
using huntmaster::Component;
using huntmaster::DebugConfig;
using huntmaster::DebugLogger;
using huntmaster::LogLevel;

/**
 * @brief Enhanced real-time recording monitor with comprehensive debugging
 */
class RealTimeRecordingMonitor {
   private:
    HuntmasterAudioEngine& engine_;
    bool verbose_;
    bool trace_;
    bool enableMetrics_;

    // Performance metrics
    std::vector<float> levelHistory_;
    std::chrono::steady_clock::time_point startTime_;

   public:
    RealTimeRecordingMonitor(bool verbose = false, bool trace = false, bool enableMetrics = false)
        : engine_(HuntmasterAudioEngine::getInstance()),
          verbose_(verbose),
          trace_(trace),
          enableMetrics_(enableMetrics) {
        // Configure debug logging
        if (trace) {
            DebugConfig::setupFullDebug();
        } else if (verbose) {
            DebugConfig::setupEngineDebug();
        } else {
            DebugConfig::enableDebugLogging(LogLevel::INFO);
        }

        LOG_INFO(Component::TOOLS, "RealTimeRecordingMonitor initialized");
        LOG_DEBUG(Component::TOOLS, "Verbose: " + std::string(verbose ? "enabled" : "disabled"));
        LOG_DEBUG(Component::TOOLS, "Trace: " + std::string(trace ? "enabled" : "disabled"));
        LOG_DEBUG(Component::TOOLS,
                  "Metrics: " + std::string(enableMetrics ? "enabled" : "disabled"));
    }

    void showRecordingLevels(int durationSeconds = 10) {
        LOG_INFO(Component::TOOLS, "=== Real-Time Recording Monitor (Enhanced) ===");
        LOG_INFO(Component::TOOLS,
                 "Recording duration: " + std::to_string(durationSeconds) + " seconds");

        if (!initializeEngine()) {
            return;
        }

        // Countdown
        showCountdown();

        // Start recording
        int recordingId = startRecording();
        if (recordingId < 0) {
            return;
        }

        // Monitor recording
        monitorRecording(recordingId, durationSeconds);

        // Stop and analyze
        stopAndAnalyzeRecording(recordingId);

        // Cleanup
        cleanupEngine();
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

    void showCountdown() {
        LOG_INFO(Component::TOOLS, "Starting recording countdown...");

        for (int i = 3; i >= 1; --i) {
            std::cout << "Starting in " << i << "..." << std::endl;
            LOG_DEBUG(Component::TOOLS, "Countdown: " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << "🎙️  RECORDING!" << std::endl << std::endl;
        LOG_INFO(Component::TOOLS, "Recording started");
    }

    int startRecording() {
        LOG_DEBUG(Component::TOOLS, "Starting audio recording");

        const float sampleRate = 44100.0f;
        int recordingId = engine_.startRecording(sampleRate);

        if (recordingId < 0) {
            LOG_ERROR(Component::TOOLS, "❌ Failed to start recording");
            return -1;
        }

        LOG_INFO(Component::TOOLS, "✅ Recording started with ID: " + std::to_string(recordingId));
        LOG_DEBUG(Component::TOOLS, "Sample rate: " + std::to_string(sampleRate) + " Hz");

        startTime_ = std::chrono::steady_clock::now();
        levelHistory_.clear();

        return recordingId;
    }

    void monitorRecording(int recordingId, int durationSeconds) {
        LOG_DEBUG(Component::TOOLS, "Starting real-time monitoring for " +
                                        std::to_string(durationSeconds) + " seconds");

        auto endTime = std::chrono::steady_clock::now() + std::chrono::seconds(durationSeconds);

        float peakLevel = 0.0f;
        float avgLevel = 0.0f;
        int sampleCount = 0;

        // Performance metrics
        auto lastUpdateTime = std::chrono::steady_clock::now();
        int updateCount = 0;

        while (std::chrono::steady_clock::now() < endTime) {
            auto currentTime = std::chrono::steady_clock::now();
            float level = engine_.getRecordingLevel();

            // Update statistics
            peakLevel = std::max(peakLevel, level);
            avgLevel += level;
            sampleCount++;

            // Store for metrics
            if (enableMetrics_) {
                levelHistory_.push_back(level);
            }

            // Display real-time level bar
            displayLevelBar(level, currentTime);

            // Trace logging
            if (trace_ && updateCount % 20 == 0) {
                LOG_TRACE(Component::TOOLS, "Level: " + std::to_string(level) +
                                                ", Peak: " + std::to_string(peakLevel) +
                                                ", Avg: " + std::to_string(avgLevel / sampleCount));
            }

            updateCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::cout << std::endl << std::endl;

        // Log final statistics
        avgLevel /= sampleCount;
        LOG_INFO(Component::TOOLS, "Recording monitoring completed");
        LOG_INFO(Component::TOOLS, "Peak level: " + std::to_string(peakLevel * 100.0f) + "%");
        LOG_INFO(Component::TOOLS, "Average level: " + std::to_string(avgLevel * 100.0f) + "%");
        LOG_DEBUG(Component::TOOLS, "Total samples: " + std::to_string(sampleCount));
        LOG_DEBUG(Component::TOOLS,
                  "Update rate: " +
                      std::to_string(updateCount / static_cast<float>(durationSeconds)) + " Hz");

        // Performance analysis
        if (enableMetrics_) {
            analyzePerformanceMetrics(avgLevel, peakLevel);
        }
    }

    void displayLevelBar(float level, std::chrono::steady_clock::time_point currentTime) {
        const int barLength = 50;
        int filledBars = static_cast<int>(level * barLength);

        std::cout << "\r[";

        // Color-coded level bar
        for (int i = 0; i < barLength; ++i) {
            if (i < filledBars) {
                if (level > 0.9f) {
                    std::cout << "!";  // Critical/clipping
                } else if (level > 0.7f) {
                    std::cout << "=";  // Loud
                } else if (level > 0.3f) {
                    std::cout << "-";  // Good
                } else {
                    std::cout << ".";  // Quiet
                }
            } else {
                std::cout << " ";
            }
        }

        // Level percentage and status
        std::cout << "] " << std::fixed << std::setprecision(1) << (level * 100) << "%";

        // Status indicator
        std::string status = getStatusString(level);
        std::cout << " " << status;

        // Time elapsed (if verbose)
        if (verbose_) {
            auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime_);
            std::cout << " [" << elapsed.count() << "s]";
        }

        std::cout << std::string(10, ' ');  // Clear any remaining characters
        std::cout.flush();
    }

    std::string getStatusString(float level) {
        if (level < 0.05f) {
            return "[Too Quiet]    ";
        } else if (level > 0.95f) {
            return "[🔴 CLIPPING!]  ";
        } else if (level > 0.9f) {
            return "[⚠️  Near Clip]   ";
        } else if (level > 0.7f) {
            return "[🟡 Loud]       ";
        } else if (level > 0.3f) {
            return "[🟢 Good]       ";
        } else if (level > 0.1f) {
            return "[🔵 Quiet]      ";
        } else {
            return "[⚪ Very Quiet] ";
        }
    }

    void analyzePerformanceMetrics(float avgLevel, float peakLevel) {
        LOG_DEBUG(Component::TOOLS, "=== Performance Metrics Analysis ===");

        if (levelHistory_.empty()) {
            LOG_WARN(Component::TOOLS, "No level history available for analysis");
            return;
        }

        // Calculate advanced statistics
        float variance = 0.0f;
        for (float level : levelHistory_) {
            variance += (level - avgLevel) * (level - avgLevel);
        }
        variance /= levelHistory_.size();
        float stdDev = std::sqrt(variance);

        // Calculate signal-to-noise ratio estimate
        float minLevel = *std::min_element(levelHistory_.begin(), levelHistory_.end());
        float snrEstimate =
            (avgLevel > 0.0f) ? 20.0f * std::log10(avgLevel / std::max(minLevel, 0.001f)) : 0.0f;

        // Count clipping events
        int clippingEvents = 0;
        for (float level : levelHistory_) {
            if (level > 0.95f) {
                clippingEvents++;
            }
        }

        LOG_DEBUG(Component::TOOLS, "Advanced Statistics:");
        LOG_DEBUG(Component::TOOLS, "  - Standard deviation: " + std::to_string(stdDev));
        LOG_DEBUG(Component::TOOLS, "  - Dynamic range: " + std::to_string(peakLevel - minLevel));
        LOG_DEBUG(Component::TOOLS, "  - SNR estimate: " + std::to_string(snrEstimate) + " dB");
        LOG_DEBUG(Component::TOOLS, "  - Clipping events: " + std::to_string(clippingEvents));
        LOG_DEBUG(Component::TOOLS, "  - Clipping rate: " +
                                        std::to_string(static_cast<float>(clippingEvents) /
                                                       levelHistory_.size() * 100.0f) +
                                        "%");

        // Generate recommendations
        generateRecommendations(avgLevel, peakLevel, stdDev, clippingEvents);
    }

    void generateRecommendations(float avgLevel, float peakLevel, float stdDev,
                                 int clippingEvents) {
        LOG_DEBUG(Component::TOOLS, "=== Recording Quality Recommendations ===");

        bool hasIssues = false;

        if (avgLevel < 0.1f) {
            LOG_WARN(Component::TOOLS, "⚠️  Recording level is too low");
            LOG_INFO(Component::TOOLS,
                     "💡 Recommendation: Increase microphone gain or move closer to source");
            hasIssues = true;
        }

        if (peakLevel > 0.95f) {
            LOG_WARN(Component::TOOLS, "⚠️  Recording level is too high (clipping detected)");
            LOG_INFO(Component::TOOLS,
                     "💡 Recommendation: Reduce microphone gain or move away from source");
            hasIssues = true;
        }

        if (clippingEvents > 0) {
            LOG_WARN(Component::TOOLS,
                     "⚠️  " + std::to_string(clippingEvents) + " clipping events detected");
            LOG_INFO(Component::TOOLS,
                     "💡 Recommendation: Reduce input gain to prevent distortion");
            hasIssues = true;
        }

        if (stdDev < 0.02f) {
            LOG_WARN(Component::TOOLS, "⚠️  Very low audio variation detected");
            LOG_INFO(Component::TOOLS,
                     "💡 Recommendation: Check if microphone is working properly");
            hasIssues = true;
        }

        if (!hasIssues) {
            LOG_INFO(Component::TOOLS, "✅ Recording quality looks good!");
        }
    }

    void stopAndAnalyzeRecording(int recordingId) {
        LOG_DEBUG(Component::TOOLS, "Stopping recording with ID: " + std::to_string(recordingId));

        engine_.stopRecording(recordingId);

        // Get save filename
        std::cout << "💾 Save recording as (without .wav extension): ";
        std::string filename;
        std::getline(std::cin, filename);

        if (filename.empty()) {
            filename = "recording_" +
                       std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            LOG_INFO(Component::TOOLS, "Using auto-generated filename: " + filename);
        }

        // Save recording
        saveRecording(recordingId, filename);
    }

    void saveRecording(int recordingId, const std::string& filename) {
        LOG_DEBUG(Component::TOOLS, "Saving recording to: " + filename);

        auto saveResult = engine_.saveRecording(recordingId, filename);
        if (!saveResult.isOk()) {
            LOG_ERROR(Component::TOOLS, "❌ Failed to save recording: " + filename);

            // Provide troubleshooting info
            LOG_INFO(Component::TOOLS, "💡 Troubleshooting:");
            LOG_INFO(Component::TOOLS, "  - Check if directory exists and is writable");
            LOG_INFO(Component::TOOLS, "  - Verify filename is valid");
            LOG_INFO(Component::TOOLS, "  - Check disk space");
            return;
        }

        std::string savedPath = saveResult.value();
        LOG_INFO(Component::TOOLS, "✅ Recording saved successfully to: " + savedPath);

        // Additional file information
        if (verbose_) {
            try {
                auto fileSize = std::filesystem::file_size(savedPath);
                LOG_DEBUG(Component::TOOLS, "File size: " + std::to_string(fileSize) + " bytes");
            } catch (const std::exception& e) {
                LOG_DEBUG(Component::TOOLS, "Could not get file size: " + std::string(e.what()));
            }
        }
    }

    void cleanupEngine() {
        LOG_DEBUG(Component::TOOLS, "Cleaning up engine resources");

        try {
            engine_.shutdown();
            LOG_INFO(Component::TOOLS, "✅ Engine shutdown completed");
        } catch (const std::exception& e) {
            LOG_WARN(Component::TOOLS, "⚠️  Engine shutdown warning: " + std::string(e.what()));
        }
    }
};

void printUsage() {
    std::cout << "Real-Time Recording Monitor - Enhanced Version\n\n";
    std::cout << "Usage: real_time_recording_monitor [options] [duration]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --verbose, -v      Enable verbose debugging output\n";
    std::cout << "  --trace, -t        Enable trace-level debugging\n";
    std::cout << "  --metrics, -m      Enable performance metrics analysis\n";
    std::cout << "  --help, -h         Show this help message\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  duration           Recording duration in seconds (default: 10)\n\n";
    std::cout << "Examples:\n";
    std::cout << "  real_time_recording_monitor\n";
    std::cout << "  real_time_recording_monitor --verbose 15\n";
    std::cout << "  real_time_recording_monitor --trace --metrics 30\n\n";
}

int main(int argc, char* argv[]) {
    bool verbose = false;
    bool trace = false;
    bool enableMetrics = false;
    int duration = 10;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--trace" || arg == "-t") {
            trace = true;
        } else if (arg == "--metrics" || arg == "-m") {
            enableMetrics = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else {
            // Try to parse as duration
            try {
                duration = std::stoi(arg);
                if (duration <= 0) {
                    std::cerr << "Error: Duration must be positive\n";
                    return 1;
                }
            } catch (const std::exception&) {
                std::cerr << "Error: Invalid duration: " << arg << "\n";
                return 1;
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

    LOG_INFO(Component::TOOLS, "=== Real-Time Recording Monitor (Enhanced) ===");
    LOG_DEBUG(Component::TOOLS, "Configuration:");
    LOG_DEBUG(Component::TOOLS, "  - Duration: " + std::to_string(duration) + " seconds");
    LOG_DEBUG(Component::TOOLS, "  - Verbose: " + std::string(verbose ? "enabled" : "disabled"));
    LOG_DEBUG(Component::TOOLS, "  - Trace: " + std::string(trace ? "enabled" : "disabled"));
    LOG_DEBUG(Component::TOOLS,
              "  - Metrics: " + std::string(enableMetrics ? "enabled" : "disabled"));

    // Create and run monitor
    RealTimeRecordingMonitor monitor(verbose, trace, enableMetrics);
    monitor.showRecordingLevels(duration);

    LOG_INFO(Component::TOOLS, "✅ Recording monitoring session completed");
    return 0;
}

int main(int argc, char* argv[]) {
    int duration = 10;
    if (argc > 1) {
        duration = std::atoi(argv[1]);
    }

    showRecordingLevels(duration);
    return 0;
}