#include "huntmaster/core/DebugLogger.h"

#include <cstring>
#include <iomanip>
#include <sstream>

namespace huntmaster {

DebugLogger& DebugLogger::getInstance() {
    static DebugLogger instance;
    return instance;
}

DebugLogger::~DebugLogger() {
    if (logFile_) {
        logFile_->close();
    }
}

void DebugLogger::setGlobalLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex_);
    globalLogLevel_ = level;
}

LogLevel DebugLogger::getGlobalLogLevel() const {
    std::lock_guard<std::mutex> lock(logMutex_);
    return globalLogLevel_;
}

void DebugLogger::setComponentLogLevel(Component component, LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex_);
    componentLogLevels_[component] = level;
}

LogLevel DebugLogger::getComponentLogLevel(Component component) const {
    std::lock_guard<std::mutex> lock(logMutex_);
    auto it = componentLogLevels_.find(component);
    return (it != componentLogLevels_.end()) ? it->second : globalLogLevel_;
}

void DebugLogger::enableConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(logMutex_);
    consoleOutputEnabled_ = enable;
}

void DebugLogger::disableConsoleOutput() {
    enableConsoleOutput(false);
}

void DebugLogger::enableFileLogging(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex_);
    logFile_ = std::make_unique<std::ofstream>(filename, std::ios::app);
    fileLoggingEnabled_ = logFile_->is_open();
}

void DebugLogger::disableFileLogging() {
    std::lock_guard<std::mutex> lock(logMutex_);
    if (logFile_) {
        logFile_->close();
        logFile_.reset();
    }
    fileLoggingEnabled_ = false;
}

void DebugLogger::enableTimestamps(bool enable) {
    std::lock_guard<std::mutex> lock(logMutex_);
    timestampsEnabled_ = enable;
}

void DebugLogger::enableThreadIds(bool enable) {
    std::lock_guard<std::mutex> lock(logMutex_);
    threadIdsEnabled_ = enable;
}

void DebugLogger::log(Component component,
                      LogLevel level,
                      const std::string& message,
                      const char* file,
                      int line,
                      const char* function) {
    // Quick check before expensive operations
    LogLevel componentLevel = getComponentLogLevel(component);
    if (level > componentLevel) {
        return;
    }

    logImpl(component, level, message, file, line, function);
}

void DebugLogger::error(Component component,
                        const std::string& message,
                        const char* file,
                        int line,
                        const char* function) {
    log(component, LogLevel::ERROR, message, file, line, function);
}

void DebugLogger::warn(Component component,
                       const std::string& message,
                       const char* file,
                       int line,
                       const char* function) {
    log(component, LogLevel::WARN, message, file, line, function);
}

void DebugLogger::info(Component component,
                       const std::string& message,
                       const char* file,
                       int line,
                       const char* function) {
    log(component, LogLevel::INFO, message, file, line, function);
}

void DebugLogger::debug(Component component,
                        const std::string& message,
                        const char* file,
                        int line,
                        const char* function) {
    log(component, LogLevel::DEBUG, message, file, line, function);
}

void DebugLogger::trace(Component component,
                        const std::string& message,
                        const char* file,
                        int line,
                        const char* function) {
    log(component, LogLevel::TRACE, message, file, line, function);
}

void DebugLogger::logImpl(Component component,
                          LogLevel level,
                          const std::string& message,
                          const char* file,
                          int line,
                          const char* function) {
    std::lock_guard<std::mutex> lock(logMutex_);

    std::string formattedMessage = formatMessage(component, level, message, file, line, function);

    // Console output
    if (consoleOutputEnabled_) {
        // Color-coded output for different levels
        switch (level) {
            case LogLevel::ERROR:
                std::cerr << "\033[31m" << formattedMessage << "\033[0m" << std::endl;
                break;
            case LogLevel::WARN:
                std::cerr << "\033[33m" << formattedMessage << "\033[0m" << std::endl;
                break;
            case LogLevel::INFO:
                std::cout << "\033[32m" << formattedMessage << "\033[0m" << std::endl;
                break;
            case LogLevel::DEBUG:
                std::cout << "\033[36m" << formattedMessage << "\033[0m" << std::endl;
                break;
            case LogLevel::TRACE:
                std::cout << "\033[37m" << formattedMessage << "\033[0m" << std::endl;
                break;
            default:
                std::cout << formattedMessage << std::endl;
        }
    }

    // File output
    if (fileLoggingEnabled_ && logFile_) {
        *logFile_ << formattedMessage << std::endl;
        logFile_->flush();
    }
}

std::string DebugLogger::formatMessage(Component component,
                                       LogLevel level,
                                       const std::string& message,
                                       const char* file,
                                       int line,
                                       const char* function) const {
    std::ostringstream oss;

    // Timestamp
    if (timestampsEnabled_) {
        oss << "[" << getCurrentTimestamp() << "] ";
    }

    // Thread ID
    if (threadIdsEnabled_) {
        oss << "[T:" << std::this_thread::get_id() << "] ";
    }

    // Log level
    oss << "[" << getLevelString(level) << "] ";

    // Component
    oss << "[" << getComponentString(component) << "] ";

    // Function and location (for DEBUG and TRACE levels)
    if (level >= LogLevel::DEBUG) {
        // Extract filename from full path
        const char* filename = strrchr(file, '/');
        if (!filename)
            filename = strrchr(file, '\\');
        if (!filename)
            filename = file;
        else
            filename++;  // Skip the separator

        oss << function << "() [" << filename << ":" << line << "] ";
    }

    // Message
    oss << message;

    return oss.str();
}

std::string DebugLogger::getLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::WARN:
            return "WARN ";
        case LogLevel::INFO:
            return "INFO ";
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::TRACE:
            return "TRACE";
        default:
            return "UNKN ";
    }
}

std::string DebugLogger::getComponentString(Component component) const {
    switch (component) {
        case Component::GENERAL:
            return "GENERAL";
        case Component::UNIFIED_ENGINE:
            return "ENGINE ";
        case Component::MFCC_PROCESSOR:
            return "MFCC   ";
        case Component::DTW_COMPARATOR:
            return "DTW    ";
        case Component::VAD:
            return "VAD    ";
        case Component::REALTIME_PROCESSOR:
            return "RTPROC ";
        case Component::AUDIO_BUFFER_POOL:
            return "BUFPOOL";
        case Component::AUDIO_LEVEL_PROCESSOR:
            return "LEVELS ";
        case Component::WAVEFORM_GENERATOR:
            return "WAVE   ";
        case Component::REALTIME_SCORER:
            return "SCORER ";
        case Component::TOOLS:
            return "TOOLS  ";
        default:
            return "UNKN   ";
    }
}

std::string DebugLogger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

}  // namespace huntmaster
