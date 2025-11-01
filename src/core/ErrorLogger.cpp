#include <algorithm>
#include <iomanip>
#include <sstream>

#include <huntmaster/core/DebugLogger.h>
#include <huntmaster/core/ErrorLogger.h>

namespace huntmaster {

ErrorLogger& ErrorLogger::getInstance() {
    static ErrorLogger instance;
    return instance;
}

void ErrorLogger::logError(Component component,
                           ErrorSeverity severity,
                           ErrorCategory category,
                           const std::string& errorCode,
                           const std::string& message,
                           const std::string& details,
                           const char* file,
                           int line,
                           const char* function) {
    if (!enabled_ || severity > minSeverity_) {
        return;
    }

    ErrorInfo error(component, severity, category, errorCode, message);
    error.details = details;
    error.file = file ? file : "";
    error.line = line;
    error.function = function ? function : "";

    std::lock_guard<std::mutex> lock(errorMutex_);

    updateStats(error);
    addToRecentErrors(error);
    notifyCallbacks(error);

    // Also log to DebugLogger for integrated logging
    LogLevel debugLevel;
    switch (severity) {
        case ErrorSeverity::CRITICAL:
        case ErrorSeverity::HIGH:
            debugLevel = LogLevel::ERROR;
            break;
        case ErrorSeverity::MEDIUM:
            debugLevel = LogLevel::WARN;
            break;
        case ErrorSeverity::LOW:
        case ErrorSeverity::INFO:
            debugLevel = LogLevel::INFO;
            break;
    }

    std::ostringstream logMessage;
    logMessage << "[" << severityToString(severity) << "|" << categoryToString(category) << "|"
               << errorCode << "] " << message;
    if (!details.empty()) {
        logMessage << " - " << details;
    }

    DebugLogger::getInstance().log(component, debugLevel, logMessage.str(), file, line, function);
}

void ErrorLogger::logErrorWithContext(Component component,
                                      ErrorSeverity severity,
                                      ErrorCategory category,
                                      const std::string& errorCode,
                                      const std::string& message,
                                      const std::unordered_map<std::string, std::string>& context,
                                      const char* file,
                                      int line,
                                      const char* function) {
    if (!enabled_ || severity > minSeverity_) {
        return;
    }

    ErrorInfo error(component, severity, category, errorCode, message);
    error.context = context;
    error.file = file ? file : "";
    error.line = line;
    error.function = function ? function : "";

    // Build details from context
    std::ostringstream detailsStream;
    for (const auto& [key, value] : context) {
        detailsStream << key << "=" << value << "; ";
    }
    error.details = detailsStream.str();

    std::lock_guard<std::mutex> lock(errorMutex_);

    updateStats(error);
    addToRecentErrors(error);
    notifyCallbacks(error);

    // Log to DebugLogger with context
    LogLevel debugLevel;
    switch (severity) {
        case ErrorSeverity::CRITICAL:
        case ErrorSeverity::HIGH:
            debugLevel = LogLevel::ERROR;
            break;
        case ErrorSeverity::MEDIUM:
            debugLevel = LogLevel::WARN;
            break;
        case ErrorSeverity::LOW:
        case ErrorSeverity::INFO:
            debugLevel = LogLevel::INFO;
            break;
    }

    std::ostringstream logMessage;
    logMessage << "[" << severityToString(severity) << "|" << categoryToString(category) << "|"
               << errorCode << "] " << message;
    if (!error.details.empty()) {
        logMessage << " [Context: " << error.details << "]";
    }

    DebugLogger::getInstance().log(component, debugLevel, logMessage.str(), file, line, function);
}

void ErrorLogger::registerErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    callbacks_.push_back(std::move(callback));
}

ErrorStats ErrorLogger::getErrorStats() const {
    std::lock_guard<std::mutex> lock(errorMutex_);
    return stats_;
}

void ErrorLogger::clearErrorStats() {
    std::lock_guard<std::mutex> lock(errorMutex_);
    stats_ = ErrorStats{};
    recentErrors_.clear();
}

std::vector<ErrorInfo> ErrorLogger::getRecentErrors(size_t count) const {
    std::lock_guard<std::mutex> lock(errorMutex_);

    if (recentErrors_.size() <= count) {
        return recentErrors_;
    }

    return std::vector<ErrorInfo>(recentErrors_.end() - count, recentErrors_.end());
}

bool ErrorLogger::isErrorRateHigh(size_t errorsPerMinute) const {
    std::lock_guard<std::mutex> lock(errorMutex_);

    auto now = std::chrono::system_clock::now();
    auto oneMinuteAgo = now - std::chrono::minutes(1);

    size_t recentErrorCount = 0;
    for (const auto& error : recentErrors_) {
        if (error.timestamp >= oneMinuteAgo) {
            recentErrorCount++;
        }
    }

    return recentErrorCount >= errorsPerMinute;
}

// [20251102-FIX-003] Clear recent errors buffer to prevent memory accumulation
void ErrorLogger::clearRecentErrors() {
    std::lock_guard<std::mutex> lock(errorMutex_);
    recentErrors_.clear();
    recentErrors_.shrink_to_fit();  // Release memory back to OS
}

void ErrorLogger::setEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    enabled_ = enabled;
}

void ErrorLogger::setMinimumSeverity(ErrorSeverity minSeverity) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    minSeverity_ = minSeverity;
}

std::string ErrorLogger::severityToString(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::CRITICAL:
            return "CRITICAL";
        case ErrorSeverity::HIGH:
            return "HIGH";
        case ErrorSeverity::MEDIUM:
            return "MEDIUM";
        case ErrorSeverity::LOW:
            return "LOW";
        case ErrorSeverity::INFO:
            return "INFO";
        default:
            return "UNKNOWN";
    }
}

std::string ErrorLogger::categoryToString(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::INITIALIZATION:
            return "INIT";
        case ErrorCategory::MEMORY:
            return "MEMORY";
        case ErrorCategory::IO:
            return "IO";
        case ErrorCategory::PROCESSING:
            return "PROCESSING";
        case ErrorCategory::CONFIGURATION:
            return "CONFIG";
        case ErrorCategory::RESOURCE:
            return "RESOURCE";
        case ErrorCategory::NETWORK:
            return "NETWORK";
        case ErrorCategory::VALIDATION:
            return "VALIDATION";
        case ErrorCategory::THREAD:
            return "THREAD";
        case ErrorCategory::SYSTEM:
            return "SYSTEM";
        default:
            return "UNKNOWN";
    }
}

void ErrorLogger::notifyCallbacks(const ErrorInfo& error) {
    for (const auto& callback : callbacks_) {
        try {
            callback(error);
        } catch (...) {
            // Ignore callback exceptions to prevent infinite error loops
        }
    }
}

void ErrorLogger::updateStats(const ErrorInfo& error) {
    stats_.totalErrors++;

    switch (error.severity) {
        case ErrorSeverity::CRITICAL:
            stats_.criticalErrors++;
            break;
        case ErrorSeverity::HIGH:
            stats_.highErrors++;
            break;
        case ErrorSeverity::MEDIUM:
            stats_.mediumErrors++;
            break;
        case ErrorSeverity::LOW:
            stats_.lowErrors++;
            break;
        case ErrorSeverity::INFO:
            stats_.infoErrors++;
            break;
    }

    stats_.errorsByComponent[error.component]++;
    stats_.errorsByCategory[error.category]++;
    stats_.lastError = error.timestamp;
}

void ErrorLogger::addToRecentErrors(const ErrorInfo& error) {
    recentErrors_.push_back(error);

    // Keep only the most recent errors
    if (recentErrors_.size() > MAX_RECENT_ERRORS) {
        recentErrors_.erase(recentErrors_.begin(),
                            recentErrors_.begin() + (recentErrors_.size() - MAX_RECENT_ERRORS));
    }
}

}  // namespace huntmaster
