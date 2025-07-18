# Huntmaster Engine Deployment Guide

## Overview

This guide covers the deployment and distribution of the Huntmaster Engine, including build configuration, debugging infrastructure, and integration considerations.

## Build Configuration

### Debug vs Release Builds

The engine supports both debug and release configurations:

#### Debug Build

```bash
# Configure for debug
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build with debug symbols
make -j$(nproc)
```

Debug builds include:

- Full debugging infrastructure enabled
- Symbol information for debugging
- Comprehensive logging capabilities
- Performance monitoring tools

#### Release Build

```bash
# Configure for release
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build optimized
make -j$(nproc)
```

Release builds feature:

- Optimized performance
- Minimal logging overhead
- Reduced binary size
- Production-ready configuration

## Debugging Infrastructure in Production

### Runtime Debug Configuration

The debugging system can be configured at runtime without recompilation:

```cpp
// Production deployment with configurable debugging
auto& logger = huntmaster::DebugLogger::getInstance();

// Default: minimal logging
logger.setGlobalLogLevel(huntmaster::LogLevel::ERROR);

// Can be enabled for troubleshooting
if (troubleshooting_mode) {
    logger.setGlobalLogLevel(huntmaster::LogLevel::DEBUG);
    logger.enableFileLogging("production_debug.log");
}
```

### Component-Specific Debugging

In production, enable debugging only for specific components:

```cpp
// Enable only audio engine debugging
logger.setComponentLogLevel(huntmaster::Component::AUDIO_ENGINE,
                           huntmaster::LogLevel::INFO);

// Keep other components at error level
logger.setComponentLogLevel(huntmaster::Component::MFCC_PROCESSOR,
                           huntmaster::LogLevel::ERROR);
```

## Integration Guidelines

### Library Integration

When integrating the Huntmaster Engine as a static library:

1. **Include Headers**: Include the debugging headers if needed

````cpp
#include <huntmaster/core/DebugLogger.h>
```cpp
// Initialize with appropriate level for your application
````

3. **Configure Components**: Set debug levels for components you're using

```cpp
auto& logger = huntmaster::DebugLogger::getInstance();
logger.setComponentLogLevel(huntmaster::Component::AUDIO_ENGINE,
                           huntmaster::LogLevel::INFO);
```

### Performance Considerations

The debugging infrastructure is designed for minimal performance impact:

- **Zero-cost when disabled**: No performance penalty when logging is disabled
- **Component-specific levels**: Only log what you need
- **Efficient formatting**: String formatting only occurs when logging is enabled
- **Thread-safe**: Safe for multi-threaded applications

## Deployment Checklist

### Pre-Deployment

- [ ] Build in Release mode for production
- [ ] Set appropriate global log level (ERROR or WARN for production)
- [ ] Configure component-specific log levels
- [ ] Test with debugging enabled and disabled
- [ ] Verify log file rotation if using file logging

### Tool Deployment

All 10 tools can be deployed with debugging capabilities:

```bash
# Package tools with debugging support
cp build/interactive_recorder /deployment/tools/
cp build/debug_dtw_similarity /deployment/tools/
cp build/test_mfcc_debugging /deployment/tools/
# ... other tools
```

### Documentation Deployment

Include debugging documentation:

```bash
cp DEBUGGING.md /deployment/docs/
cp README.md /deployment/docs/
```

```
[2025-07-15 10:30:45] [AUDIO_ENGINE] [INFO] Engine initialized successfully
[2025-07-15 10:30:45] [PERFORMANCE] [DEBUG] Audio processing completed in 1250μs
[2025-07-15 10:30:46] [MFCC_PROCESSOR] [TRACE] Extracted 13 MFCC coefficients
```

### Performance Monitoring

Performance metrics are logged when enabled:

```cpp
// Enable performance monitoring
logger.setComponentLogLevel(huntmaster::Component::PERFORMANCE,
                           huntmaster::LogLevel::DEBUG);
```

This provides timing information for:

- Audio processing operations
- Feature extraction
- Similarity analysis
- Real-time processing

## Platform-Specific Considerations

### Windows Deployment

- Ensure proper DLL dependencies if using dynamic linking
- Consider Windows-specific logging paths
- Test with Windows Audio Session API integration

### Linux Deployment

- Verify audio subsystem compatibility (ALSA, PulseAudio)
- Check file permissions for log files
- Consider systemd integration for service deployment

### macOS Deployment

- Test with Core Audio framework
- Ensure proper code signing for App Store distribution
- Consider sandboxing limitations for file logging

## Security Considerations

### Log Content

- Avoid logging sensitive audio data
- Implement log rotation to prevent disk space issues
- Consider log encryption for sensitive deployments

### Debug Access

- Restrict debug-enabled builds to authorized personnel
- Use environment variables or config files for debug settings
- Implement secure log file access controls

## Integration Testing

### Debug-Enabled Testing

Test your integration with debugging enabled:

```bash
# Test with various debug levels
./your_application --debug-level=INFO
./your_application --debug-level=DEBUG
./your_application --debug-level=TRACE

# Test component-specific debugging
./your_application --debug-component=AUDIO_ENGINE
./your_application --debug-component=MFCC_PROCESSOR
```

### Performance Testing

Measure performance impact of debugging:

```bash
# Benchmark with debugging disabled
./benchmark_tool --no-debug

# Benchmark with debugging enabled
./benchmark_tool --debug-level=INFO
./benchmark_tool --debug-level=DEBUG
```

## Maintenance

### Log Rotation

Implement log rotation for production deployments:

```cpp
// Rotate logs daily or by size
logger.enableFileLogging("huntmaster_" + getCurrentDate() + ".log");
```

### Debug Level Management

Provide runtime control over debug levels:

```cpp
// Configuration through environment variables
const char* debug_level = std::getenv("HUNTMASTER_DEBUG_LEVEL");
if (debug_level) {
    logger.setGlobalLogLevel(parseLogLevel(debug_level));
}
```

This deployment guide ensures the debugging infrastructure is properly configured and maintained in production environments while providing the flexibility needed for development and troubleshooting.
