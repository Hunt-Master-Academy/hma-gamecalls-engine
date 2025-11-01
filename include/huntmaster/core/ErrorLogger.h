#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <huntmaster/core/DebugLogger.h>

namespace huntmaster {

/**
 * @brief Error severity levels for comprehensive error tracking
 */
enum class ErrorSeverity : int {
    CRITICAL = 1,  ///< System-breaking errors that require immediate attention
    HIGH = 2,      ///< Significant errors that affect functionality
    MEDIUM = 3,    ///< Moderate errors that may impact performance
    LOW = 4,       ///< Minor errors that don't affect core functionality
    INFO = 5       ///< Informational error events
};

/**
 * @brief Error categories for better classification
 */
enum class ErrorCategory : int {
    INITIALIZATION = 1,  ///< Component initialization failures
    MEMORY = 2,          ///< Memory allocation/deallocation errors
    IO = 3,              ///< Input/Output operation errors
    PROCESSING = 4,      ///< Audio/Data processing errors
    CONFIGURATION = 5,   ///< Configuration validation errors
    RESOURCE = 6,        ///< Resource management errors
    NETWORK = 7,         ///< Network-related errors
    VALIDATION = 8,      ///< Input validation errors
    THREAD = 9,          ///< Threading/concurrency errors
    SYSTEM = 10          ///< System-level errors
};

/**
 * @brief Comprehensive error information structure
 */
struct ErrorInfo {
    Component component;                                   ///< Component where error occurred
    ErrorSeverity severity;                                ///< Error severity level
    ErrorCategory category;                                ///< Error category
    std::string errorCode;                                 ///< Unique error code
    std::string message;                                   ///< Human-readable error message
    std::string details;                                   ///< Detailed error information
    std::string function;                                  ///< Function where error occurred
    std::string file;                                      ///< Source file name
    int line;                                              ///< Source line number
    std::chrono::system_clock::time_point timestamp;       ///< When error occurred
    std::unordered_map<std::string, std::string> context;  ///< Additional context data

    ErrorInfo() = default;

    ErrorInfo(Component comp,
              ErrorSeverity sev,
              ErrorCategory cat,
              const std::string& code,
              const std::string& msg)
        : component(comp), severity(sev), category(cat), errorCode(code), message(msg), line(0),
          timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Error callback function type
 */
using ErrorCallback = std::function<void(const ErrorInfo&)>;

/**
 * @brief Error statistics for monitoring
 */
struct ErrorStats {
    size_t totalErrors = 0;
    size_t criticalErrors = 0;
    size_t highErrors = 0;
    size_t mediumErrors = 0;
    size_t lowErrors = 0;
    size_t infoErrors = 0;
    std::unordered_map<Component, size_t> errorsByComponent;
    std::unordered_map<ErrorCategory, size_t> errorsByCategory;
    std::chrono::system_clock::time_point lastError;
};

/**
 * @brief Comprehensive error logging and tracking system
 *
 * This class extends the basic DebugLogger with specialized error tracking,
 * categorization, and monitoring capabilities. It provides:
 * - Structured error information with severity and categories
 * - Error statistics and monitoring
 * - Callback mechanisms for error handling
 * - Integration with existing DebugLogger infrastructure
 */
class ErrorLogger {
  public:
    /**
     * @brief Get the singleton instance
     */
    static ErrorLogger& getInstance();

    /**
     * @brief Log a comprehensive error with full context
     *
     * @param component Component where error occurred
     * @param severity Error severity level
     * @param category Error category
     * @param errorCode Unique error identifier
     * @param message Primary error message
     * @param details Detailed error information (optional)
     * @param file Source file name (__FILE__)
     * @param line Source line number (__LINE__)
     * @param function Function name (__func__)
     */
    void logError(Component component,
                  ErrorSeverity severity,
                  ErrorCategory category,
                  const std::string& errorCode,
                  const std::string& message,
                  const std::string& details = "",
                  const char* file = nullptr,
                  int line = 0,
                  const char* function = nullptr);

    /**
     * @brief Log error with additional context data
     */
    void logErrorWithContext(Component component,
                             ErrorSeverity severity,
                             ErrorCategory category,
                             const std::string& errorCode,
                             const std::string& message,
                             const std::unordered_map<std::string, std::string>& context,
                             const char* file = nullptr,
                             int line = 0,
                             const char* function = nullptr);

    /**
     * @brief Register error callback for real-time error handling
     */
    void registerErrorCallback(ErrorCallback callback);

    /**
     * @brief Get current error statistics
     */
    ErrorStats getErrorStats() const;

    /**
     * @brief Clear error statistics
     */
    void clearErrorStats();

    /**
     * @brief Get recent errors (last N errors)
     */
    std::vector<ErrorInfo> getRecentErrors(size_t count = 10) const;

    /**
     * @brief Clear recent errors buffer (releases memory)
     *
     * [20251102-FIX-003] Added to prevent global error accumulation
     * across sessions. Clears recentErrors_ vector and shrinks capacity
     * to release memory back to OS.
     */
    void clearRecentErrors();

    /**
     * @brief Check if error rate exceeds threshold
     */
    bool isErrorRateHigh(size_t errorsPerMinute = 60) const;

    /**
     * @brief Enable/disable error logging
     */
    void setEnabled(bool enabled);

    /**
     * @brief Set minimum severity level for logging
     */
    void setMinimumSeverity(ErrorSeverity minSeverity);

    /**
     * @brief Get string representation of severity
     */
    static std::string severityToString(ErrorSeverity severity);

    /**
     * @brief Get string representation of category
     */
    static std::string categoryToString(ErrorCategory category);

  private:
    ErrorLogger() = default;
    ~ErrorLogger() = default;
    ErrorLogger(const ErrorLogger&) = delete;
    ErrorLogger& operator=(const ErrorLogger&) = delete;

    mutable std::mutex errorMutex_;
    bool enabled_ = true;
    ErrorSeverity minSeverity_ = ErrorSeverity::LOW;

    std::vector<ErrorInfo> recentErrors_;
    static constexpr size_t MAX_RECENT_ERRORS = 1000;

    ErrorStats stats_;
    std::vector<ErrorCallback> callbacks_;

    void notifyCallbacks(const ErrorInfo& error);
    void updateStats(const ErrorInfo& error);
    void addToRecentErrors(const ErrorInfo& error);
};

// Convenience macros for error logging
#define LOG_CRITICAL_ERROR(component, category, code, message)                           \
    huntmaster::ErrorLogger::getInstance().logError(component,                           \
                                                    huntmaster::ErrorSeverity::CRITICAL, \
                                                    category,                            \
                                                    code,                                \
                                                    message,                             \
                                                    "",                                  \
                                                    __FILE__,                            \
                                                    __LINE__,                            \
                                                    __func__)

#define LOG_HIGH_ERROR(component, category, code, message)                           \
    huntmaster::ErrorLogger::getInstance().logError(component,                       \
                                                    huntmaster::ErrorSeverity::HIGH, \
                                                    category,                        \
                                                    code,                            \
                                                    message,                         \
                                                    "",                              \
                                                    __FILE__,                        \
                                                    __LINE__,                        \
                                                    __func__)

#define LOG_MEDIUM_ERROR(component, category, code, message)                           \
    huntmaster::ErrorLogger::getInstance().logError(component,                         \
                                                    huntmaster::ErrorSeverity::MEDIUM, \
                                                    category,                          \
                                                    code,                              \
                                                    message,                           \
                                                    "",                                \
                                                    __FILE__,                          \
                                                    __LINE__,                          \
                                                    __func__)

#define LOG_LOW_ERROR(component, category, code, message)                           \
    huntmaster::ErrorLogger::getInstance().logError(component,                      \
                                                    huntmaster::ErrorSeverity::LOW, \
                                                    category,                       \
                                                    code,                           \
                                                    message,                        \
                                                    "",                             \
                                                    __FILE__,                       \
                                                    __LINE__,                       \
                                                    __func__)

#define LOG_DETAILED_ERROR(component, severity, category, code, message, details) \
    huntmaster::ErrorLogger::getInstance().logError(                              \
        component, severity, category, code, message, details, __FILE__, __LINE__, __func__)

#define LOG_ERROR_WITH_CONTEXT(component, severity, category, code, message, context) \
    huntmaster::ErrorLogger::getInstance().logErrorWithContext(                       \
        component, severity, category, code, message, context, __FILE__, __LINE__, __func__)

// Component-specific error logging macros
#define LOG_AUDIO_ENGINE_ERROR(severity, category, code, message) \
    LOG_##severity##_ERROR(huntmaster::Component::AUDIO_ENGINE, category, code, message)

#define LOG_MFCC_ERROR(severity, category, code, message) \
    LOG_##severity##_ERROR(huntmaster::Component::MFCC_PROCESSOR, category, code, message)

#define LOG_UNIFIED_ENGINE_ERROR(severity, category, code, message) \
    LOG_##severity##_ERROR(huntmaster::Component::UNIFIED_ENGINE, category, code, message)

#define LOG_REALTIME_ERROR(severity, category, code, message) \
    LOG_##severity##_ERROR(huntmaster::Component::REALTIME_PROCESSOR, category, code, message)

}  // namespace huntmaster
