# Performance Profiling and Bottleneck Resolution Tools

This directory contains advanced performance profiling and optimization tools for the Huntmaster UnifiedAudioEngine. These tools provide comprehensive analysis of audio processing performance, bottleneck identification, and automated optimization suggestions.

## Overview

The performance profiling system consists of several key components:

- **PerformanceProfiler**: Core profiling framework for real-time performance monitoring
- **PerformanceBenchmark**: Comprehensive benchmarking suite for systematic testing
- **AutoProfiler**: RAII-style component profiling with automatic timing
- **Performance Analysis Tools**: Bottleneck detection and optimization recommendations

## Quick Start

### Building the Tools

```bash
# Build the entire project including performance tools
cmake -B build -DHUNTMASTER_BUILD_TOOLS=ON
cmake --build build

# The performance profiling demo will be built as: build/tools/performance_profiling_demo
```

### Running the Performance Profiling Demo

```bash
# Run the comprehensive demonstration
./build/tools/performance_profiling_demo

# This will generate several output files:
# - performance_report.json
# - comprehensive_benchmark_results.json
# - session_*_performance.json
```

## Core Components

### 1. PerformanceProfiler Class

The main profiling interface that provides:

- **Real-time Performance Monitoring**: Track processing times, memory usage, and quality metrics
- **Component-Level Analysis**: Profile individual components (MFCC, DTW, VAD, Scoring)
- **Alert System**: Automatic detection of performance issues with configurable thresholds
- **Bottleneck Analysis**: Identify primary performance bottlenecks and optimization opportunities

```cpp
#include "huntmaster/profiling/PerformanceProfiler.h"

// Create profiler with custom thresholds
PerformanceThresholds thresholds;
thresholds.maxRealTimeRatio = 0.5f; // 50% of real-time
thresholds.maxChunkLatency = 10.0f; // 10ms max per chunk
PerformanceProfiler profiler(thresholds);

// Start profiling a session
profiler.startProfiling(sessionId);

// Profile specific components
{
 PROFILE_COMPONENT(profiler, sessionId, "MFCC_Processing");
 // Your MFCC processing code here
}

// Get performance metrics
auto metrics = profiler.getSessionMetrics(sessionId);
auto alerts = profiler.checkPerformanceAlerts(sessionId);
```

### 2. PerformanceBenchmark Class

Comprehensive benchmarking suite for systematic performance evaluation:

```cpp
#include "huntmaster/profiling/PerformanceProfiler.h"

PerformanceBenchmark benchmark(engine.get());

// Configure benchmark parameters
PerformanceBenchmark::BenchmarkConfig config;
config.testDurations = {1, 5, 10, 30}; // seconds
config.chunkSizes = {256, 512, 1024}; // samples
config.sampleRates = {44100.0f}; // Hz

// Run comprehensive benchmark
auto results = benchmark.runComprehensiveBenchmark(config);

// Generate report
auto report = benchmark.generateBenchmarkReport(results);
std::cout << report << std::endl;
```

### 3. AutoProfiler (RAII Profiling)

Automatic component profiling using RAII pattern:

```cpp
void processAudioChunk(SessionId sessionId, std::span<const float> audio) {
 PROFILE_COMPONENT(profiler, sessionId, "AudioProcessing");

 // Your processing code is automatically profiled
 // Timing starts when entering scope, ends when leaving
 performMFCCExtraction(audio);
 performDTWComparison();
 performVADAnalysis();
}
```

## Performance Metrics

### Core Metrics Tracked

- **Real-time Ratio**: Processing time vs. audio duration (< 1.0 is good)
- **Chunk Processing Latency**: Time to process each audio chunk
- **Memory Usage**: Peak and average memory consumption
- **Component Timing**: Individual component performance breakdown
- **Quality Metrics**: Similarity scores and feature extraction counts

### Performance Categories

Results are automatically categorized based on performance:

- **Excellent**: Real-time ratio < 0.2 (5x faster than real-time)
- **Good**: Real-time ratio < 0.5 (2x faster than real-time)
- **Acceptable**: Real-time ratio < 1.0 (meets real-time requirements)
- **Poor**: Real-time ratio ≥ 1.0 (cannot keep up with real-time)

## Bottleneck Analysis

### Automatic Bottleneck Detection

The system automatically identifies performance bottlenecks by analyzing:

1. **Component Time Distribution**: Which components consume the most processing time
2. **Memory Usage Patterns**: Components with high memory allocation/deallocation
3. **Call Frequency Analysis**: Components called most frequently
4. **Quality vs. Performance Trade-offs**: Balance between processing time and output quality

### Optimization Suggestions

Based on bottleneck analysis, the system provides specific optimization recommendations:

#### MFCC Processing Optimizations

- Reduce frame size (512 → 256 samples)
- Decrease coefficient count (13 → 10 coefficients)
- Enable SIMD optimizations
- Use pre-computed window functions

#### DTW Comparison Optimizations

- Reduce window ratio (10% → 5%)
- Enable early termination for poor matches
- Use approximate distance calculations
- Implement parallel computation

#### VAD Processing Optimizations

- Use energy-only detection
- Reduce frame sizes
- Implement fast silence detection
- Batch process multiple frames

#### Memory Optimizations

- Implement buffer pooling
- Reduce feature vector caching
- Optimize memory access patterns
- Use circular buffers

## Advanced Features

### Real-time Performance Monitoring

Enable continuous performance monitoring with alerts:

```cpp
profiler.enableRealTimeMonitoring(true);
profiler.setCallbackOnAlert([](const PerformanceAlert& alert) {
 std::cout << "PERFORMANCE ALERT: " << alert.description << std::endl;
 for (const auto& suggestion : alert.suggestions) {
 std::cout << " Suggestion: " << suggestion << std::endl;
 }
});
```

### Automatic Optimization Application

Apply optimization suggestions automatically:

```cpp
// Get optimization suggestions
auto suggestions = profiler.suggestOptimizations(sessionId);

// Apply supported optimizations automatically
profiler.applyAutomaticOptimizations(sessionId, engine.get());
```

### Performance Trend Analysis

Analyze performance trends over time:

```cpp
// Analyze performance trends over last 10 minutes
auto trend = profiler.analyzePerformanceTrend(sessionId, std::chrono::minutes(10));

// Detect performance regression
bool hasRegression = profiler.detectPerformanceRegression(sessionId, 0.1f); // 10% threshold
```

### Export and Analysis

Export performance data for external analysis:

```cpp
// Export to JSON for programmatic analysis
profiler.exportToJson(sessionId, "performance_data.json");

// Export to CSV for spreadsheet analysis
profiler.exportToCsv(sessionId, "performance_data.csv");

// Generate detailed performance report
std::string report = profiler.generatePerformanceReport(sessionId);
```

## Integration Examples

### Basic Session Profiling

```cpp
#include "huntmaster/core/UnifiedAudioEngine.h"
#include "huntmaster/profiling/PerformanceProfiler.h"

// Create engine and profiler
auto engine = UnifiedAudioEngine::create();
PerformanceProfiler profiler;

// Create session
auto sessionResult = engine->createSession(44100.0f);
SessionId sessionId = *sessionResult;

// Start profiling
profiler.startProfiling(sessionId);

// Process audio with automatic profiling
std::vector<float> audioData = loadAudioFile("test.wav");
const size_t chunkSize = 512;

for (size_t i = 0; i < audioData.size(); i += chunkSize) {
 auto chunk = std::span<const float>(audioData.data() + i,
 std::min(chunkSize, audioData.size() - i));

 auto startTime = std::chrono::high_resolution_clock::now();
 auto status = engine->processAudioChunk(sessionId, chunk);
 auto endTime = std::chrono::high_resolution_clock::now();

 // Record performance metrics
 auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
 profiler.recordChunkProcessed(sessionId, chunk.size(), duration.count() / 1000.0f);
}

// Get results
profiler.stopProfiling(sessionId);
auto metrics = profiler.getSessionMetrics(sessionId);
auto alerts = profiler.checkPerformanceAlerts(sessionId);

// Generate report
std::cout << profiler.generatePerformanceReport(sessionId) << std::endl;
```

### Multi-Session Monitoring

```cpp
// Monitor multiple concurrent sessions
std::vector<SessionId> sessions;
PerformanceProfiler profiler;

// Create multiple sessions
for (int i = 0; i < 5; ++i) {
 auto sessionResult = engine->createSession(44100.0f);
 SessionId sessionId = *sessionResult;
 sessions.push_back(sessionId);
 profiler.startProfiling(sessionId);
}

// Process audio concurrently
for (SessionId sessionId : sessions) {
 std::thread([&profiler, &engine, sessionId]() {
 // Process audio in parallel
 auto audioData = generateTestAudio();
 processAudioWithProfiling(profiler, engine.get(), sessionId, audioData);
 }).detach();
}

// Get system overview
auto systemOverview = profiler.getSystemOverview();
std::cout << "Active Sessions: " << systemOverview.totalActiveSessions << std::endl;
std::cout << "System Load: " << systemOverview.avgSystemLoad << std::endl;
```

## Performance Optimization Workflow

### 1. Establish Baseline

```bash
# Run comprehensive benchmark to establish baseline
./build/tools/performance_profiling_demo
```

### 2. Identify Bottlenecks

```cpp
// Analyze specific session for bottlenecks
auto analysis = profiler.analyzeBottlenecks(sessionId);
std::cout << "Primary Bottleneck: " << analysis.bottleneckComponent << std::endl;
std::cout << "Impact: " << analysis.impactPercentage << "%" << std::endl;
```

### 3. Apply Optimizations

```cpp
// Get specific optimization suggestions
auto suggestions = profiler.suggestOptimizations(sessionId);

// Apply optimizations manually or automatically
profiler.applyAutomaticOptimizations(sessionId, engine.get());
```

### 4. Validate Improvements

```cpp
// Compare performance before and after optimization
PerformanceBenchmark benchmark(engine.get());
auto newResults = benchmark.runComprehensiveBenchmark();
bool improved = benchmark.compareWithBaseline(newResults, "baseline.json");
```

## Configuration Options

### Performance Thresholds

Customize performance thresholds for your specific requirements:

```cpp
PerformanceThresholds thresholds;
thresholds.maxRealTimeRatio = 0.3f; // Must process 3x faster than real-time
thresholds.maxChunkLatency = 5.0f; // 5ms maximum per chunk
thresholds.maxMemoryUsage = 50 * 1024 * 1024; // 50MB maximum
thresholds.minSimilarityScore = 0.01f; // Minimum quality threshold
thresholds.maxCpuUsage = 20.0f; // 20% CPU maximum
```

### Benchmark Configuration

Customize benchmark parameters:

```cpp
PerformanceBenchmark::BenchmarkConfig config;
config.testDurations = {1, 5, 10, 30, 60}; // Test durations in seconds
config.chunkSizes = {128, 256, 512, 1024, 2048}; // Chunk sizes to test
config.sampleRates = {22050.0f, 44100.0f, 48000.0f}; // Sample rates
config.numRuns = 10; // Number of runs for averaging
config.enableMemoryProfiling = true; // Enable memory analysis
config.enableLatencyProfiling = true; // Enable latency analysis
config.enableQualityValidation = true; // Enable quality validation
```

## Output Files

The profiling tools generate several output files for analysis:

### performance_report.json

```json
{
 "sessionId": 12345,
 "realTimeRatio": 0.25,
 "avgChunkProcessingTime": 2.5,
 "peakMemoryUsage": 52428800,
 "totalChunksProcessed": 1000,
 "avgSimilarityScore": 0.75,
 "components": {
 "mfcc": {
 "totalTime": 15000,
 "avgTime": 15,
 "callCount": 1000
 },
 "dtw": {
 "totalTime": 45000,
 "avgTime": 45,
 "callCount": 1000
 }
 }
}
```

### comprehensive_benchmark_results.json

```json
[
 {
 "testName": "RealTime_5s_512_44100",
 "performanceCategory": "Good",
 "realTimeRatio": 0.35,
 "avgProcessingTime": 1750.5,
 "peakMemoryUsage": 45678912,
 "passedRealTimeThreshold": true,
 "avgSimilarityScore": 0.82
 }
]
```

## Troubleshooting

### Common Issues

1. **High Memory Usage**

 - Check for memory leaks in session management
 - Implement buffer pooling
 - Reduce feature vector caching

2. **High Latency**

 - Optimize MFCC frame sizes
 - Reduce DTW window ratios
 - Enable SIMD optimizations

3. **Poor Quality Scores**
 - Check master call loading
 - Validate audio preprocessing
 - Verify feature extraction parameters

### Debug Mode

Enable detailed debug output:

```cpp
// Enable comprehensive logging
profiler.enableRealTimeMonitoring(true);
profiler.setCallbackOnAlert([](const PerformanceAlert& alert) {
 // Log all performance alerts
 std::cout << "[" << alert.timestamp.time_since_epoch().count() << "] "
 << alert.description << std::endl;
});
```

## Best Practices

1. **Profile Early and Often**: Integrate profiling into development workflow
2. **Use Realistic Data**: Test with actual wildlife call recordings
3. **Monitor Continuously**: Enable real-time monitoring in production
4. **Validate Optimizations**: Always measure impact of optimizations
5. **Consider Trade-offs**: Balance performance vs. quality requirements
6. **Document Baselines**: Keep baseline performance measurements for comparison

## API Reference

For detailed API documentation, see:

- `include/huntmaster/profiling/PerformanceProfiler.h`
- Generated Doxygen documentation
- Unit tests in `tests/unit/test_performance_profiler.cpp`

## Contributing

When adding new performance metrics or optimizations:

1. Update the `ComponentMetrics` structure
2. Add corresponding recording methods
3. Update optimization suggestion algorithms
4. Add test cases for new functionality
5. Update this documentation
