#pragma once

#include <atomic>
#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

#include <huntmaster/core/ComponentErrorHandler.h>
#include <huntmaster/core/ErrorLogger.h>

namespace huntmaster {

/**
 * @brief Real-time error monitoring and alerting system
 *
 * This class provides automated error monitoring, threshold checking,
 * and reporting capabilities for the Huntmaster engine.
 */
class ErrorMonitor {
  public:
    /**
     * @brief Monitoring configuration
     */
    struct Config {
        bool enableRealTimeMonitoring = true;
        size_t errorRateThreshold = 10;     // Errors per minute
        size_t criticalErrorThreshold = 3;  // Critical errors per minute
        std::chrono::seconds monitoringInterval{30};
        bool enableFileLogging = true;
        std::string logFilePath = "huntmaster_errors.log";
        bool enableConsoleAlerts = true;
        bool enablePerformanceTracking = true;
    };

    /**
     * @brief Error alert information
     */
    struct ErrorAlert {
        std::chrono::system_clock::time_point timestamp;
        std::string alertType;
        std::string message;
        ErrorStats stats;
        std::vector<ErrorInfo> recentErrors;
    };

    /**
     * @brief Performance metrics for error tracking
     */
    struct PerformanceMetrics {
        double averageErrorsPerMinute = 0.0;
        double peakErrorsPerMinute = 0.0;
        Component mostProblematicComponent = Component::GENERAL;
        ErrorCategory mostCommonCategory = ErrorCategory::PROCESSING;
        std::chrono::system_clock::time_point lastResetTime;
    };

    /**
     * @brief Initialize error monitor with configuration
     */
    ErrorMonitor();
    explicit ErrorMonitor(const Config& config);

    /**
     * @brief Destructor - stops monitoring
     */
    ~ErrorMonitor();

    /**
     * @brief Start error monitoring
     */
    void startMonitoring();

    /**
     * @brief Stop error monitoring
     */
    void stopMonitoring();

    /**
     * @brief Check if monitoring is active
     */
    bool isMonitoring() const;

    /**
     * @brief Get current performance metrics
     */
    PerformanceMetrics getPerformanceMetrics() const;

    /**
     * @brief Generate comprehensive error report
     */
    std::string generateErrorReport() const;

    /**
     * @brief Export error data to file
     */
    bool exportErrorData(const std::string& filename) const;

    /**
     * @brief Reset all monitoring statistics
     */
    void resetStatistics();

    /**
     * @brief Update configuration
     */
    void updateConfig(const Config& newConfig);

    /**
     * @brief Force immediate error check and report
     */
    void forceErrorCheck();

  private:
    Config config_;
    std::atomic<bool> monitoring_{false};
    std::unique_ptr<std::thread> monitorThread_;
    mutable std::mutex metricsMutex_;

    PerformanceMetrics metrics_;
    std::vector<ErrorAlert> alerts_;
    static constexpr size_t MAX_STORED_ALERTS = 100;

    // Monitoring implementation
    void monitoringLoop();
    void checkErrorRates();
    void generateAlert(const std::string& alertType, const std::string& message);
    void writeToLogFile(const std::string& message);
    void analyzeErrorPatterns();
    void updatePerformanceMetrics();
    std::string formatErrorReport(const ErrorStats& stats) const;
    std::string formatTimestamp(const std::chrono::system_clock::time_point& time) const;
};

/**
 * @brief Global error monitoring instance accessor
 */
ErrorMonitor& getGlobalErrorMonitor();

/**
 * @brief Initialize global error monitoring with default configuration
 */
void initializeGlobalErrorMonitoring(const ErrorMonitor::Config& config = ErrorMonitor::Config{});

/**
 * @brief Cleanup global error monitoring
 */
void shutdownGlobalErrorMonitoring();

}  // namespace huntmaster
