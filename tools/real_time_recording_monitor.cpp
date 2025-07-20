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
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;
using huntmaster::DebugComponent;
using huntmaster::DebugConfig;
using huntmaster::DebugLevel;
using huntmaster::DebugLogger;
using SessionId = uint32_t;

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

/**
 * @brief Enhanced real-time recording monitor with comprehensive debugging
 */
class RealTimeRecordingMonitor {
   private:
    std::unique_ptr<UnifiedAudioEngine> engine_;
    SessionId sessionId_;
    bool verbose_;
    bool trace_;
    bool enableMetrics_;

    // Performance metrics
    std::vector<float> levelHistory_;
    std::chrono::steady_clock::time_point startTime_;

   public:
    RealTimeRecordingMonitor(bool verbose = false, bool trace = false, bool enableMetrics = false)
        : verbose_(verbose), trace_(trace), enableMetrics_(enableMetrics) {
        // Configure debug logging
        if (enableMetrics) {
            DebugConfig::enableDebugLogging(DebugLevel::INFO);
        }

        LOG_INFO(DebugComponent::TOOLS, "RealTimeRecordingMonitor initialized");
        LOG_DEBUG(DebugComponent::TOOLS,
                  "Verbose: " + std::string(verbose ? "enabled" : "disabled"));
        LOG_DEBUG(DebugComponent::TOOLS, "Trace: " + std::string(trace ? "enabled" : "disabled"));
        LOG_DEBUG(DebugComponent::TOOLS,
                  "Performance metrics: " + std::string(enableMetrics ? "enabled" : "disabled"));
    }

    void showRecordingLevels(int durationSeconds = 10) {
        LOG_INFO(DebugComponent::TOOLS, "=== Real-Time Recording Monitor (Enhanced) ===");
        LOG_INFO(DebugComponent::TOOLS,
                 "Recording duration: " + std::to_string(durationSeconds) + " seconds");

        if (!initializeEngine()) {
            return;
        }

        // Countdown
        showCountdown();

        // Start recording
        if (!startRecording()) {
            return;
        }

        // Monitor recording
        monitorRecording(durationSeconds);

        // Stop and analyze recording
        stopAndAnalyzeRecording();

        // Cleanup
        cleanupEngine();
    }

   private:
    bool initializeEngine() {
        LOG_DEBUG(DebugComponent::TOOLS, "Initializing UnifiedAudioEngine");

        try {
            auto engineResult = UnifiedAudioEngine::create();
            if (!engineResult.isOk()) {
                LOG_ERROR(DebugComponent::TOOLS,
                          "❌ Engine creation failed: " + statusToString(engineResult.error()));
                return false;
            }
            engine_ = std::move(engineResult.value);

            // Start realtime session
            auto sessionResult = engine_->startRealtimeSession(44100.0f);
            if (!sessionResult.isOk()) {
                LOG_ERROR(DebugComponent::TOOLS, "❌ Failed to start realtime session: " +
                                                     statusToString(sessionResult.error()));
                return false;
            }
            sessionId_ = sessionResult.value;

            LOG_INFO(DebugComponent::TOOLS, "✅ Engine initialized successfully");
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR(DebugComponent::TOOLS,
                      "❌ Engine initialization failed: " + std::string(e.what()));
            return false;
        }
    }

    void showCountdown() {
        LOG_INFO(DebugComponent::TOOLS, "Starting recording countdown...");

        for (int i = 3; i >= 1; --i) {
            std::cout << "Starting in " << i << "..." << std::endl;
            LOG_DEBUG(DebugComponent::TOOLS, "Countdown: " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << "🎙️  RECORDING!" << std::endl << std::endl;
        LOG_INFO(DebugComponent::TOOLS, "Recording started");
    }

    bool startRecording() {
        LOG_DEBUG(DebugComponent::TOOLS, "Starting audio recording");

        auto recordingResult = engine_->startRecording(sessionId_);
        if (recordingResult != UnifiedAudioEngine::Status::OK) {
            LOG_ERROR(DebugComponent::TOOLS, "❌ Failed to start recording");
            return false;
        }

        LOG_INFO(DebugComponent::TOOLS, "✅ Recording started successfully");
        LOG_DEBUG(DebugComponent::TOOLS, "Session ID: " + std::to_string(sessionId_));

        startTime_ = std::chrono::steady_clock::now();
        levelHistory_.clear();

        return true;
    }

    void monitorRecording(int durationSeconds) {
        LOG_DEBUG(DebugComponent::TOOLS, "Starting real-time monitoring for " +
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
            auto levelResult = engine_->getRecordingLevel(sessionId_);

            float level = 0.0f;
            if (levelResult.isOk()) {
                level = levelResult.value;
            } else {
                LOG_WARN(DebugComponent::TOOLS,
                         "Failed to get recording level: " + statusToString(levelResult.error()));
            }

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
                LOG_TRACE(DebugComponent::TOOLS,
                          "Level: " + std::to_string(level) +
                              ", Peak: " + std::to_string(peakLevel) +
                              ", Avg: " + std::to_string(avgLevel / sampleCount));
            }

            updateCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::cout << std::endl << std::endl;

        // Log final statistics
        avgLevel /= sampleCount;
        LOG_INFO(DebugComponent::TOOLS, "Recording monitoring completed");
        LOG_INFO(DebugComponent::TOOLS, "Peak level: " + std::to_string(peakLevel * 100.0f) + "%");
        LOG_INFO(DebugComponent::TOOLS,
                 "Average level: " + std::to_string(avgLevel * 100.0f) + "%");
        LOG_DEBUG(DebugComponent::TOOLS, "Total samples: " + std::to_string(sampleCount));
        LOG_DEBUG(DebugComponent::TOOLS,
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
                if (level > 0.8f) {
                    std::cout << "🔴";  // High level (red)
                } else if (level > 0.5f) {
                    std::cout << "🟡";  // Medium level (yellow)
                } else {
                    std::cout << "🟢";  // Normal level (green)
                }
            } else {
                std::cout << "⚫";  // Empty
            }
        }

        std::cout << "] " << std::fixed << std::setprecision(1) << (level * 100.0f) << "%";

        // Add timestamp if verbose
        if (verbose_) {
            auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime_).count();
            std::cout << " [" << elapsed << "s]";
        }

        std::cout.flush();
    }

    void analyzePerformanceMetrics(float avgLevel, float peakLevel) {
        LOG_DEBUG(DebugComponent::TOOLS, "=== Performance Metrics Analysis ===");

        if (levelHistory_.empty()) {
            LOG_WARN(DebugComponent::TOOLS, "No level history available for analysis");
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

        LOG_DEBUG(DebugComponent::TOOLS, "Advanced Statistics:");
        LOG_DEBUG(DebugComponent::TOOLS, "  - Standard deviation: " + std::to_string(stdDev));
        LOG_DEBUG(DebugComponent::TOOLS,
                  "  - Dynamic range: " + std::to_string(peakLevel - minLevel));
        LOG_DEBUG(DebugComponent::TOOLS,
                  "  - SNR estimate: " + std::to_string(snrEstimate) + " dB");
        LOG_DEBUG(DebugComponent::TOOLS, "  - Clipping events: " + std::to_string(clippingEvents));
        LOG_DEBUG(DebugComponent::TOOLS, "  - Clipping rate: " +
                                             std::to_string(static_cast<float>(clippingEvents) /
                                                            levelHistory_.size() * 100.0f) +
                                             "%");

        // Generate recommendations
        generateRecommendations(avgLevel, peakLevel, stdDev, clippingEvents);
    }

    void generateRecommendations(float avgLevel, float peakLevel, float stdDev,
                                 int clippingEvents) {
        LOG_DEBUG(DebugComponent::TOOLS, "=== Recording Quality Recommendations ===");

        bool hasIssues = false;

        if (avgLevel < 0.1f) {
            LOG_WARN(DebugComponent::TOOLS, "⚠️  Recording level is too low");
            LOG_INFO(DebugComponent::TOOLS,
                     "💡 Recommendation: Increase microphone gain or move closer to source");
            hasIssues = true;
        }

        if (peakLevel > 0.95f) {
            LOG_WARN(DebugComponent::TOOLS, "⚠️  Recording level is too high (clipping detected)");
            LOG_INFO(DebugComponent::TOOLS,
                     "💡 Recommendation: Reduce microphone gain or move away from source");
            hasIssues = true;
        }

        if (clippingEvents > 0) {
            LOG_WARN(DebugComponent::TOOLS,
                     "⚠️  " + std::to_string(clippingEvents) + " clipping events detected");
            LOG_INFO(DebugComponent::TOOLS,
                     "💡 Recommendation: Reduce input gain to prevent distortion");
            hasIssues = true;
        }

        if (stdDev < 0.02f) {
            LOG_WARN(DebugComponent::TOOLS, "⚠️  Very low audio variation detected");
            LOG_INFO(DebugComponent::TOOLS,
                     "💡 Recommendation: Check if microphone is working properly");
            hasIssues = true;
        }

        if (!hasIssues) {
            LOG_INFO(DebugComponent::TOOLS, "✅ Recording quality looks good!");
        }
    }

    void stopAndAnalyzeRecording() {
        LOG_DEBUG(DebugComponent::TOOLS, "Stopping recording");

        auto stopResult = engine_->stopRecording(sessionId_);
        if (stopResult != UnifiedAudioEngine::Status::OK) {
            LOG_ERROR(DebugComponent::TOOLS, "❌ Failed to stop recording");
        }

        // Get save filename
        std::cout << "💾 Save recording as (without .wav extension): ";
        std::string filename;
        std::getline(std::cin, filename);

        if (filename.empty()) {
            filename = "recording_" +
                       std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            LOG_INFO(DebugComponent::TOOLS, "Using auto-generated filename: " + filename);
        }

        // Save recording
        saveRecording(filename);
    }

    void saveRecording(const std::string& filename) {
        LOG_DEBUG(DebugComponent::TOOLS, "Saving recording to: " + filename);

        auto saveResult = engine_->saveRecording(sessionId_, filename);
        if (!saveResult.isOk()) {
            LOG_ERROR(DebugComponent::TOOLS, "❌ Failed to save recording: " + filename);

            // Provide troubleshooting info
            LOG_INFO(DebugComponent::TOOLS, "💡 Troubleshooting:");
            LOG_INFO(DebugComponent::TOOLS, "  - Check if directory exists and is writable");
            LOG_INFO(DebugComponent::TOOLS, "  - Verify filename is valid");
            LOG_INFO(DebugComponent::TOOLS, "  - Check disk space");
            return;
        }

        std::string savedPath = saveResult.value;
        LOG_INFO(DebugComponent::TOOLS, "✅ Recording saved successfully to: " + savedPath);

        // Additional file information
        if (verbose_) {
            try {
                auto fileSize = std::filesystem::file_size(savedPath);
                LOG_DEBUG(DebugComponent::TOOLS,
                          "File size: " + std::to_string(fileSize) + " bytes");
            } catch (const std::exception& e) {
                LOG_DEBUG(DebugComponent::TOOLS,
                          "Could not get file size: " + std::string(e.what()));
            }
        }
    }

    void cleanupEngine() {
        LOG_DEBUG(DebugComponent::TOOLS, "Cleaning up engine resources");

        try {
            if (sessionId_ != 0) {
                auto endResult = engine_->endRealtimeSession(sessionId_);
                if (endResult == UnifiedAudioEngine::Status::OK) {
                    LOG_INFO(DebugComponent::TOOLS, "✅ Session cleanup completed");
                } else {
                    LOG_WARN(DebugComponent::TOOLS, "⚠️  Session cleanup warning");
                }
            }
        } catch (const std::exception& e) {
            LOG_WARN(DebugComponent::TOOLS, "⚠️  Engine cleanup warning: " + std::string(e.what()));
        }
    }
};

void printUsage() {
    std::cout << "Real-Time Recording Monitor (Enhanced)\n";
    std::cout << "======================================\n\n";
    std::cout << "Usage: real_time_recording_monitor [OPTIONS] [DURATION]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --verbose, -v      Enable verbose output\n";
    std::cout << "  --trace, -t        Enable trace logging (most detailed)\n";
    std::cout << "  --metrics, -m      Enable performance metrics\n";
    std::cout << "  --help, -h         Show this help message\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  DURATION           Recording duration in seconds (default: 10)\n\n";
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
        logger.setGlobalLogLevel(DebugLevel::TRACE);
    } else if (verbose) {
        logger.setGlobalLogLevel(DebugLevel::DEBUG);
    } else {
        logger.setGlobalLogLevel(DebugLevel::INFO);
    }

    LOG_INFO(DebugComponent::TOOLS, "=== Real-Time Recording Monitor (Enhanced) ===");
    LOG_DEBUG(DebugComponent::TOOLS, "Configuration:");
    LOG_DEBUG(DebugComponent::TOOLS, "  - Duration: " + std::to_string(duration) + " seconds");
    LOG_DEBUG(DebugComponent::TOOLS,
              "  - Verbose: " + std::string(verbose ? "enabled" : "disabled"));
    LOG_DEBUG(DebugComponent::TOOLS, "  - Trace: " + std::string(trace ? "enabled" : "disabled"));
    LOG_DEBUG(DebugComponent::TOOLS,
              "  - Metrics: " + std::string(enableMetrics ? "enabled" : "disabled"));

    // Create and run monitor
    RealTimeRecordingMonitor monitor(verbose, trace, enableMetrics);
    monitor.showRecordingLevels(duration);

    LOG_INFO(DebugComponent::TOOLS, "✅ Recording monitoring session completed");
    return 0;
}
