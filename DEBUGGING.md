# Huntmaster Engine Debugging Guide

## Test Coverage & Next Steps

- All debugging features validated by unit and integration tests:
  - DebugLogger, macros, error codes, diagnostic output
  - Thread safety, performance monitoring, component filtering

### Edge Case & Stress Testing Plan

1. **Edge Case Testing:**

   - Debug output under buffer overflow/underflow
   - Error handling for invalid config, audio data, and channel count
   - Debugging with large audio streams and rapid config changes

2. **Stress Testing:**

   - High-frequency debug output (multi-threaded)
   - Simultaneous session debugging under load

3. **Release Candidate Checklist:**
   - All debugging outputs validated
   - No deadlocks or performance bottlenecks
   - Documentation up to date

## Overview

The Huntmaster Engine includes a comprehensive debugging infrastructure designed to help developers diagnose issues, monitor performance, and understand the behavior of the audio processing system. This guide explains how to use the debugging capabilities effectively.

## Debugging Architecture

### DebugLogger Class

The core of the debugging system is the `DebugLogger` class, which provides:

- **Thread-safe logging** - Safe for use in multi-threaded environments
- **Component-specific levels** - Different log levels for different parts of the system
- **Multiple output formats** - Console and file logging with timestamps
- **Performance monitoring** - Built-in timing and metrics collection

### Log Levels

The system supports 5 log levels in order of verbosity:

1. **NONE** (0) - No logging
2. **ERROR** (1) - Critical errors only
3. **WARN** (2) - Warnings and errors
4. **INFO** (3) - General information
5. **DEBUG** (4) - Debug information
6. **TRACE** (5) - Detailed trace information

### Debug Components

The system is organized into 14 components for targeted debugging:

| Component             | Purpose                            |
| --------------------- | ---------------------------------- |
| GENERAL               | General system operations          |
| UNIFIED_ENGINE        | Core engine functionality          |
| MFCC_PROCESSOR        | MFCC feature extraction            |
| DTW_COMPARATOR        | Dynamic Time Warping comparison    |
| VAD                   | Voice Activity Detection           |
| REALTIME_PROCESSOR    | Real-time processing               |
| AUDIO_BUFFER_POOL     | Audio buffer management            |
| AUDIO_LEVEL_PROCESSOR | Audio level processing             |
| WAVEFORM_GENERATOR    | Audio waveform generation          |
| REALTIME_SCORER       | Real-time scoring                  |
| TOOLS                 | Development tools                  |
| AUDIO_ENGINE          | Audio engine operations            |
| FEATURE_EXTRACTION    | Feature extraction processes       |
| SIMILARITY_ANALYSIS   | Similarity analysis algorithms     |
| PERFORMANCE           | Performance monitoring and metrics |

## Tool-Specific Debugging

### Interactive Recorder

The interactive recorder supports comprehensive debugging options:

```bash
# Basic usage
./interactive_recorder

# Debug options
./interactive_recorder --debug              # Enable debug logging
./interactive_recorder --trace              # Enable trace logging
./interactive_recorder --verbose            # Enable verbose output
./interactive_recorder --engine-debug       # Engine-specific debugging
./interactive_recorder --recording-debug    # Recording operations
./interactive_recorder --playback-debug     # Playback operations
./interactive_recorder --analysis-debug     # Analysis operations
./interactive_recorder --performance        # Performance metrics
./interactive_recorder --help               # Show help
```

### MFCC Debugging Tool

Specialized tool for debugging MFCC feature extraction:

```bash
# Test with default files
./test_mfcc_debugging

# Debug MFCC processing
./test_mfcc_debugging --debug --mfcc-debug

# Frame-by-frame analysis
./test_mfcc_debugging --frame-debug --trace

# Test specific file
./test_mfcc_debugging --wav-debug test.wav
```

### DTW Similarity Debugging

Tool for debugging Dynamic Time Warping similarity analysis:

```bash
# Basic DTW debugging
./debug_dtw_similarity --debug

# Detailed similarity analysis
./debug_dtw_similarity --trace --performance
```

## Programming API

### Using DebugLogger in Code

```cpp
#include <huntmaster/core/DebugLogger.h>

// Get logger instance
auto& logger = huntmaster::DebugLogger::getInstance();

// Configure global log level
logger.setGlobalLogLevel(huntmaster::LogLevel::DEBUG);

// Configure component-specific levels
logger.setComponentLogLevel(huntmaster::Component::AUDIO_ENGINE,
                           huntmaster::LogLevel::TRACE);

// Log messages
logger.log(huntmaster::Component::AUDIO_ENGINE,
           huntmaster::LogLevel::INFO,
           "Audio engine initialized");
```

### Using Convenience Macros

```cpp
// Simple logging macros
LOG_ERROR(huntmaster::Component::AUDIO_ENGINE, "Critical error occurred");
LOG_WARN(huntmaster::Component::AUDIO_ENGINE, "Warning message");
LOG_INFO(huntmaster::Component::AUDIO_ENGINE, "Information message");
LOG_DEBUG(huntmaster::Component::AUDIO_ENGINE, "Debug message");
LOG_TRACE(huntmaster::Component::AUDIO_ENGINE, "Trace message");

// Conditional logging (only logs if component level allows)
LOG_IF_DEBUG(huntmaster::Component::AUDIO_ENGINE, "Debug if enabled");
```

### Performance Monitoring

```cpp
// Create performance monitor
PerformanceMonitor monitor("Audio processing", true);

// Add checkpoints
monitor.checkpoint("Audio loaded");
// ... processing ...
monitor.checkpoint("Features extracted");
// ... more processing ...
// Monitor automatically logs completion time in destructor
```

## Configuration

### Global Configuration

```cpp
// Enable/disable console output
logger.enableConsoleOutput(true);

// Enable file logging
logger.enableFileLogging("huntmaster_debug.log");

// Configure timestamps and thread IDs
logger.enableTimestamps(true);
logger.enableThreadIds(true);
```

### Component-Specific Configuration

```cpp
// Set different levels for different components
logger.setComponentLogLevel(huntmaster::Component::AUDIO_ENGINE,
                           huntmaster::LogLevel::INFO);
logger.setComponentLogLevel(huntmaster::Component::PERFORMANCE,
                           huntmaster::LogLevel::DEBUG);
logger.setComponentLogLevel(huntmaster::Component::MFCC_PROCESSOR,
                           huntmaster::LogLevel::TRACE);
```

## Best Practices

### 1. Use Appropriate Log Levels

- **ERROR**: Critical failures that prevent operation
- **WARN**: Issues that don't prevent operation but should be noted
- **INFO**: General operational information
- **DEBUG**: Detailed information for debugging
- **TRACE**: Very detailed execution flow information

### 2. Use Component-Specific Logging

Always specify the appropriate component when logging:

```cpp
// Good
LOG_INFO(huntmaster::Component::AUDIO_ENGINE, "Engine started");

// Less specific
LOG_INFO(huntmaster::Component::GENERAL, "Engine started");
```

### 3. Performance Monitoring

Use performance monitors for timing-critical operations:

```cpp
void processAudio() {
    PerformanceMonitor monitor("Audio processing", true);

    // Your processing code here

    monitor.checkpoint("Pre-processing complete");
    // More processing
    monitor.checkpoint("Feature extraction complete");
}
```

### 4. Conditional Compilation

For production builds, consider using conditional compilation:

```cpp
#ifdef DEBUG_BUILD
    LOG_DEBUG(huntmaster::Component::AUDIO_ENGINE, "Debug info");
#endif
```

## Troubleshooting

### Common Issues

1. **No debug output**: Check that global log level is set appropriately
2. **Missing component logs**: Verify component-specific log levels
3. **Performance overhead**: Use conditional logging macros in hot paths
4. **Thread safety**: The logger is thread-safe, but your log messages should be meaningful

### Debug Environment Setup

For development, use this typical setup:

```cpp
// In main() or initialization
auto& logger = huntmaster::DebugLogger::getInstance();
logger.setGlobalLogLevel(huntmaster::LogLevel::DEBUG);
logger.enableConsoleOutput(true);
logger.enableFileLogging("debug.log");
logger.enableTimestamps(true);
```

## Tool Integration

All 10 tools in the Huntmaster Engine have been enhanced with debugging capabilities:

- Command-line argument parsing for debug options
- Performance monitoring integration
- Component-specific debug logging
- Comprehensive error handling and logging

This provides consistent debugging experience across all development and analysis tools.
