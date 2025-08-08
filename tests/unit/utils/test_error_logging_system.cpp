#include <gtest/gtest.h>
#include <huntmaster/core/ErrorLogger.h>
#include <huntmaster/core/ComponentErrorHandler.h>
#include <huntmaster/core/ErrorMonitor.h>
#include <huntmaster/core/UnifiedAudioEngine.h>
#include <thread>
#include <chrono>
#include <fstream>

using namespace huntmaster;

class ErrorLoggingSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear any existing error stats
        ErrorLogger::getInstance().clearErrorStats();
        
        // Initialize error monitoring
        ErrorMonitor::Config config;
        config.criticalErrorThreshold = 3;
        config.errorRateThreshold = 5.0;
        config.enableConsoleAlerts = true;
        config.enableFileLogging = true;
        config.logFilePath = "test_error_monitor.log";
        config.monitoringInterval = std::chrono::milliseconds(100);
        
        errorMonitor_ = std::make_unique<ErrorMonitor>(config);
        errorMonitor_->startMonitoring();
        
        // Clear log file
        std::ofstream logFile(config.logFilePath, std::ios::trunc);
        logFile.close();
    }
    
    void TearDown() override {
        if (errorMonitor_) {
            errorMonitor_->stopMonitoring();
            errorMonitor_.reset();
        }
        
        // Clean up test log file
        std::remove("test_error_monitor.log");
        
        ErrorLogger::getInstance().clearErrorStats();
    }
    
    std::unique_ptr<ErrorMonitor> errorMonitor_;
};

TEST_F(ErrorLoggingSystemTest, BasicErrorLogging) {
    // Test basic error logging functionality
    ErrorLogger& logger = ErrorLogger::getInstance();
    
    // Log errors with different severities and categories
    logger.logError(ErrorSeverity::CRITICAL, ErrorCategory::INITIALIZATION, 
                   Component::UNIFIED_ENGINE, "TEST_CRITICAL_001", 
                   "Critical initialization error", "Test details");
    
    logger.logError(ErrorSeverity::HIGH, ErrorCategory::MEMORY, 
                   Component::MFCC_PROCESSOR, "TEST_MEMORY_001", 
                   "Memory allocation failed", "Out of memory");
    
    logger.logError(ErrorSeverity::MEDIUM, ErrorCategory::PROCESSING, 
                   Component::AUDIO_RECORDER, "TEST_PROCESSING_001", 
                   "Audio processing warning", "Buffer underrun");
    
    // Check statistics
    auto stats = logger.getErrorStats();
    EXPECT_EQ(stats.totalErrors, 3);
    EXPECT_EQ(stats.criticalErrors, 1);
    EXPECT_EQ(stats.highErrors, 1);
    EXPECT_EQ(stats.mediumErrors, 1);
    EXPECT_EQ(stats.lowErrors, 0);
    EXPECT_EQ(stats.infoErrors, 0);
    
    // Check component breakdown
    EXPECT_EQ(stats.errorsByComponent[Component::UNIFIED_ENGINE], 1);
    EXPECT_EQ(stats.errorsByComponent[Component::MFCC_PROCESSOR], 1);
    EXPECT_EQ(stats.errorsByComponent[Component::AUDIO_RECORDER], 1);
    
    // Check category breakdown
    EXPECT_EQ(stats.errorsByCategory[ErrorCategory::INITIALIZATION], 1);
    EXPECT_EQ(stats.errorsByCategory[ErrorCategory::MEMORY], 1);
    EXPECT_EQ(stats.errorsByCategory[ErrorCategory::PROCESSING], 1);
}

TEST_F(ErrorLoggingSystemTest, ComponentErrorHandlers) {
    // Test component-specific error handlers
    
    // Test UnifiedEngine errors
    ComponentErrorHandler::UnifiedEngineErrors::logInitializationError(
        "ENGINE_INIT_TEST", "Test initialization error",
        {{"test_param", "test_value"}});
    
    ComponentErrorHandler::UnifiedEngineErrors::logSessionError(
        "SESSION_TEST", "Test session error", 123,
        {{"session_id", "123"}});
    
    // Test MFCC errors
    ComponentErrorHandler::MFCCProcessorErrors::logConfigurationError(
        "MFCC_CONFIG_TEST", "Test MFCC configuration error",
        {{"sample_rate", "44100"}});
    
    ComponentErrorHandler::MFCCProcessorErrors::logFeatureExtractionError(
        "MFCC_EXTRACTION_TEST", "Test feature extraction error",
        {{"frame_count", "100"}});
    
    // Test AudioRecorder errors
    ComponentErrorHandler::AudioRecorderErrors::logDeviceError(
        "DEVICE_TEST", "Test device error",
        {{"device_id", "0"}});
    
    ComponentErrorHandler::AudioRecorderErrors::logBufferError(
        "BUFFER_TEST", "Test buffer error",
        {{"buffer_size", "1024"}});
    
    // Test Memory errors
    ComponentErrorHandler::MemoryErrors::logMemoryAllocationError(
        "MEMORY_ALLOC_TEST", "Test memory allocation error", 1024,
        {{"requested_size", "1024"}});
    
    ComponentErrorHandler::MemoryErrors::logMemoryLeakWarning(
        "MEMORY_LEAK_TEST", "Test memory leak warning", 2048,
        {{"leaked_size", "2048"}});
    
    // Verify all errors were logged
    auto stats = ErrorLogger::getInstance().getErrorStats();
    EXPECT_GE(stats.totalErrors, 8);
    
    // Check that different components are represented
    EXPECT_GT(stats.errorsByComponent.size(), 1);
    
    // Check that different categories are represented
    EXPECT_GT(stats.errorsByCategory.size(), 1);
}

TEST_F(ErrorLoggingSystemTest, ErrorCallbackSystem) {
    bool callbackTriggered = false;
    ErrorInfo lastError;
    
    // Register callback
    ErrorLogger::getInstance().registerErrorCallback([&](const ErrorInfo& error) {
        callbackTriggered = true;
        lastError = error;
    });
    
    // Log an error
    ErrorLogger::getInstance().logError(ErrorSeverity::HIGH, ErrorCategory::IO, 
                                       Component::GENERAL, "CALLBACK_TEST", 
                                       "Test callback error", "Callback details");
    
    // Verify callback was triggered
    EXPECT_TRUE(callbackTriggered);
    EXPECT_EQ(lastError.errorCode, "CALLBACK_TEST");
    EXPECT_EQ(lastError.message, "Test callback error");
    EXPECT_EQ(lastError.severity, ErrorSeverity::HIGH);
    EXPECT_EQ(lastError.category, ErrorCategory::IO);
    EXPECT_EQ(lastError.component, Component::GENERAL);
}

TEST_F(ErrorLoggingSystemTest, ErrorMonitoringAndAlerts) {
    // Generate critical errors to trigger alerts
    for (int i = 0; i < 5; ++i) {
        ErrorLogger::getInstance().logError(ErrorSeverity::CRITICAL, ErrorCategory::SYSTEM, 
                                           Component::UNIFIED_ENGINE, "CRITICAL_TEST_" + std::to_string(i), 
                                           "Critical error " + std::to_string(i), "Details");
    }
    
    // Wait for monitoring to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Force error check
    errorMonitor_->forceErrorCheck();
    
    // Check performance metrics
    auto metrics = errorMonitor_->getPerformanceMetrics();
    EXPECT_GT(metrics.averageErrorsPerMinute, 0);
    EXPECT_EQ(metrics.mostProblematicComponent, Component::UNIFIED_ENGINE);
    EXPECT_EQ(metrics.mostCommonCategory, ErrorCategory::SYSTEM);
}

TEST_F(ErrorLoggingSystemTest, ErrorReportGeneration) {
    // Generate various types of errors
    ErrorLogger& logger = ErrorLogger::getInstance();
    
    logger.logError(ErrorSeverity::CRITICAL, ErrorCategory::INITIALIZATION, 
                   Component::UNIFIED_ENGINE, "REPORT_TEST_001", "Critical init error", "");
    logger.logError(ErrorSeverity::HIGH, ErrorCategory::MEMORY, 
                   Component::MFCC_PROCESSOR, "REPORT_TEST_002", "Memory error", "");
    logger.logError(ErrorSeverity::MEDIUM, ErrorCategory::PROCESSING, 
                   Component::AUDIO_RECORDER, "REPORT_TEST_003", "Processing warning", "");
    
    // Generate report
    std::string report = errorMonitor_->generateErrorReport();
    
    // Verify report contains expected sections
    EXPECT_TRUE(report.find("=== Huntmaster Engine Error Report ===") != std::string::npos);
    EXPECT_TRUE(report.find("Overall Statistics:") != std::string::npos);
    EXPECT_TRUE(report.find("Errors by Component:") != std::string::npos);
    EXPECT_TRUE(report.find("Errors by Category:") != std::string::npos);
    EXPECT_TRUE(report.find("Performance Metrics:") != std::string::npos);
    
    // Check for specific error counts
    EXPECT_TRUE(report.find("Critical: 1") != std::string::npos);
    EXPECT_TRUE(report.find("High: 1") != std::string::npos);
    EXPECT_TRUE(report.find("Medium: 1") != std::string::npos);
}

TEST_F(ErrorLoggingSystemTest, ErrorDataExport) {
    // Generate test errors
    ErrorLogger& logger = ErrorLogger::getInstance();
    
    logger.logErrorWithContext(ErrorSeverity::HIGH, ErrorCategory::IO, 
                              Component::GENERAL, "EXPORT_TEST_001", 
                              "Export test error", "Export details",
                              {{"file_path", "/test/path"}, {"operation", "read"}});
    
    // Export error data
    std::string exportFile = "test_error_export.txt";
    bool exportSuccess = errorMonitor_->exportErrorData(exportFile);
    EXPECT_TRUE(exportSuccess);
    
    // Verify export file exists and contains data
    std::ifstream file(exportFile);
    EXPECT_TRUE(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)), 
                        std::istreambuf_iterator<char>());
    file.close();
    
    EXPECT_TRUE(content.find("=== Huntmaster Engine Error Report ===") != std::string::npos);
    EXPECT_TRUE(content.find("EXPORT_TEST_001") != std::string::npos);
    EXPECT_TRUE(content.find("Export test error") != std::string::npos);
    EXPECT_TRUE(content.find("file_path = /test/path") != std::string::npos);
    
    // Clean up
    std::remove(exportFile.c_str());
}

TEST_F(ErrorLoggingSystemTest, UnifiedAudioEngineErrorIntegration) {
    // Test error logging integration in UnifiedAudioEngine
    auto engineResult = UnifiedAudioEngine::create();
    EXPECT_TRUE(engineResult.status == UnifiedAudioEngine::Status::OK);
    EXPECT_NE(engineResult.value, nullptr);
    
    auto engine = std::move(engineResult.value);
    
    // Test session creation with invalid parameters
    auto sessionResult = engine->createSession(-1.0f); // Invalid sample rate
    EXPECT_EQ(sessionResult.status, UnifiedAudioEngine::Status::INVALID_PARAMS);
    
    // Check that error was logged
    auto stats = ErrorLogger::getInstance().getErrorStats();
    EXPECT_GT(stats.totalErrors, 0);
    
    // Test successful session creation
    auto validSessionResult = engine->createSession(44100.0f);
    EXPECT_EQ(validSessionResult.status, UnifiedAudioEngine::Status::OK);
    
    SessionId sessionId = validSessionResult.value;
    
    // Test processing with empty buffer (should log error)
    std::vector<float> emptyBuffer;
    auto processResult = engine->processAudioChunk(sessionId, emptyBuffer);
    EXPECT_EQ(processResult, UnifiedAudioEngine::Status::INVALID_PARAMS);
    
    // Test processing with valid buffer
    std::vector<float> validBuffer(1024, 0.5f);
    auto validProcessResult = engine->processAudioChunk(sessionId, validBuffer);
    EXPECT_EQ(validProcessResult, UnifiedAudioEngine::Status::OK);
    
    // Test processing with invalid session ID
    auto invalidSessionResult = engine->processAudioChunk(99999, validBuffer);
    EXPECT_EQ(invalidSessionResult, UnifiedAudioEngine::Status::SESSION_NOT_FOUND);
    
    // Clean up
    engine->destroySession(sessionId);
    
    // Verify error logging occurred
    auto finalStats = ErrorLogger::getInstance().getErrorStats();
    EXPECT_GT(finalStats.totalErrors, stats.totalErrors);
}

TEST_F(ErrorLoggingSystemTest, StatisticsAndClearance) {
    ErrorLogger& logger = ErrorLogger::getInstance();
    
    // Log several errors
    for (int i = 0; i < 10; ++i) {
        logger.logError(ErrorSeverity::MEDIUM, ErrorCategory::PROCESSING, 
                       Component::GENERAL, "STATS_TEST_" + std::to_string(i), 
                       "Test error " + std::to_string(i), "");
    }
    
    auto stats = logger.getErrorStats();
    EXPECT_EQ(stats.totalErrors, 10);
    EXPECT_EQ(stats.mediumErrors, 10);
    
    // Clear statistics
    logger.clearErrorStats();
    errorMonitor_->resetStatistics();
    
    auto clearedStats = logger.getErrorStats();
    EXPECT_EQ(clearedStats.totalErrors, 0);
    EXPECT_EQ(clearedStats.criticalErrors, 0);
    EXPECT_EQ(clearedStats.highErrors, 0);
    EXPECT_EQ(clearedStats.mediumErrors, 0);
    EXPECT_EQ(clearedStats.lowErrors, 0);
    EXPECT_EQ(clearedStats.infoErrors, 0);
}

// Performance test for error logging system
TEST_F(ErrorLoggingSystemTest, ErrorLoggingPerformance) {
    const int numErrors = 1000;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Log many errors quickly
    for (int i = 0; i < numErrors; ++i) {
        ErrorLogger::getInstance().logError(ErrorSeverity::LOW, ErrorCategory::PROCESSING, 
                                           Component::GENERAL, "PERF_TEST_" + std::to_string(i), 
                                           "Performance test error", "");
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Should be able to log 1000 errors in reasonable time (less than 1 second)
    EXPECT_LT(duration.count(), 1000);
    
    auto stats = ErrorLogger::getInstance().getErrorStats();
    EXPECT_EQ(stats.totalErrors, numErrors);
    EXPECT_EQ(stats.lowErrors, numErrors);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
