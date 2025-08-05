# GitHub Copilot Instructions - Huntmaster Audio Engine

You are an AI programming assistant helping with the Huntmaster Audio Engine, a production-ready C++20 wildlife call analysis system.

## 🎯 Primary Development Guide

**ALWAYS FOLLOW**: The development plan is documented in `docs/mvp_todo.md` - this is the authoritative source for:
- Current project status and phase
- Implementation priorities and roadmap
- Enhanced platform features roadmap
- Success criteria and completion status

**DOCUMENTATION APPROACH**:
- Reference and update `docs/mvp_todo.md` for all major development decisions
- Only create supplementary documentation when it adds value beyond the MVP todo
- All status updates should be reflected in the MVP todo document first

## Project Context

This is a cross-platform audio processing engine that analyzes and compares animal calls using MFCC feature extraction and DTW pattern matching. The engine targets desktop, web (WebAssembly), and mobile platforms with a unified build system.

**Current Status**: MVP COMPLETED - Production Deployment Ready (August 3, 2025)
- Phase: Enhanced Platform Implementation (Advanced Features)
- Next: PitchTracker, HarmonicAnalyzer, CadenceAnalyzer implementation

### Core Architecture Principles

1. **Session-Based Design**: All operations require explicit `SessionId` for thread-safe isolation
2. **Result<T> Error Handling**: No exceptions - use custom Result<T> with `.isOk()` and `.value`
3. **Zero-Copy Audio**: Process audio using `std::span<const float>` to avoid allocations
4. **Factory Pattern**: Always use `::create()` methods, never direct constructors
5. **Timeout Protection**: Always use `timeout` command when executing tests or long-running operations

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

- `src/core/UnifiedAudioEngine.cpp` - Main orchestrator (95%+ complete)
- `src/core/MFCCProcessor.cpp` - Feature extraction (production-ready)
- `src/core/DTWComparator.cpp` - Pattern matching (production-ready)
- `src/core/VoiceActivityDetector.cpp` - Voice detection (operational)
- `src/core/AudioLevelProcessor.cpp` - Level monitoring (complete)
- `src/core/RealtimeScorer.cpp` - Multi-dimensional scoring (operational)

### Enhanced Platform Components (Next Implementation Phase)

**From Enhanced Roadmap in `docs/mvp_todo.md`**:
- `src/core/PitchTracker.cpp` - YIN algorithm pitch detection (planned)
- `src/core/HarmonicAnalyzer.cpp` - Tonal quality analysis (planned)
- `src/core/CadenceAnalyzer.cpp` - Rhythm pattern analysis (planned)
- `src/core/VolumeEnvelopeTracker.cpp` - Dynamic analysis (planned)
- `src/core/MasterCallAnalyzer.cpp` - Automated analysis (planned)

### Audio Processing Pipeline

```cpp
// Standard processing flow
engine->loadMasterCall(session, "deer_grunt");
engine->processAudioChunk(session, audioBuffer);
auto score = engine->getSimilarityScore(session);
```

## Development Phase Guidance

### Current Phase: Enhanced Platform Implementation

**Follow the enhanced roadmap in `docs/mvp_todo.md` for:**

1. **Advanced Analysis Components (Phase 2)**:
   - PitchTracker implementation with YIN algorithm
   - HarmonicAnalyzer for tonal quality assessment
   - CadenceAnalyzer for rhythm patterns
   - Multi-dimensional analysis features

2. **Learning & Gamification System (Phase 3)**:
   - Progress tracking and analytics
   - Achievement and gamification system
   - Interactive scenario engine
   - Personalized learning recommendations

3. **Cloud Integration & CMS (Phase 4)**:
   - Backend API and database design
   - User authentication and data sync
   - CMS integration for content management
   - Cross-platform synchronization

**Implementation Timeline**: Follow the 32-week development strategy outlined in MVP todo

## Alpha Testing Status

**Current State**: 97.5% Complete - Ready for Testing
- **Access**: `http://localhost:8080/alpha_test_refactored.html`
- **Features**: Real-time recording, similarity scoring, waveform visualization
- **Performance**: <50ms latency, 44.1kHz sampling, 30-90% similarity range

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

### Test Execution Best Practices

**CRITICAL: Always use timeouts when running test commands** to prevent hanging and blocking CI/CD pipelines.

#### Command Timeout Guidelines

- **Unit Tests**: 60 seconds timeout (quick execution expected)
- **Integration Tests**: 90 seconds timeout (moderate complexity)
- **Performance/Load Tests**: 120 seconds timeout (resource intensive)
- **WASM Tests**: 120 seconds timeout (compilation + execution overhead)
- **Security/Fuzz Tests**: 180 seconds timeout (extensive validation)

#### Terminal Command Patterns

```bash
# Unit test execution with timeout
timeout 60 ./bin/RunEngineTests --gtest_filter="ComponentTest.*"

# Integration test execution with timeout
timeout 90 ./bin/RunEngineTests --gtest_filter="IntegrationTest.*"

# Performance test execution with timeout
timeout 120 ./bin/performance_profiling_demo

# Coverage analysis with timeout
timeout 120 ./scripts/master_test_with_coverage.sh

# WASM build and test with timeout
timeout 120 ./scripts/build_wasm.sh && timeout 60 npm test

# Comprehensive test suite with timeout
timeout 180 ./scripts/master_test.sh
```

#### Test Script Template

```bash
#!/bin/bash
# Always include timeout protection in test scripts

TEST_TIMEOUT=${TEST_TIMEOUT:-60}  # Default 60 seconds
EXECUTABLE="./bin/RunEngineTests"
FILTER=${1:-"*"}

# Validate executable exists
if [[ ! -f "$EXECUTABLE" ]]; then
    echo "Error: Test executable not found: $EXECUTABLE"
    exit 1
fi

# Execute with timeout and proper error handling
if timeout "$TEST_TIMEOUT" "$EXECUTABLE" --gtest_filter="$FILTER" --gtest_brief=yes; then
    echo "✅ Tests completed successfully"
    exit 0
else
    exit_code=$?
    if [[ $exit_code -eq 124 ]]; then
        echo "❌ Tests timed out after ${TEST_TIMEOUT} seconds"
    else
        echo "❌ Tests failed with exit code: $exit_code"
    fi
    exit $exit_code
fi
```

#### CI/CD Integration

```yaml
# Example GitHub Actions step with timeout
- name: Run Unit Tests
  run: |
    timeout 60 ./bin/RunEngineTests --gtest_brief=yes
  timeout-minutes: 2 # Additional CI timeout layer

- name: Run Performance Tests
  run: |
    timeout 120 ./bin/performance_profiling_demo
  timeout-minutes: 3 # Additional CI timeout layer
```

#### Debugging Hung Tests

```bash
# For debugging test hangs, use strace/gdb with timeout
timeout 60 strace -e trace=all ./bin/RunEngineTests 2>&1 | head -100

# Memory debugging with timeout
timeout 90 valgrind --tool=memcheck --leak-check=full ./bin/RunEngineTests

# Background execution for investigation
timeout 120 ./bin/RunEngineTests &
TEST_PID=$!
sleep 30
if kill -0 $TEST_PID 2>/dev/null; then
    echo "Test still running, investigating..."
    ps aux | grep RunEngineTests
    kill -TERM $TEST_PID
fi
```

## Build Commands

### Development Workflow

```bash
# Standard build
cmake -B build && cmake --build build

# Debug build with logging
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build

# Run tests with timeout protection
timeout 60 ./build/bin/RunEngineTests

# Run specific test suite with timeout
timeout 60 ./build/bin/RunEngineTests --gtest_filter="AudioPlayerTest.*"

# WebAssembly build with timeout
timeout 120 ./scripts/build_wasm.sh
```

### Debugging Tools

```bash
# Interactive analysis with timeout
timeout 120 ./build/bin/interactive_recorder --debug --trace

# Component debugging with timeout
timeout 90 ./build/bin/simple_unified_test --engine-debug --mfcc-debug

# Performance profiling with timeout
timeout 120 ./build/bin/performance_profiling_demo

# Master test suite with timeout
timeout 180 ./scripts/master_test.sh
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

## WSL-Specific Development Notes

### Environment Setup

- Development occurs in WSL2 Ubuntu 24.04 on Windows 11
- Use Linux-specific paths and commands
- Audio testing may require PulseAudio configuration in WSL

### WSL Audio Configuration

# Enable audio in WSL (if needed)

sudo apt-get install pulseaudio
export PULSE_SERVER=tcp:$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}')

### File System Considerations

- Use `/home/username/projects/` for best performance
- Avoid `/mnt/c/` for build operations (slow I/O)
- Git line endings: Use LF (configure `.gitattributes`)

### Build Performance Tips

- Use `ccache` for faster rebuilds
- Consider WSL2 memory allocation in `.wslconfig`
- Use native Linux tools, not Windows versions

## Important Notes

### Current Project Status (August 3, 2025)

**From `docs/mvp_todo.md`**:
- ✅ **MVP Phase**: COMPLETED - Production deployment ready
- ✅ **Core Audio Engine**: 93.0% Complete → PRODUCTION READY
- ✅ **Security Framework**: 99.3% Complete → PRODUCTION DEPLOYMENT READY
- ✅ **Performance Framework**: Sub-real-time processing (0.275x ratio)
- ✅ **Alpha Testing**: 97.5% Complete with operational interface
- 🔄 **Enhanced Features**: Next implementation phase ready

### Development Priorities

**Current Focus**: Enhanced platform features implementation
1. **PitchTracker**: YIN algorithm for fundamental frequency detection
2. **HarmonicAnalyzer**: Spectral analysis for tonal quality assessment
3. **CadenceAnalyzer**: Temporal pattern analysis for rhythm detection
4. **Learning Platform**: Progress tracking and personalized feedback

### Current Limitations

- Enhanced analysis components (PitchTracker, HarmonicAnalyzer) not yet implemented
- Learning platform and gamification features in planning phase
- Cloud integration and CMS features planned for Phase 4
- Advanced UI components for configuration panel (5% remaining)

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

## 📋 Development Workflow

### Before Starting Any Work
1. **Check `docs/mvp_todo.md`** for current phase and priorities
2. **Update MVP todo** with progress and decisions
3. **Follow established roadmap** rather than creating new plans
4. **Reference Alpha Testing status** for integration requirements

### When Adding Features
1. **Validate against enhanced roadmap** in MVP todo
2. **Follow 32-week development timeline** priorities
3. **Update status in MVP todo** upon completion
4. **Integrate with Alpha Testing interface** when applicable

### Documentation Guidelines
- **Primary**: Update `docs/mvp_todo.md` for all major changes
- **Secondary**: Create supplementary docs only when they add unique value
- **Testing**: Reference Alpha Testing execution chain for validation
- **Status**: All project status updates go in MVP todo first
