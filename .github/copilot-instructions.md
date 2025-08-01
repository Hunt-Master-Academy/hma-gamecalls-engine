# GitHub Copilot Instructions - Huntmaster Audio Engine

You are an AI programming assistant helping with the Huntmaster Audio Engine, a production-ready C++20 wildlife call analysis system.

## Project Context

This is a cross-platform audio processing engine that analyzes and compares animal calls using MFCC feature extraction and DTW pattern matching. The engine targets desktop, web (WebAssembly), and mobile platforms with a unified build system.

### Core Architecture Principles

1. **Session-Based Design**: All operations require explicit `SessionId` for thread-safe isolation
2. **Result<T> Error Handling**: No exceptions - use custom Result<T> with `.isOk()` and `.value`
3. **Zero-Copy Audio**: Process audio using `std::span<const float>` to avoid allocations
4. **Factory Pattern**: Always use `::create()` methods, never direct constructors

## Code Style Guidelines

### Namespace Conventions

```cpp
// Primary namespace
namespace huntmaster {
    // Core components here
}

// Test files use
using namespace huntmaster;

// Note: VADConfig is in huntmaster::, NOT UnifiedAudioEngine::VADConfig
```

### Result<T> Pattern

```cpp
// CORRECT usage
auto result = engine->getSimilarityScore(sessionId);
if (result.isOk()) {
    float score = result.value;  // Direct member access
}

// INCORRECT - avoid these
float score = result.getValue();  // ❌ Method doesn't exist
float score = *result;            // ❌ Not a pointer type
```

### Session Management

```cpp
// Always follow this pattern
auto engineResult = UnifiedAudioEngine::create();
if (!engineResult.isOk()) return;
auto engine = std::move(engineResult.value);

auto sessionResult = engine->createSession(44100.0f);
if (!sessionResult.isOk()) return;
SessionId session = sessionResult.value;

// ... use session ...

engine->destroySession(session);  // Always cleanup
```

## Component Reference

### Core Components Location

- `src/core/UnifiedAudioEngine.cpp` - Main orchestrator
- `src/core/MFCCProcessor.cpp` - Feature extraction
- `src/core/DTWComparator.cpp` - Pattern matching
- `src/core/VoiceActivityDetector.cpp` - Voice detection
- `src/core/AudioLevelProcessor.cpp` - Level monitoring
- `src/core/RealtimeScorer.cpp` - Multi-dimensional scoring

### Audio Processing Pipeline

```cpp
// Standard processing flow
engine->loadMasterCall(session, "deer_grunt");
engine->processAudioChunk(session, audioBuffer);
auto score = engine->getSimilarityScore(session);
```

## Testing Patterns

### Test Structure

```cpp
class MyTest : public TestFixtureBase {
protected:
    void SetUp() override {
        auto engineResult = UnifiedAudioEngine::create();
        ASSERT_TRUE(engineResult.isOk());
        engine = std::move(engineResult.value);

        auto sessionResult = engine->createSession(44100.0f);
        ASSERT_TRUE(sessionResult.isOk());
        sessionId = sessionResult.value;
    }

    void TearDown() override {
        if (engine && sessionId != -1) {
            engine->destroySession(sessionId);
        }
    }

    std::unique_ptr<UnifiedAudioEngine> engine;
    SessionId sessionId = -1;
};
```

### Test Assertions

```cpp
// Always check Result<T> before accessing
auto result = engine->someMethod();
ASSERT_TRUE(result.isOk()) << "Failed with status: " << static_cast<int>(result.error());
auto value = result.value;

// For Status returns
auto status = engine->processAudioChunk(session, audio);
EXPECT_EQ(status, UnifiedAudioEngine::Status::OK);
```

## Build Commands

### Development Workflow

```bash
# Standard build
cmake -B build && cmake --build build

# Debug build with logging
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build

# Run tests
./build/bin/RunEngineTests

# WebAssembly build
./scripts/build_wasm.sh
```

### Debugging Tools

```bash
# Interactive analysis
./build/bin/interactive_recorder --debug --trace

# Component debugging
./build/bin/simple_unified_test --engine-debug --mfcc-debug

# Performance profiling
./build/bin/performance_profiling_demo
```

## Common Patterns

### Audio Buffer Processing

```cpp
// Generate test audio
std::vector<float> generateSineWave(float frequency, float duration, float sampleRate) {
    size_t numSamples = static_cast<size_t>(duration * sampleRate);
    std::vector<float> buffer(numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        buffer[i] = std::sin(2.0f * M_PI * frequency * i / sampleRate);
    }
    return buffer;
}

// Process with span
auto status = engine->processAudioChunk(session, std::span<const float>(buffer));
```

### Performance Monitoring

```cpp
// Use AutoProfiler for RAII timing
{
    AutoProfiler profiler("MFCCExtraction");
    // Timed operation here
}

// Memory tracking
MemoryGuard guard("AudioProcessing");
// Memory-intensive operation
```

### Configuration Patterns

```cpp
// VAD configuration (global namespace)
VADConfig vadConfig;
vadConfig.energyThreshold = 0.01f;
vadConfig.minSpeechDuration = 0.1f;
engine->setVADConfig(session, vadConfig);

// Realtime scorer config
RealtimeScorerConfig scorerConfig;
scorerConfig.mfccWeight = 0.6f;
scorerConfig.volumeWeight = 0.2f;
engine->setRealtimeScorerConfig(session, scorerConfig);
```

## WebAssembly Specifics

### Module Structure

- Core bindings: `web/src/modules/UnifiedAudioModule.js`
- Event system: `web/src/modules/EventManager.js`
- ES6 modular interface for browser integration

### WASM API Pattern

```javascript
// JavaScript usage
const engine = await HuntmasterEngine.create();
const session = await engine.createSession(44100);
await engine.loadMasterCall(session, "turkey_gobble");
```

## Important Notes

### Current Limitations

- `MemoryGuard` methods not fully implemented (causes linker errors)
- `StreamingAudioProcessor` API not yet available
- Some tests excluded in CMakeLists.txt due to incomplete APIs

### Performance Targets

- Real-time processing: < 10ms latency
- Memory usage: < 50MB per session
- CPU usage: < 20% on mobile devices

### Thread Safety

- Each session is thread-isolated
- No shared mutable state between sessions
- Lock-free audio processing paths

## Error Handling Best Practices

1. Always check `Result<T>` with `.isOk()` before accessing `.value`
2. Use factory methods (`::create()`) instead of constructors
3. Clean up sessions in reverse order of creation
4. Handle `Status` enums explicitly in switch statements
5. Log errors with component-specific debug flags

## File Organization

- `include/huntmaster/` - Public headers
- `src/core/` - Core implementation
- `src/platform/` - Platform-specific code
- `tests/unit/` - Unit tests
- `tests/lib/` - Test utilities
- `tools/` - CLI diagnostic tools
- `web/` - WebAssembly interface

Remember: This is a production audio engine where correctness and real-time performance are critical. Always follow the established patterns for session management, error handling, and memory efficiency.
