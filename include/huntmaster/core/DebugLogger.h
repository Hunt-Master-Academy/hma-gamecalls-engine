#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

namespace huntmaster {

/**
 * @brief Debug logging levels following standard conventions
 */
enum class LogLevel : int {
    NONE = 0,   // No logging
    ERROR = 1,  // Critical errors only
    WARN = 2,   // Warnings and errors
    INFO = 3,   // General information
    DEBUG = 4,  // Debug information
    TRACE = 5   // Detailed trace information
};

/**
 * @brief Component identifiers for targeted debugging
 */
enum class Component : int {
    GENERAL = 0,
    UNIFIED_ENGINE = 1,
    MFCC_PROCESSOR = 2,
    DTW_COMPARATOR = 3,
    VAD = 4,
    REALTIME_PROCESSOR = 5,
    AUDIO_BUFFER_POOL = 6,
    AUDIO_LEVEL_PROCESSOR = 7,
    WAVEFORM_GENERATOR = 8,
    SPECTROGRAM_PROCESSOR = 9,
    REALTIME_SCORER = 10,
    TOOLS = 11,
    AUDIO_ENGINE = 12,
    FEATURE_EXTRACTION = 13,
    SIMILARITY_ANALYSIS = 14,
    PERFORMANCE = 15,
    MEMORY_MANAGER = 16
};

// Alias for backward compatibility with tools
using DebugComponent = Component;
using DebugLevel = LogLevel;

/**
 * @brief Thread-safe, configurable debug logger with component-specific levels
 */
class DebugLogger {
  public:
    static DebugLogger& getInstance();

    // Global log level control
    void setGlobalLogLevel(LogLevel level);
    LogLevel getGlobalLogLevel() const;

    // Component-specific log level control
    void setComponentLogLevel(Component component, LogLevel level);
    LogLevel getComponentLogLevel(Component component) const;

    // Enable/disable console output
    void enableConsoleOutput(bool enable = true);
    void disableConsoleOutput();

    // Enable/disable file logging
    void enableFileLogging(const std::string& filename = "huntmaster_debug.log");
    void disableFileLogging();

    // Enable/disable timestamps
    void enableTimestamps(bool enable = true);
    void enableThreadIds(bool enable = true);

    // Main logging function
    void log(Component component,
             LogLevel level,
             const std::string& message,
             const char* file = nullptr,
             int line = 0,
             const char* function = nullptr);

    // Convenience methods
    void error(Component component,
               const std::string& message,
               const char* file = nullptr,
               int line = 0,
               const char* function = nullptr);
    void warn(Component component,
              const std::string& message,
              const char* file = nullptr,
              int line = 0,
              const char* function = nullptr);
    void info(Component component,
              const std::string& message,
              const char* file = nullptr,
              int line = 0,
              const char* function = nullptr);
    void debug(Component component,
               const std::string& message,
               const char* file = nullptr,
               int line = 0,
               const char* function = nullptr);
    void trace(Component component,
               const std::string& message,
               const char* file = nullptr,
               int line = 0,
               const char* function = nullptr);

  private:
    DebugLogger() = default;
    ~DebugLogger();

    // Thread-safe logging implementation
    void logImpl(Component component,
                 LogLevel level,
                 const std::string& message,
                 const char* file,
                 int line,
                 const char* function);

    std::string formatMessage(Component component,
                              LogLevel level,
                              const std::string& message,
                              const char* file,
                              int line,
                              const char* function) const;

    std::string getLevelString(LogLevel level) const;
    std::string getComponentString(Component component) const;
    std::string getCurrentTimestamp() const;

    // Configuration
    LogLevel globalLogLevel_{LogLevel::NONE};
    std::unordered_map<Component, LogLevel> componentLogLevels_;

    bool consoleOutputEnabled_{true};
    bool fileLoggingEnabled_{false};
    bool timestampsEnabled_{true};
    bool threadIdsEnabled_{false};

    // Output streams
    std::unique_ptr<std::ofstream> logFile_;
    mutable std::mutex logMutex_;
};

// [20251229-BINDINGS-FIX-023] Conditional logging macros - disable when DISABLE_LOGGING is defined
#ifdef DISABLE_LOGGING
// No-op macros when logging is disabled (for Node-API bindings safety)
#define LOG_ERROR(component, message) ((void)0)
#define LOG_WARN(component, message) ((void)0)
#define LOG_INFO(component, message) ((void)0)
#define LOG_DEBUG(component, message) ((void)0)
#define LOG_TRACE(component, message) ((void)0)
#define LOG_IF_ERROR(component, message) ((void)0)
#define LOG_IF_WARN(component, message) ((void)0)
#define LOG_IF_INFO(component, message) ((void)0)
#define LOG_IF_DEBUG(component, message) ((void)0)
#define LOG_IF_TRACE(component, message) ((void)0)
#define LOG_STREAM(component, level) \
    if (false)                       \
    std::cout

#else
// Convenience macros for cleaner logging
#define LOG_ERROR(component, message) \
    huntmaster::DebugLogger::getInstance().error(component, message, __FILE__, __LINE__, __func__)

#define LOG_WARN(component, message) \
    huntmaster::DebugLogger::getInstance().warn(component, message, __FILE__, __LINE__, __func__)

#define LOG_INFO(component, message) \
    huntmaster::DebugLogger::getInstance().info(component, message, __FILE__, __LINE__, __func__)

#define LOG_DEBUG(component, message) \
    huntmaster::DebugLogger::getInstance().debug(component, message, __FILE__, __LINE__, __func__)

#define LOG_TRACE(component, message) \
    huntmaster::DebugLogger::getInstance().trace(component, message, __FILE__, __LINE__, __func__)

// Conditional logging macros (only log if component level allows)
#define LOG_IF_ERROR(component, message)                                       \
    if (huntmaster::DebugLogger::getInstance().getComponentLogLevel(component) \
        >= huntmaster::LogLevel::ERROR)                                        \
    LOG_ERROR(component, message)

#define LOG_IF_WARN(component, message)                                        \
    if (huntmaster::DebugLogger::getInstance().getComponentLogLevel(component) \
        >= huntmaster::LogLevel::WARN)                                         \
    LOG_WARN(component, message)

#define LOG_IF_INFO(component, message)                                        \
    if (huntmaster::DebugLogger::getInstance().getComponentLogLevel(component) \
        >= huntmaster::LogLevel::INFO)                                         \
    LOG_INFO(component, message)

#define LOG_IF_DEBUG(component, message)                                       \
    if (huntmaster::DebugLogger::getInstance().getComponentLogLevel(component) \
        >= huntmaster::LogLevel::DEBUG)                                        \
    LOG_DEBUG(component, message)

#define LOG_IF_TRACE(component, message)                                       \
    if (huntmaster::DebugLogger::getInstance().getComponentLogLevel(component) \
        >= huntmaster::LogLevel::TRACE)                                        \
    LOG_TRACE(component, message)

// Stream-style logging macros
#define LOG_STREAM(component, level) \
    huntmaster::LogStream(component, level, __FILE__, __LINE__, __func__)

#endif  // DISABLE_LOGGING

/**
 * @brief Stream-style logging helper
 */
class LogStream {
  public:
    LogStream(Component component, LogLevel level, const char* file, int line, const char* function)
        : component_(component), level_(level), file_(file), line_(line), function_(function) {}

    ~LogStream() {
        DebugLogger::getInstance().log(component_, level_, stream_.str(), file_, line_, function_);
    }

    template <typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

  private:
    Component component_;
    LogLevel level_;
    const char* file_;
    int line_;
    const char* function_;
    std::ostringstream stream_;
};

}  // namespace huntmaster
