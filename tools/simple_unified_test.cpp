/**
 * @file simple_unified_test.cpp
 * @brief Simple test program to verify UnifiedAudioEngine functionality
 *
 * This is a standalone test program that can be built and run to verify
 * that the UnifiedAudioEngine is working correctly.
 */

#include <huntmaster/core/DebugConfig.h>
#include <huntmaster/core/DebugLogger.h>
#include <huntmaster/core/UnifiedAudioEngine.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// Debug options for simple unified test
struct DebugOptions {
    bool enableDebug = false;
    bool enableTrace = false;
    bool enableVerbose = false;
    bool enableEngineDebug = false;
    bool enableSessionDebug = false;
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
            } else if (arg == "--session-debug") {
                enableSessionDebug = true;
            } else if (arg == "--performance" || arg == "-p") {
                enablePerformanceMetrics = true;
            } else if (arg == "--help" || arg == "-h") {
                printHelp = true;
            }
        }
    }

    void printUsage(const char* programName) {
        std::cout << "Usage: " << programName << " [OPTIONS]\n"
                  << "Simple test program to verify UnifiedAudioEngine functionality\n\n"
                  << "Options:\n"
                  << "  --debug, -d          Enable debug logging\n"
                  << "  --trace, -t          Enable trace logging (most verbose)\n"
                  << "  --verbose, -v        Enable verbose output\n"
                  << "  --engine-debug       Enable engine-specific debugging\n"
                  << "  --session-debug      Enable session-specific debugging\n"
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

using huntmaster::SessionId;
using huntmaster::UnifiedAudioEngine;
using Status = huntmaster::UnifiedAudioEngine::Status;

using huntmaster::SessionId;
using huntmaster::UnifiedAudioEngine;
using Status = huntmaster::UnifiedAudioEngine::Status;

// Enhanced test class with comprehensive debugging
class UnifiedEngineTestSuite {
   private:
    DebugOptions& options;

   public:
    UnifiedEngineTestSuite(DebugOptions& opts) : options(opts) {}

    void runAllTests() {
        PerformanceMonitor totalMonitor("Complete test suite", options.enablePerformanceMetrics);

        huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                   huntmaster::DebugLevel::INFO,
                                                   "Starting UnifiedAudioEngine test suite");

        std::cout << "=== UnifiedAudioEngine Test Suite ===" << std::endl;

        bool allPassed = true;

        allPassed &= testSingleSessionLifecycle();
        allPassed &= testMultipleSessions();
        allPassed &= testInvalidSession();
        allPassed &= testEngineCreation();
        allPassed &= testSessionQueries();

        totalMonitor.checkpoint("All tests completed");

        if (allPassed) {
            std::cout << "\n✅ All tests passed!" << std::endl;
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::INFO,
                                                       "All tests passed successfully");
        } else {
            std::cout << "\n❌ Some tests failed!" << std::endl;
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::ERROR,
                                                       "Some tests failed");
        }
    }

   private:
    bool testSingleSessionLifecycle() {
        PerformanceMonitor monitor("Single session lifecycle test",
                                   options.enablePerformanceMetrics);

        std::cout << "\n--- Testing Single Session Lifecycle ---" << std::endl;

        if (options.enableSessionDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Starting single session lifecycle test");
        }

        auto engineResult = UnifiedAudioEngine::create();
        if (!engineResult) {
            std::cerr << "✗ Failed to create engine: " << static_cast<int>(engineResult.error())
                      << std::endl;

            if (options.enableEngineDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Failed to create UnifiedAudioEngine: " +
                        std::to_string(static_cast<int>(engineResult.error())));
            }
            return false;
        }

        auto engine = std::move(*engineResult);
        monitor.checkpoint("Engine created");

        if (options.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::AUDIO_ENGINE,
                                                       huntmaster::DebugLevel::INFO,
                                                       "UnifiedAudioEngine created successfully");
        }

        // 1. Create Session
        if (options.enableVerbose) {
            std::cout << "  Creating session..." << std::endl;
        }

        auto sessionResult = engine->createSession();
        if (!sessionResult) {
            std::cerr << "✗ Failed to create session: " << static_cast<int>(sessionResult.error())
                      << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Failed to create session: " +
                        std::to_string(static_cast<int>(sessionResult.error())));
            }
            return false;
        }

        SessionId sessionId = *sessionResult;
        std::cout << "✓ Session created with ID: " << sessionId << std::endl;
        monitor.checkpoint("Session created");

        if (options.enableSessionDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "Session created successfully with ID: " + std::to_string(sessionId));
        }

        // 2. Verify Session Exists
        if (options.enableVerbose) {
            std::cout << "  Verifying session is active..." << std::endl;
        }

        if (!engine->isSessionActive(sessionId)) {
            std::cerr << "✗ Session " << sessionId << " should be active but is not." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Session " + std::to_string(sessionId) + " should be active but is not");
            }
            return false;
        } else {
            std::cout << "✓ Session " << sessionId << " is active." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::DEBUG,
                    "Session " + std::to_string(sessionId) + " is active as expected");
            }
        }

        monitor.checkpoint("Session verified active");

        // 3. Destroy Session
        if (options.enableVerbose) {
            std::cout << "  Destroying session..." << std::endl;
        }

        Status destroyResult = engine->destroySession(sessionId);
        if (destroyResult != Status::OK) {
            std::cerr << "✗ Failed to destroy session: " << static_cast<int>(destroyResult)
                      << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Failed to destroy session " + std::to_string(sessionId) + ": " +
                        std::to_string(static_cast<int>(destroyResult)));
            }
            return false;
        } else {
            std::cout << "✓ Session destroyed." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                    "Session " + std::to_string(sessionId) + " destroyed successfully");
            }
        }

        monitor.checkpoint("Session destroyed");

        // 4. Verify Session is Gone
        if (options.enableVerbose) {
            std::cout << "  Verifying session is inactive..." << std::endl;
        }

        if (engine->isSessionActive(sessionId)) {
            std::cerr << "✗ Session " << sessionId << " should be inactive but is not."
                      << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Session " + std::to_string(sessionId) +
                        " should be inactive but is still active");
            }
            return false;
        } else {
            std::cout << "✓ Session " << sessionId << " is inactive as expected." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::DEBUG,
                    "Session " + std::to_string(sessionId) + " is inactive as expected");
            }
        }

        monitor.checkpoint("Session verified inactive");

        if (options.enableSessionDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                "Single session lifecycle test completed successfully");
        }

        return true;
    }

    bool testMultipleSessions() {
        PerformanceMonitor monitor("Multiple sessions test", options.enablePerformanceMetrics);

        std::cout << "\n--- Testing Multiple Sessions ---" << std::endl;

        if (options.enableSessionDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Starting multiple sessions test");
        }

        auto engineResult = UnifiedAudioEngine::create();
        if (!engineResult) {
            if (options.enableEngineDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Failed to create engine for multiple sessions test");
            }
            return false;
        }

        auto engine = std::move(*engineResult);
        monitor.checkpoint("Engine created");

        if (options.enableVerbose) {
            std::cout << "  Creating first session..." << std::endl;
        }

        auto session1Result = engine->createSession();
        auto session2Result = engine->createSession();

        monitor.checkpoint("Sessions created");

        if (!session1Result || !session2Result) {
            std::cerr << "✗ Failed to create one or more sessions." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Failed to create one or more sessions - Session1: " +
                        std::to_string(session1Result.isOk()) +
                        ", Session2: " + std::to_string(session2Result.isOk()));
            }
            return false;
        }

        SessionId session1 = *session1Result;
        SessionId session2 = *session2Result;
        std::cout << "✓ Created two sessions with IDs: " << session1 << " and " << session2
                  << std::endl;

        if (options.enableSessionDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                "Created two sessions - ID1: " + std::to_string(session1) +
                    ", ID2: " + std::to_string(session2));
        }

        if (options.enableVerbose) {
            std::cout << "  Querying active sessions..." << std::endl;
        }

        auto activeSessions = engine->getActiveSessions();
        monitor.checkpoint("Active sessions queried");

        if (activeSessions.size() == 2) {
            std::cout << "✓ getActiveSessions reports 2 sessions." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::DEBUG,
                    "getActiveSessions correctly reports 2 sessions");
            }
        } else {
            std::cerr << "✗ getActiveSessions reports " << activeSessions.size()
                      << " sessions, expected 2." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "getActiveSessions reports " + std::to_string(activeSessions.size()) +
                        " sessions, expected 2");
            }
            return false;
        }

        if (options.enableVerbose) {
            std::cout << "  Destroying both sessions..." << std::endl;
        }

        auto destroyResult1 = engine->destroySession(session1);
        auto destroyResult2 = engine->destroySession(session2);
        std::cout << "✓ Destroyed both sessions." << std::endl;

        monitor.checkpoint("Sessions destroyed");

        if (options.enableSessionDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::AUDIO_ENGINE,
                                                       huntmaster::DebugLevel::INFO,
                                                       "Both sessions destroyed successfully");
        }

        if (options.enableSessionDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                "Multiple sessions test completed successfully");
        }

        return true;
    }

    bool testInvalidSession() {
        PerformanceMonitor monitor("Invalid session test", options.enablePerformanceMetrics);

        std::cout << "\n--- Testing Invalid Session ---" << std::endl;

        if (options.enableSessionDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Starting invalid session test");
        }

        auto engineResult = UnifiedAudioEngine::create();
        if (!engineResult) {
            if (options.enableEngineDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Failed to create engine for invalid session test");
            }
            return false;
        }

        auto engine = std::move(*engineResult);
        monitor.checkpoint("Engine created");

        if (options.enableVerbose) {
            std::cout << "  Attempting to destroy non-existent session..." << std::endl;
        }

        SessionId invalidSessionId = 999;
        Status invalidResult = engine->destroySession(invalidSessionId);

        monitor.checkpoint("Invalid session destroy attempted");

        if (invalidResult == Status::SESSION_NOT_FOUND) {
            std::cout << "✓ Correctly failed to destroy non-existent session." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::DEBUG,
                    "Correctly failed to destroy non-existent session " +
                        std::to_string(invalidSessionId));
            }
        } else {
            std::cerr << "✗ Incorrect status when destroying non-existent session: "
                      << static_cast<int>(invalidResult) << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Incorrect status when destroying non-existent session: " +
                        std::to_string(static_cast<int>(invalidResult)));
            }
            return false;
        }

        // Test isSessionActive with invalid session
        if (options.enableVerbose) {
            std::cout << "  Checking if invalid session is active..." << std::endl;
        }

        if (!engine->isSessionActive(invalidSessionId)) {
            std::cout << "✓ Correctly reports invalid session as inactive." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::DEBUG,
                    "Correctly reports invalid session " + std::to_string(invalidSessionId) +
                        " as inactive");
            }
        } else {
            std::cerr << "✗ Incorrectly reports invalid session as active." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Incorrectly reports invalid session " + std::to_string(invalidSessionId) +
                        " as active");
            }
            return false;
        }

        monitor.checkpoint("Invalid session activity checked");

        if (options.enableSessionDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                "Invalid session test completed successfully");
        }

        return true;
    }

    bool testEngineCreation() {
        PerformanceMonitor monitor("Engine creation test", options.enablePerformanceMetrics);

        std::cout << "\n--- Testing Engine Creation ---" << std::endl;

        if (options.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Starting engine creation test");
        }

        // Test multiple engine creation
        if (options.enableVerbose) {
            std::cout << "  Creating multiple engines..." << std::endl;
        }

        auto engine1Result = UnifiedAudioEngine::create();
        auto engine2Result = UnifiedAudioEngine::create();

        monitor.checkpoint("Multiple engines created");

        if (!engine1Result || !engine2Result) {
            std::cerr << "✗ Failed to create multiple engines." << std::endl;

            if (options.enableEngineDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Failed to create multiple engines - Engine1: " +
                        std::to_string(engine1Result.isOk()) +
                        ", Engine2: " + std::to_string(engine2Result.isOk()));
            }
            return false;
        }

        std::cout << "✓ Successfully created multiple engines." << std::endl;

        if (options.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::AUDIO_ENGINE,
                                                       huntmaster::DebugLevel::INFO,
                                                       "Successfully created multiple engines");
        }

        // Test engine destruction (automatic via RAII)
        if (options.enableVerbose) {
            std::cout << "  Engines will be destroyed automatically..." << std::endl;
        }

        if (options.enableEngineDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                "Engine creation test completed successfully");
        }

        return true;
    }

    bool testSessionQueries() {
        PerformanceMonitor monitor("Session queries test", options.enablePerformanceMetrics);

        std::cout << "\n--- Testing Session Queries ---" << std::endl;

        if (options.enableSessionDebug) {
            huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::DEBUG,
                                                       "Starting session queries test");
        }

        auto engineResult = UnifiedAudioEngine::create();
        if (!engineResult) {
            if (options.enableEngineDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Failed to create engine for session queries test");
            }
            return false;
        }

        auto engine = std::move(*engineResult);
        monitor.checkpoint("Engine created");

        // Test getActiveSessions with no sessions
        if (options.enableVerbose) {
            std::cout << "  Querying active sessions (should be empty)..." << std::endl;
        }

        auto emptySessions = engine->getActiveSessions();
        if (emptySessions.empty()) {
            std::cout << "✓ getActiveSessions correctly returns empty list." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::DEBUG,
                    "getActiveSessions correctly returns empty list");
            }
        } else {
            std::cerr << "✗ getActiveSessions should return empty list but returned "
                      << emptySessions.size() << " sessions." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "getActiveSessions should return empty list but returned " +
                        std::to_string(emptySessions.size()) + " sessions");
            }
            return false;
        }

        monitor.checkpoint("Empty sessions query tested");

        // Create some sessions and test queries
        if (options.enableVerbose) {
            std::cout << "  Creating sessions for query testing..." << std::endl;
        }

        std::vector<SessionId> sessionIds;
        for (int i = 0; i < 3; ++i) {
            auto sessionResult = engine->createSession();
            if (sessionResult) {
                sessionIds.push_back(*sessionResult);
            }
        }

        monitor.checkpoint("Test sessions created");

        if (sessionIds.size() == 3) {
            std::cout << "✓ Created 3 test sessions." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::INFO,
                    "Created 3 test sessions for query testing");
            }
        } else {
            std::cerr << "✗ Expected to create 3 sessions but created " << sessionIds.size()
                      << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "Expected to create 3 sessions but created " +
                        std::to_string(sessionIds.size()));
            }
            return false;
        }

        // Test getActiveSessions with sessions
        if (options.enableVerbose) {
            std::cout << "  Querying active sessions (should have 3)..." << std::endl;
        }

        auto activeSessions = engine->getActiveSessions();
        if (activeSessions.size() == 3) {
            std::cout << "✓ getActiveSessions correctly returns 3 sessions." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::DEBUG,
                    "getActiveSessions correctly returns 3 sessions");
            }
        } else {
            std::cerr << "✗ getActiveSessions returned " << activeSessions.size()
                      << " sessions, expected 3." << std::endl;

            if (options.enableSessionDebug) {
                huntmaster::DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::AUDIO_ENGINE, huntmaster::DebugLevel::ERROR,
                    "getActiveSessions returned " + std::to_string(activeSessions.size()) +
                        " sessions, expected 3");
            }
            return false;
        }

        monitor.checkpoint("Active sessions query tested");

        // Clean up
        if (options.enableVerbose) {
            std::cout << "  Cleaning up test sessions..." << std::endl;
        }

        for (SessionId sessionId : sessionIds) {
            auto destroyResult = engine->destroySession(sessionId);
        }

        monitor.checkpoint("Test sessions cleaned up");

        if (options.enableSessionDebug) {
            huntmaster::DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                "Session queries test completed successfully");
        }

        return true;
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
        huntmaster::DebugConfig::setupFullDebug();
    } else if (debugOptions.enableDebug) {
        huntmaster::DebugConfig::setupToolsDebug();
    }

    // Configure component-specific debug levels
    auto& logger = huntmaster::DebugLogger::getInstance();
    if (debugOptions.enableEngineDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::AUDIO_ENGINE,
                                 huntmaster::DebugLevel::DEBUG);
    }
    if (debugOptions.enableSessionDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::AUDIO_ENGINE,
                                 huntmaster::DebugLevel::TRACE);
    }
    if (debugOptions.enablePerformanceMetrics) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::PERFORMANCE,
                                 huntmaster::DebugLevel::DEBUG);
    }

    huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::INFO,
                                               "=== Simple Unified Test Tool Started ===");

    try {
        UnifiedEngineTestSuite testSuite(debugOptions);
        testSuite.runAllTests();

        huntmaster::DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
            "=== Simple Unified Test Tool Completed Successfully ===");

    } catch (const std::exception& e) {
        std::cerr << "❌ An unexpected error occurred: " << e.what() << std::endl;
        huntmaster::DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                   huntmaster::DebugLevel::ERROR,
                                                   "Exception occurred: " + std::string(e.what()));
        return 1;
    }

    return 0;
}
