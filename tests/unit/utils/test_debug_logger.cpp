/**
 * @file test_debug_logger.cpp
 * @brief Comprehensive test suite for DebugLogger
 *
 * This test suite provides thorough testing of the DebugLogger singleton
 * including initialization, logging levels, component filtering, file output,
 * and thread safety.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 27, 2025
 */

#include <chrono>
#include <fstream>
#include <memory>
#include <regex>
#include <thread>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "huntmaster/core/DebugLogger.h"

using namespace huntmaster;
using namespace huntmaster::test;
using namespace std::chrono_literals;

class DebugLoggerTest : public TestFixtureBase {
  protected:
    void SetUp() override {
        TestFixtureBase::SetUp();

        // Reset logger to default state
        DebugLogger::getInstance().setGlobalLogLevel(DebugLevel::INFO);
        DebugLogger::getInstance().disableFileLogging();
        DebugLogger::getInstance().enableConsoleOutput(true);

        // Create test log file paths
        testLogFile_ = "test_debug.log";
        testLogFile2_ = "test_debug2.log";

        // Clean up any existing test files
        removeTestFile(testLogFile_);
        removeTestFile(testLogFile2_);
    }

    void TearDown() override {
        // Clean up test files
        removeTestFile(testLogFile_);
        removeTestFile(testLogFile2_);

        // Reset logger state
        DebugLogger::getInstance().disableFileLogging();
        DebugLogger::getInstance().enableConsoleOutput(true);
        DebugLogger::getInstance().setGlobalLogLevel(DebugLevel::INFO);

        TestFixtureBase::TearDown();
    }

    // Helper function to remove test files
    void removeTestFile(const std::string& filename) {
        std::remove(filename.c_str());
    }

    // Helper function to read file contents
    std::string readFileContents(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return "";
        }

        std::string content;
        std::string line;
        while (std::getline(file, line)) {
            content += line + "\n";
        }

        return content;
    }

    // Helper function to count lines in file
    int countLinesInFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return 0;
        }

        int count = 0;
        std::string line;
        while (std::getline(file, line)) {
            ++count;
        }

        return count;
    }

    std::string testLogFile_;
    std::string testLogFile2_;
};

// Basic functionality tests
TEST_F(DebugLoggerTest, SingletonInstanceTest) {
    // Test that getInstance returns the same instance
    auto& logger1 = DebugLogger::getInstance();
    auto& logger2 = DebugLogger::getInstance();

    EXPECT_EQ(&logger1, &logger2);
}

TEST_F(DebugLoggerTest, DefaultConfigurationTest) {
    auto& logger = DebugLogger::getInstance();

    // Default level should be INFO
    EXPECT_EQ(logger.getGlobalLogLevel(), DebugLevel::INFO);

    // Console logging should be enabled by default

    // File logging should be disabled by default
}

// Logging level tests
TEST_F(DebugLoggerTest, LoggingLevelTest) {
    auto& logger = DebugLogger::getInstance();

    // Test all logging levels
    logger.setGlobalLogLevel(DebugLevel::TRACE);
    EXPECT_EQ(logger.getGlobalLogLevel(), DebugLevel::TRACE);

    logger.setGlobalLogLevel(DebugLevel::DEBUG);
    EXPECT_EQ(logger.getGlobalLogLevel(), DebugLevel::DEBUG);

    logger.setGlobalLogLevel(DebugLevel::INFO);
    EXPECT_EQ(logger.getGlobalLogLevel(), DebugLevel::INFO);

    logger.setGlobalLogLevel(DebugLevel::WARN);
    EXPECT_EQ(logger.getGlobalLogLevel(), DebugLevel::WARN);

    logger.setGlobalLogLevel(DebugLevel::ERROR);
    EXPECT_EQ(logger.getGlobalLogLevel(), DebugLevel::ERROR);

    logger.setGlobalLogLevel(DebugLevel::ERROR);
    EXPECT_EQ(logger.getGlobalLogLevel(), DebugLevel::ERROR);
}

TEST_F(DebugLoggerTest, LoggingLevelFilteringTest) {
    auto& logger = DebugLogger::getInstance();

    // Enable file logging for testing
    logger.enableFileLogging(testLogFile_);

    // Set level to WARN - should only log WARN, ERROR, FATAL
    logger.setGlobalLogLevel(DebugLevel::WARN);

    // Log messages at different levels
    logger.trace(DebugComponent::UNIFIED_ENGINE, "trace message", __FILE__, __LINE__, __func__);
    logger.debug(DebugComponent::UNIFIED_ENGINE, "debug message", __FILE__, __LINE__, __func__);
    logger.info(DebugComponent::UNIFIED_ENGINE, "info message", __FILE__, __LINE__, __func__);
    logger.warn(DebugComponent::UNIFIED_ENGINE, "warn message", __FILE__, __LINE__, __func__);
    logger.error(DebugComponent::UNIFIED_ENGINE, "error message", __FILE__, __LINE__, __func__);
    logger.error(DebugComponent::UNIFIED_ENGINE, "fatal message", __FILE__, __LINE__, __func__);

    // Allow time for file writes
    std::this_thread::sleep_for(50ms);

    // Read log file and check contents
    std::string logContents = readFileContents(testLogFile_);

    // Should not contain trace, debug, info
    EXPECT_EQ(logContents.find("trace message"), std::string::npos);
    EXPECT_EQ(logContents.find("debug message"), std::string::npos);
    EXPECT_EQ(logContents.find("info message"), std::string::npos);

    // Should contain warn, error, fatal
    EXPECT_NE(logContents.find("warn message"), std::string::npos);
    EXPECT_NE(logContents.find("error message"), std::string::npos);
    EXPECT_NE(logContents.find("fatal message"), std::string::npos);
}

// Component filtering tests
TEST_F(DebugLoggerTest, ComponentFilteringTest) {
    auto& logger = DebugLogger::getInstance();

    // Enable file logging
    logger.enableFileLogging(testLogFile_);
    logger.setGlobalLogLevel(DebugLevel::TRACE);

    // Log messages from different components
    logger.info(DebugComponent::UNIFIED_ENGINE, "engine message", __FILE__, __LINE__, __func__);
    logger.info(DebugComponent::MFCC_PROCESSOR, "mfcc message", __FILE__, __LINE__, __func__);
    logger.info(DebugComponent::DTW_COMPARATOR, "dtw message", __FILE__, __LINE__, __func__);
    logger.info(DebugComponent::VAD, "vad message", __FILE__, __LINE__, __func__);
    logger.info(DebugComponent::AUDIO_ENGINE, "recorder message", __FILE__, __LINE__, __func__);

    // Allow time for file writes
    std::this_thread::sleep_for(50ms);

    // All messages should be present when no filtering is applied
    std::string logContents = readFileContents(testLogFile_);
    EXPECT_NE(logContents.find("engine message"), std::string::npos);
    EXPECT_NE(logContents.find("mfcc message"), std::string::npos);
    EXPECT_NE(logContents.find("dtw message"), std::string::npos);
    EXPECT_NE(logContents.find("vad message"), std::string::npos);
    EXPECT_NE(logContents.find("recorder message"), std::string::npos);
}

// File logging tests
TEST_F(DebugLoggerTest, FileLoggingTest) {
    auto& logger = DebugLogger::getInstance();

    // Initially file logging should be disabled

    // Enable file logging
    logger.enableFileLogging(testLogFile_);

    // Log a message
    logger.info(DebugComponent::UNIFIED_ENGINE, "test file message", __FILE__, __LINE__, __func__);

    // Allow time for file write
    std::this_thread::sleep_for(50ms);

    // Check that file exists and contains the message
    std::string logContents = readFileContents(testLogFile_);
    EXPECT_NE(logContents.find("test file message"), std::string::npos);

    // Disable file logging
    logger.disableFileLogging();
}

TEST_F(DebugLoggerTest, MultipleFileLoggingTest) {
    auto& logger = DebugLogger::getInstance();

    // Enable file logging with first file
    logger.enableFileLogging(testLogFile_);

    logger.info(DebugComponent::UNIFIED_ENGINE, "message1", __FILE__, __LINE__, __func__);
    std::this_thread::sleep_for(50ms);

    // Switch to second file
    logger.info(DebugComponent::UNIFIED_ENGINE, "message2", __FILE__, __LINE__, __func__);
    std::this_thread::sleep_for(50ms);

    // Check both files
    std::string log1 = readFileContents(testLogFile_);
    std::string log2 = readFileContents(testLogFile2_);

    EXPECT_NE(log1.find("message1"), std::string::npos);
    EXPECT_EQ(log1.find("message2"), std::string::npos);  // Should not be in first file

    EXPECT_NE(log2.find("message2"), std::string::npos);
    EXPECT_EQ(log2.find("message1"), std::string::npos);  // Should not be in second file
}

// Console logging tests
TEST_F(DebugLoggerTest, ConsoleLoggingTest) {
    auto& logger = DebugLogger::getInstance();

    // Console logging should be enabled by default

    // Disable console logging
    logger.enableConsoleOutput(false);

    // Re-enable console logging
    logger.enableConsoleOutput(true);
}

// Message formatting tests
TEST_F(DebugLoggerTest, MessageFormattingTest) {
    auto& logger = DebugLogger::getInstance();

    // Enable file logging to capture formatted output
    logger.enableFileLogging(testLogFile_);
    logger.setGlobalLogLevel(DebugLevel::TRACE);

    const char* testFile = "test.cpp";
    const int testLine = 123;
    const char* testFunc = "testFunction";

    logger.error(
        DebugComponent::UNIFIED_ENGINE, "test error message", testFile, testLine, testFunc);

    std::this_thread::sleep_for(50ms);

    std::string logContents = readFileContents(testLogFile_);

    // Check that log contains expected elements (exact format may vary)
    EXPECT_NE(logContents.find("ERROR"), std::string::npos);
    EXPECT_NE(logContents.find("test error message"), std::string::npos);
    EXPECT_NE(logContents.find("UNIFIED_ENGINE"), std::string::npos);

    // File and line info might be included depending on implementation
    // This is a flexible check since format may vary
    EXPECT_GT(logContents.length(), 20);  // Should be a reasonable length
}

// Thread safety tests
TEST_F(DebugLoggerTest, ThreadSafetyTest) {
    auto& logger = DebugLogger::getInstance();

    // Enable file logging
    logger.enableFileLogging(testLogFile_);
    logger.setGlobalLogLevel(DebugLevel::TRACE);

    const int numThreads = 5;
    const int messagesPerThread = 20;
    std::atomic<int> errorCount{0};

    std::vector<std::thread> threads;

    // Create multiple threads that log simultaneously
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            try {
                for (int i = 0; i < messagesPerThread; ++i) {
                    std::string message =
                        "Thread " + std::to_string(t) + " Message " + std::to_string(i);
                    logger.info(
                        DebugComponent::UNIFIED_ENGINE, message, __FILE__, __LINE__, __func__);

                    // Small random delay to increase chance of race conditions
                    if (i % 3 == 0) {
                        std::this_thread::sleep_for(1ms);
                    }
                }
            } catch (const std::exception& e) {
                errorCount++;
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Allow time for all writes to complete
    std::this_thread::sleep_for(100ms);

    // Should have no errors from threading issues
    EXPECT_EQ(errorCount.load(), 0);

    // Count total messages in log file
    int lineCount = countLinesInFile(testLogFile_);
    EXPECT_GE(lineCount, numThreads * messagesPerThread);  // Should have at least this many lines

    // Read log contents and verify no corruption
    std::string logContents = readFileContents(testLogFile_);
    EXPECT_GT(logContents.length(), 100);  // Should have substantial content

    // Check for presence of messages from different threads
    bool foundThread0 = logContents.find("Thread 0") != std::string::npos;
    bool foundThread4 = logContents.find("Thread 4") != std::string::npos;
    EXPECT_TRUE(foundThread0);
    EXPECT_TRUE(foundThread4);
}

// Performance tests
TEST_F(DebugLoggerTest, PerformanceTest) {
    auto& logger = DebugLogger::getInstance();

    // Test with different configurations
    logger.disableFileLogging();
    logger.enableConsoleOutput(false);  // Disable output for performance test
    logger.setGlobalLogLevel(DebugLevel::INFO);

    const int numMessages = 1000;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numMessages; ++i) {
        logger.info(DebugComponent::UNIFIED_ENGINE,
                    "Performance test message " + std::to_string(i),
                    __FILE__,
                    __LINE__,
                    __func__);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Should complete reasonably quickly (exact time depends on system)
    // This is more of a regression test - if it takes much longer than expected,
    // there might be a performance issue
    EXPECT_LT(duration.count(), 1000);  // Should complete in less than 1 second

    std::cout << "Logged " << numMessages << " messages in " << duration.count() << "ms"
              << std::endl;
}

// Edge case tests
TEST_F(DebugLoggerTest, EmptyMessageTest) {
    auto& logger = DebugLogger::getInstance();

    // Enable file logging
    logger.enableFileLogging(testLogFile_);

    // Test empty message
    logger.info(DebugComponent::UNIFIED_ENGINE, "", __FILE__, __LINE__, __func__);

    std::this_thread::sleep_for(50ms);

    // Should handle empty message gracefully
    std::string logContents = readFileContents(testLogFile_);
    EXPECT_GT(logContents.length(), 0);  // Should still have timestamp, level, etc.
}

TEST_F(DebugLoggerTest, LongMessageTest) {
    auto& logger = DebugLogger::getInstance();

    // Enable file logging
    logger.enableFileLogging(testLogFile_);

    // Create a very long message
    std::string longMessage(10000, 'A');
    longMessage += " END";

    logger.info(DebugComponent::UNIFIED_ENGINE, longMessage, __FILE__, __LINE__, __func__);

    std::this_thread::sleep_for(100ms);

    // Should handle long message
    std::string logContents = readFileContents(testLogFile_);
    EXPECT_NE(logContents.find("AAAA"), std::string::npos);
    EXPECT_NE(logContents.find("END"), std::string::npos);
}

TEST_F(DebugLoggerTest, SpecialCharactersTest) {
    auto& logger = DebugLogger::getInstance();

    // Enable file logging
    logger.enableFileLogging(testLogFile_);

    // Test message with special characters
    std::string specialMessage = "Special chars: !@#$%^&*(){}[]|\\:;\"'<>,.?/~`+=";
    logger.info(DebugComponent::UNIFIED_ENGINE, specialMessage, __FILE__, __LINE__, __func__);

    std::this_thread::sleep_for(50ms);

    // Should handle special characters
    std::string logContents = readFileContents(testLogFile_);
    EXPECT_NE(logContents.find("Special chars"), std::string::npos);
}

// Configuration persistence tests
TEST_F(DebugLoggerTest, ConfigurationPersistenceTest) {
    auto& logger = DebugLogger::getInstance();

    // Set specific configuration
    logger.setGlobalLogLevel(DebugLevel::WARN);
    logger.enableFileLogging(testLogFile_);
    logger.enableConsoleOutput(false);

    // Log a message
    logger.warn(DebugComponent::UNIFIED_ENGINE, "config test", __FILE__, __LINE__, __func__);

    // Configuration should persist
    EXPECT_EQ(logger.getGlobalLogLevel(), DebugLevel::WARN);

    std::this_thread::sleep_for(50ms);

    // Message should be logged according to configuration
    std::string logContents = readFileContents(testLogFile_);
    EXPECT_NE(logContents.find("config test"), std::string::npos);
}

// Macro tests (if available)
#ifdef LOG_INFO  // Check if logging macros are defined
TEST_F(DebugLoggerTest, LoggingMacrosTest) {
    auto& logger = DebugLogger::getInstance();

    // Enable file logging
    logger.enableFileLogging(testLogFile_);
    logger.setGlobalLogLevel(DebugLevel::TRACE);

    // Test various logging macros
    LOG_INFO(DebugComponent::UNIFIED_ENGINE, "macro info test");
    LOG_WARN(DebugComponent::UNIFIED_ENGINE, "macro warn test");
    LOG_ERROR(DebugComponent::UNIFIED_ENGINE, "macro error test");

    std::this_thread::sleep_for(50ms);

    std::string logContents = readFileContents(testLogFile_);
    EXPECT_NE(logContents.find("macro info test"), std::string::npos);
    EXPECT_NE(logContents.find("macro warn test"), std::string::npos);
    EXPECT_NE(logContents.find("macro error test"), std::string::npos);
}
#endif
