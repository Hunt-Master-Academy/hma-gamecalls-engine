#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "huntmaster/core/DebugConfig.h"
#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using huntmaster::DebugConfig;
using huntmaster::DebugLogger;
using huntmaster::UnifiedAudioEngine;

// Debug options structure
struct DebugOptions {
    bool enableDebug = false;
    bool enableTrace = false;
    bool enableVerbose = false;
    bool enablePerformanceMetrics = false;
    bool enableEngineDebug = false;
    bool enableFeatureDebug = false;
    bool enableBatchDebug = false;
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
            } else if (arg == "--performance" || arg == "-p") {
                enablePerformanceMetrics = true;
            } else if (arg == "--engine-debug") {
                enableEngineDebug = true;
            } else if (arg == "--feature-debug") {
                enableFeatureDebug = true;
            } else if (arg == "--batch-debug") {
                enableBatchDebug = true;
            } else if (arg == "--help" || arg == "-h") {
                printHelp = true;
            }
        }
    }

    void printUsage(const char* programName) {
        std::cout << "=== MFCC Feature Generator ===" << std::endl;
        std::cout << "Usage: " << programName << " [options] [call_names...]" << std::endl;
        std::cout << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  call_names       Specific call names to process (optional)" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --debug, -d      Enable debug logging" << std::endl;
        std::cout << "  --trace, -t      Enable trace logging" << std::endl;
        std::cout << "  --verbose, -v    Enable verbose output" << std::endl;
        std::cout << "  --performance, -p Enable performance metrics" << std::endl;
        std::cout << "  --engine-debug   Enable engine debugging" << std::endl;
        std::cout << "  --feature-debug  Enable feature extraction debugging" << std::endl;
        std::cout << "  --batch-debug    Enable batch processing debugging" << std::endl;
        std::cout << "  --help, -h       Show this help message" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << programName << "                           # Process all default calls"
                  << std::endl;
        std::cout << "  " << programName << " --debug --performance      # Process with debug info"
                  << std::endl;
        std::cout << "  " << programName << " buck_grunt doe-grunt       # Process specific calls"
                  << std::endl;
        std::cout << "  " << programName << " --feature-debug buck_grunt # Debug specific call"
                  << std::endl;
    }
};

// Performance monitoring class
class PerformanceMonitor {
   private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::string operationName;
    bool enabled;

   public:
    PerformanceMonitor(const std::string& name, bool enable = true)
        : operationName(name), enabled(enable) {
        if (enabled) {
            startTime = std::chrono::high_resolution_clock::now();
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::INFO,
                                           "Starting " + operationName);
        }
    }

    ~PerformanceMonitor() {
        if (enabled) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                operationName + " completed in " + std::to_string(duration.count()) + "ms");
        }
    }

    void checkpoint(const std::string& message) {
        if (enabled) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);

            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                operationName + " - " + message + " (+" + std::to_string(duration.count()) + "ms)");
        }
    }
};

// Feature generation class
class FeatureGenerator {
   private:
    std::unique_ptr<UnifiedAudioEngine> engine;
    DebugOptions& options;

   public:
    FeatureGenerator(DebugOptions& opts) : options(opts) {
        auto engineResult = UnifiedAudioEngine::create();
        if (!engineResult.isOk()) {
            throw std::runtime_error("Failed to create UnifiedAudioEngine");
        }
        engine = std::move(engineResult.value);
    }
    bool processCall(const std::string& callName) {
        PerformanceMonitor monitor("Processing call: " + callName,
                                   options.enablePerformanceMetrics);

        if (options.enableFeatureDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "Starting feature generation for: " + callName);
        }

        // Check if audio file exists
        std::string audioPath = "../data/master_calls/" + callName + ".wav";
        if (!std::filesystem::exists(audioPath)) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::ERROR,
                                           "Audio file not found: " + audioPath);
            std::cerr << "Warning: Audio file not found: " << audioPath << std::endl;
            return false;
        }

        if (options.enableFeatureDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "Audio file found: " + audioPath);
            monitor.checkpoint("Audio file validated");
        }

        try {
            // Create session for processing
            auto sessionResult = engine->createSession();
            if (!sessionResult.isOk()) {
                std::cout << "  ✗ Failed to create session for: " << callName << std::endl;
                return false;
            }

            uint32_t sessionId = sessionResult.value;

            // Load master call - this will generate features
            std::cout << "Processing: " << callName << std::endl;
            auto loadResult = engine->loadMasterCall(sessionId, callName);
            if (loadResult != UnifiedAudioEngine::Status::OK) {
                std::cout << "  ✗ Failed to load master call: " << callName << std::endl;
                auto destroyResult = engine->destroySession(sessionId);
                if (destroyResult != UnifiedAudioEngine::Status::OK) {
                    // Log warning but continue since we're in error path
                }
                return false;
            }

            if (options.enableFeatureDebug) {
                DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::INFO,
                                               "Successfully loaded master call: " + callName);
                monitor.checkpoint("Master call loaded");
            }

            // Clean up session
            auto destroyResult = engine->destroySession(sessionId);
            if (destroyResult != UnifiedAudioEngine::Status::OK) {
                if (options.enableFeatureDebug) {
                    DebugLogger::getInstance().log(
                        huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::WARN,
                        "Warning: Failed to destroy session for: " + callName);
                }
            }

            // Check if feature file was generated
            std::string featurePath = "../data/features/" + callName + ".mfcc";
            if (std::filesystem::exists(featurePath)) {
                if (options.enableFeatureDebug) {
                    auto fileSize = std::filesystem::file_size(featurePath);
                    DebugLogger::getInstance().log(
                        huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                        "Feature file generated: " + featurePath +
                            " (Size: " + std::to_string(fileSize) + " bytes)");
                }
                std::cout << "  ✓ Features generated: " << featurePath << std::endl;
            } else {
                if (options.enableFeatureDebug) {
                    DebugLogger::getInstance().log(
                        huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::WARN,
                        "Feature file not found after processing: " + featurePath);
                }
                std::cout << "  ⚠ Feature file not found: " << featurePath << std::endl;
            }

            return true;

        } catch (const std::exception& e) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::ERROR,
                "Exception during feature generation for " + callName + ": " + e.what());
            std::cerr << "Error processing " << callName << ": " << e.what() << std::endl;
            return false;
        }
    }
};

int main(int argc, char* argv[]) {
    // Parse debug options
    DebugOptions debugOptions;
    debugOptions.parseArgs(argc, argv);

    if (debugOptions.printHelp) {
        debugOptions.printUsage(argv[0]);
        return 0;
    }

    // Set up debugging based on options
    if (debugOptions.enableTrace) {
        DebugConfig::setupFullDebug();
    } else if (debugOptions.enableDebug) {
        DebugConfig::setupToolsDebug();
    }

    // Configure component-specific debug levels
    auto& logger = DebugLogger::getInstance();
    if (debugOptions.enableEngineDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::AUDIO_ENGINE,
                                    huntmaster::DebugLevel::DEBUG);
    }
    if (debugOptions.enableFeatureDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::FEATURE_EXTRACTION,
                                    huntmaster::DebugLevel::DEBUG);
    }
    if (debugOptions.enableBatchDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::TOOLS,
                                    huntmaster::DebugLevel::TRACE);
    }
    if (debugOptions.enablePerformanceMetrics) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::PERFORMANCE,
                                    huntmaster::DebugLevel::DEBUG);
    }

    DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                                   "=== MFCC Feature Generator Started ===");

    PerformanceMonitor totalMonitor("Total execution", debugOptions.enablePerformanceMetrics);

    // Determine which calls to process
    std::vector<std::string> callsToProcess;

    // Check if specific calls were provided as arguments
    bool foundCalls = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg[0] != '-') {  // Not a debug option
            callsToProcess.push_back(arg);
            foundCalls = true;
        }
    }

    // If no specific calls provided, use default list
    if (!foundCalls) {
        callsToProcess = {"breeding_bellow", "buck_grunt",     "buck_rage_grunts", "buck-bawl",
                          "contact-bleatr",  "doe-grunt",      "doebleat",         "estrus_bleat",
                          "fawn-bleat",      "sparring_bucks", "tending_grunts"};
    }

    if (debugOptions.enableBatchDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Processing " + std::to_string(callsToProcess.size()) + " calls");

        for (const auto& call : callsToProcess) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::TRACE,
                                           "Call to process: " + call);
        }
    }

    // Create feature generator
    FeatureGenerator generator(debugOptions);

    // Process calls
    int successCount = 0;
    int failureCount = 0;

    for (const auto& call : callsToProcess) {
        if (generator.processCall(call)) {
            successCount++;
        } else {
            failureCount++;
        }
    }

    totalMonitor.checkpoint("All calls processed");

    // Print summary
    std::cout << "\n=== PROCESSING SUMMARY ===" << std::endl;
    std::cout << "Total calls processed: " << callsToProcess.size() << std::endl;
    std::cout << "Successful: " << successCount << std::endl;
    std::cout << "Failed: " << failureCount << std::endl;

    if (failureCount == 0) {
        std::cout << "All features generated successfully!" << std::endl;
    } else {
        std::cout << "Some features failed to generate. Check logs for details." << std::endl;
    }

    DebugLogger::getInstance().log(
        huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
        "=== MFCC Feature Generator " +
            std::string(failureCount == 0 ? "Completed Successfully" : "Completed with Errors") +
            " ===");

    return failureCount == 0 ? 0 : 1;
}
