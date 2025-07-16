#pragma once

#include "DebugLogger.h"

namespace huntmaster {

/**
 * @brief Centralized debug configuration for all Huntmaster components
 *
 * This class provides a centralized way to configure debug logging levels
 * for all components in the Huntmaster engine. It can be used to enable
 * different levels of debugging output for different components.
 */
class DebugConfig {
   public:
    /**
     * @brief Initialize debug configuration with default settings
     */
    static void initialize() {
        // Set default log level to NONE for production
        DebugLogger::getInstance().setGlobalLogLevel(LogLevel::NONE);
    }

    /**
     * @brief Enable debug logging for all components
     * @param level The debug level to set
     */
    static void enableDebugLogging(LogLevel level = LogLevel::DEBUG) {
        DebugLogger::getInstance().setGlobalLogLevel(level);
    }

    /**
     * @brief Disable all debug logging
     */
    static void disableDebugLogging() {
        DebugLogger::getInstance().setGlobalLogLevel(LogLevel::NONE);
    }

    /**
     * @brief Enable debug logging for testing scenarios
     */
    static void enableTestDebugLogging() {
        DebugLogger::getInstance().setGlobalLogLevel(LogLevel::DEBUG);
    }

    /**
     * @brief Enable trace logging for detailed debugging
     */
    static void enableTraceLogging() {
        DebugLogger::getInstance().setGlobalLogLevel(LogLevel::TRACE);
    }

    /**
     * @brief Get current debug level
     */
    static LogLevel getCurrentLevel() { return DebugLogger::getInstance().getGlobalLogLevel(); }

    /**
     * @brief Enable component-specific debugging
     */
    static void enableComponentDebug(Component component, LogLevel level = LogLevel::DEBUG) {
        DebugLogger::getInstance().setComponentLogLevel(component, level);
    }

    /**
     * @brief Quick setup for common debugging scenarios
     */
    static void setupToolsDebug() {
        DebugLogger::getInstance().setGlobalLogLevel(LogLevel::INFO);
        DebugLogger::getInstance().setComponentLogLevel(Component::TOOLS, LogLevel::DEBUG);
        DebugLogger::getInstance().enableConsoleOutput(true);
    }

    static void setupEngineDebug() {
        DebugLogger::getInstance().setGlobalLogLevel(LogLevel::WARN);
        DebugLogger::getInstance().setComponentLogLevel(Component::UNIFIED_ENGINE, LogLevel::DEBUG);
        DebugLogger::getInstance().setComponentLogLevel(Component::MFCC_PROCESSOR, LogLevel::DEBUG);
        DebugLogger::getInstance().setComponentLogLevel(Component::DTW_COMPARATOR, LogLevel::DEBUG);
        DebugLogger::getInstance().enableConsoleOutput(true);
    }

    static void setupFullDebug() {
        DebugLogger::getInstance().setGlobalLogLevel(LogLevel::DEBUG);
        DebugLogger::getInstance().enableConsoleOutput(true);
        DebugLogger::getInstance().enableFileLogging("huntmaster_full_debug.log");
        DebugLogger::getInstance().enableTimestamps(true);
        DebugLogger::getInstance().enableThreadIds(true);
    }
};

/**
 * @brief RAII-style debug scope for temporary debug enabling
 *
 * This class enables debug logging for a specific scope and automatically
 * restores the previous logging level when the scope ends.
 */
class DebugScope {
   private:
    LogLevel previous_level_;

   public:
    explicit DebugScope(LogLevel temp_level) {
        previous_level_ = DebugConfig::getCurrentLevel();
        DebugLogger::getInstance().setGlobalLogLevel(temp_level);
    }

    ~DebugScope() { DebugLogger::getInstance().setGlobalLogLevel(previous_level_); }

    // Non-copyable and non-movable
    DebugScope(const DebugScope&) = delete;
    DebugScope& operator=(const DebugScope&) = delete;
    DebugScope(DebugScope&&) = delete;
    DebugScope& operator=(DebugScope&&) = delete;
};

/**
 * @brief Component-specific debug scope
 */
class ComponentDebugScope {
   private:
    Component component_;
    LogLevel previous_level_;

   public:
    explicit ComponentDebugScope(Component component, LogLevel temp_level) : component_(component) {
        previous_level_ = DebugLogger::getInstance().getComponentLogLevel(component);
        DebugLogger::getInstance().setComponentLogLevel(component, temp_level);
    }

    ~ComponentDebugScope() {
        DebugLogger::getInstance().setComponentLogLevel(component_, previous_level_);
    }

    // Non-copyable and non-movable
    ComponentDebugScope(const ComponentDebugScope&) = delete;
    ComponentDebugScope& operator=(const ComponentDebugScope&) = delete;
    ComponentDebugScope(ComponentDebugScope&&) = delete;
    ComponentDebugScope& operator=(ComponentDebugScope&&) = delete;
};

}  // namespace huntmaster

// Convenience macros for debug scopes
#define HUNTMASTER_DEBUG_SCOPE(level) huntmaster::DebugScope debug_scope_(level)
#define HUNTMASTER_TRACE_SCOPE() HUNTMASTER_DEBUG_SCOPE(huntmaster::LogLevel::TRACE)
#define HUNTMASTER_DEBUG_SCOPE_ENABLED() HUNTMASTER_DEBUG_SCOPE(huntmaster::LogLevel::DEBUG)
#define HUNTMASTER_COMPONENT_DEBUG_SCOPE(component, level) \
    huntmaster::ComponentDebugScope component_debug_scope_(component, level)
