#include <algorithm>
#include <iomanip>
#include <sstream>

#include <huntmaster/core/ErrorMonitor.h>

namespace huntmaster {

ErrorMonitor::ErrorMonitor() : ErrorMonitor(Config{}) {}

ErrorMonitor::ErrorMonitor(const Config& config) : config_(config) {
    // Register callback with ErrorLogger to receive real-time error notifications
    ErrorLogger::getInstance().registerErrorCallback([this](const ErrorInfo& error) {
        // This will be called for every error logged
        if (monitoring_) {
            // Update real-time metrics here if needed
            // The monitoring loop will periodically check overall stats
        }
        (void)error;  // suppress unused parameter warning until metrics implemented
    });

    metrics_.lastResetTime = std::chrono::system_clock::now();
}

ErrorMonitor::~ErrorMonitor() {
    stopMonitoring();
}

void ErrorMonitor::startMonitoring() {
    if (monitoring_) {
        return;
    }

    monitoring_ = true;
    monitorThread_ = std::make_unique<std::thread>(&ErrorMonitor::monitoringLoop, this);

    LOG_INFO(Component::GENERAL, "Error monitoring started");

    if (config_.enableFileLogging) {
        writeToLogFile("Error monitoring started at "
                       + formatTimestamp(std::chrono::system_clock::now()));
    }
}

void ErrorMonitor::stopMonitoring() {
    if (!monitoring_) {
        return;
    }

    monitoring_ = false;

    if (monitorThread_ && monitorThread_->joinable()) {
        monitorThread_->join();
    }
    monitorThread_.reset();

    LOG_INFO(Component::GENERAL, "Error monitoring stopped");

    if (config_.enableFileLogging) {
        writeToLogFile("Error monitoring stopped at "
                       + formatTimestamp(std::chrono::system_clock::now()));
    }
}

bool ErrorMonitor::isMonitoring() const {
    return monitoring_;
}

ErrorMonitor::PerformanceMetrics ErrorMonitor::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    return metrics_;
}

std::string ErrorMonitor::generateErrorReport() const {
    ErrorStats stats = ErrorLogger::getInstance().getErrorStats();
    std::ostringstream report;

    report << "=== Huntmaster Engine Error Report ===\n";
    report << "Generated: " << formatTimestamp(std::chrono::system_clock::now()) << "\n\n";

    report << "Overall Statistics:\n";
    report << "  Total Errors: " << stats.totalErrors << "\n";
    report << "  Critical: " << stats.criticalErrors << "\n";
    report << "  High: " << stats.highErrors << "\n";
    report << "  Medium: " << stats.mediumErrors << "\n";
    report << "  Low: " << stats.lowErrors << "\n";
    report << "  Info: " << stats.infoErrors << "\n\n";

    if (stats.totalErrors > 0) {
        report << "Last Error: " << formatTimestamp(stats.lastError) << "\n\n";
    }

    // Component breakdown
    report << "Errors by Component:\n";
    for (const auto& [component, count] : stats.errorsByComponent) {
        report << "  " << static_cast<int>(component) << ": " << count << " errors\n";
    }
    report << "\n";

    // Category breakdown
    report << "Errors by Category:\n";
    for (const auto& [category, count] : stats.errorsByCategory) {
        report << "  " << ErrorLogger::categoryToString(category) << ": " << count << " errors\n";
    }
    report << "\n";

    // Performance metrics
    std::lock_guard<std::mutex> lock(metricsMutex_);
    report << "Performance Metrics:\n";
    report << "  Average errors/minute: " << std::fixed << std::setprecision(2)
           << metrics_.averageErrorsPerMinute << "\n";
    report << "  Peak errors/minute: " << metrics_.peakErrorsPerMinute << "\n";
    report << "  Most problematic component: "
           << static_cast<int>(metrics_.mostProblematicComponent) << "\n";
    report << "  Most common category: "
           << ErrorLogger::categoryToString(metrics_.mostCommonCategory) << "\n";

    // Recent alerts
    if (!alerts_.empty()) {
        report << "\nRecent Alerts (" << std::min(alerts_.size(), size_t(5)) << " most recent):\n";
        auto recent_alerts = alerts_;
        std::sort(
            recent_alerts.begin(),
            recent_alerts.end(),
            [](const ErrorAlert& a, const ErrorAlert& b) { return a.timestamp > b.timestamp; });

        for (size_t i = 0; i < std::min(recent_alerts.size(), size_t(5)); ++i) {
            const auto& alert = recent_alerts[i];
            report << "  [" << formatTimestamp(alert.timestamp) << "] " << alert.alertType << ": "
                   << alert.message << "\n";
        }
    }

    return report.str();
}

bool ErrorMonitor::exportErrorData(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERROR(Component::GENERAL, "Failed to open export file: " + filename);
            return false;
        }

        file << generateErrorReport();

        // Export recent errors with full details
        auto recentErrors = ErrorLogger::getInstance().getRecentErrors(50);
        if (!recentErrors.empty()) {
            file << "\n=== Recent Error Details ===\n";
            for (const auto& error : recentErrors) {
                file << "\n[" << formatTimestamp(error.timestamp) << "] "
                     << ErrorLogger::severityToString(error.severity) << " - "
                     << ErrorLogger::categoryToString(error.category) << "\n";
                file << "Component: " << static_cast<int>(error.component) << "\n";
                file << "Code: " << error.errorCode << "\n";
                file << "Message: " << error.message << "\n";
                if (!error.details.empty()) {
                    file << "Details: " << error.details << "\n";
                }
                if (!error.function.empty()) {
                    file << "Function: " << error.function << "\n";
                }
                if (!error.file.empty()) {
                    file << "File: " << error.file << ":" << error.line << "\n";
                }
                if (!error.context.empty()) {
                    file << "Context:\n";
                    for (const auto& [key, value] : error.context) {
                        file << "  " << key << " = " << value << "\n";
                    }
                }
                file << "---\n";
            }
        }

        LOG_INFO(Component::GENERAL, "Error data exported to: " + filename);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(Component::GENERAL, "Failed to export error data: " + std::string(e.what()));
        return false;
    }
}

void ErrorMonitor::resetStatistics() {
    ErrorLogger::getInstance().clearErrorStats();

    std::lock_guard<std::mutex> lock(metricsMutex_);
    metrics_ = PerformanceMetrics{};
    metrics_.lastResetTime = std::chrono::system_clock::now();
    alerts_.clear();

    LOG_INFO(Component::GENERAL, "Error monitoring statistics reset");
}

void ErrorMonitor::updateConfig(const Config& newConfig) {
    config_ = newConfig;
    LOG_INFO(Component::GENERAL, "Error monitor configuration updated");
}

void ErrorMonitor::forceErrorCheck() {
    if (monitoring_) {
        checkErrorRates();
        analyzeErrorPatterns();
        updatePerformanceMetrics();
    }
}

void ErrorMonitor::monitoringLoop() {
    LOG_DEBUG(Component::GENERAL, "Error monitoring loop started");

    while (monitoring_) {
        try {
            checkErrorRates();
            analyzeErrorPatterns();
            updatePerformanceMetrics();

            // Sleep for the configured interval
            std::this_thread::sleep_for(config_.monitoringInterval);
        } catch (const std::exception& e) {
            LOG_ERROR(Component::GENERAL, "Error in monitoring loop: " + std::string(e.what()));
            // Continue monitoring despite errors
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    LOG_DEBUG(Component::GENERAL, "Error monitoring loop finished");
}

void ErrorMonitor::checkErrorRates() {
    ErrorStats stats = ErrorLogger::getInstance().getErrorStats();

    // Check critical error threshold
    if (stats.criticalErrors >= config_.criticalErrorThreshold) {
        generateAlert("CRITICAL_ERROR_THRESHOLD",
                      "Critical error threshold exceeded: " + std::to_string(stats.criticalErrors)
                          + " critical errors detected");
    }

    // Check overall error rate
    if (ErrorLogger::getInstance().isErrorRateHigh(config_.errorRateThreshold)) {
        generateAlert("HIGH_ERROR_RATE",
                      "High error rate detected: > " + std::to_string(config_.errorRateThreshold)
                          + " errors per minute");
    }
}

void ErrorMonitor::generateAlert(const std::string& alertType, const std::string& message) {
    ErrorAlert alert;
    alert.timestamp = std::chrono::system_clock::now();
    alert.alertType = alertType;
    alert.message = message;
    alert.stats = ErrorLogger::getInstance().getErrorStats();
    alert.recentErrors = ErrorLogger::getInstance().getRecentErrors(5);

    std::lock_guard<std::mutex> lock(metricsMutex_);
    alerts_.push_back(alert);

    // Keep only recent alerts
    if (alerts_.size() > MAX_STORED_ALERTS) {
        alerts_.erase(alerts_.begin(), alerts_.begin() + (alerts_.size() - MAX_STORED_ALERTS));
    }

    // Console alert
    if (config_.enableConsoleAlerts) {
        std::cerr << "🚨 ERROR ALERT [" << formatTimestamp(alert.timestamp) << "] " << alertType
                  << ": " << message << std::endl;
    }

    // Log file alert
    if (config_.enableFileLogging) {
        writeToLogFile("ALERT [" + alertType + "]: " + message);
    }

    // Log to DebugLogger
    LOG_ERROR(Component::GENERAL, "Error Monitor Alert - " + alertType + ": " + message);
}

void ErrorMonitor::writeToLogFile(const std::string& message) {
    try {
        std::ofstream logFile(config_.logFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << "[" << formatTimestamp(std::chrono::system_clock::now()) << "] " << message
                    << std::endl;
        }
    } catch (const std::exception& e) {
        // Don't log this error to avoid infinite loops
        std::cerr << "Failed to write to error log file: " << e.what() << std::endl;
    }
}

void ErrorMonitor::analyzeErrorPatterns() {
    ErrorStats stats = ErrorLogger::getInstance().getErrorStats();

    std::lock_guard<std::mutex> lock(metricsMutex_);

    // Find most problematic component
    Component mostProblematic = Component::GENERAL;
    size_t maxErrors = 0;
    for (const auto& [component, count] : stats.errorsByComponent) {
        if (count > maxErrors) {
            maxErrors = count;
            mostProblematic = component;
        }
    }
    metrics_.mostProblematicComponent = mostProblematic;

    // Find most common category
    ErrorCategory mostCommon = ErrorCategory::PROCESSING;
    maxErrors = 0;
    for (const auto& [category, count] : stats.errorsByCategory) {
        if (count > maxErrors) {
            maxErrors = count;
            mostCommon = category;
        }
    }
    metrics_.mostCommonCategory = mostCommon;
}

void ErrorMonitor::updatePerformanceMetrics() {
    ErrorStats stats = ErrorLogger::getInstance().getErrorStats();

    std::lock_guard<std::mutex> lock(metricsMutex_);

    // Calculate time elapsed since last reset
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - metrics_.lastResetTime);

    if (elapsed.count() > 0) {
        double errorsPerMinute = static_cast<double>(stats.totalErrors) / elapsed.count();
        metrics_.averageErrorsPerMinute = errorsPerMinute;

        if (errorsPerMinute > metrics_.peakErrorsPerMinute) {
            metrics_.peakErrorsPerMinute = errorsPerMinute;
        }
    }
}

std::string ErrorMonitor::formatTimestamp(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Global instance management
static std::unique_ptr<ErrorMonitor> g_errorMonitor;

ErrorMonitor& getGlobalErrorMonitor() {
    if (!g_errorMonitor) {
        g_errorMonitor = std::make_unique<ErrorMonitor>();
    }
    return *g_errorMonitor;
}

void initializeGlobalErrorMonitoring(const ErrorMonitor::Config& config) {
    g_errorMonitor = std::make_unique<ErrorMonitor>(config);
    g_errorMonitor->startMonitoring();
}

void shutdownGlobalErrorMonitoring() {
    if (g_errorMonitor) {
        g_errorMonitor->stopMonitoring();
        g_errorMonitor.reset();
    }
}

}  // namespace huntmaster
