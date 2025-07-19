# Huntmaster Feature Implementation Guide

## Mono vs Multi-Channel Support

**Current State:**

- The engine enforces mono-only input for all feature extraction and scoring routines. Multi-channel input is explicitly rejected and will cause tests to fail.
- All edge case and consistency tests validate mono-only behavior. Multi-channel support is a future stretch goal.

**Future Work:**

- Implement multi-channel support in the engine and update all relevant tests and documentation.

---

## MFCC Consistency and Robustness

The MFCC consistency and edge case tests ensure that feature extraction is:

- Stable across repeated runs (SineWaveConsistency, ComplexWaveformConsistency, RealAudioFileConsistency)
- Robust to silence and VAD trimming (AirGapUserAttempt)
- Robust to low-amplitude input (LowVolumeUserAttempt)

All tests expect mono input and will skip if required files are missing. Feature extraction must succeed for valid mono audio. Multi-channel input is not supported and will fail.

---

## Error Handling and Result Pattern

All engine APIs use a custom expected type and Result<T> pattern for robust error handling. Tests validate that errors (e.g., invalid input, missing files, multi-channel audio) are handled gracefully and do not crash the engine. Mono-only enforcement is strictly validated.

---

# Huntmaster Cross-Platform Architecture and Feature Implementation Guide

## Test Coverage & Next Steps

- All platform integration features validated by unit and integration tests:
  - Waveform generation, MFCC, DTW, VAD, session isolation
  - Error handling, debugging, JSON export, platform data integration

### Edge Case & Stress Testing Plan

1. **Edge Case Testing:**

   - Integration with minimum/maximum buffer sizes and config values
   - Multi-channel audio, range queries, large JSON exports

2. **Stress Testing:**

   - High-frequency API calls, rapid config changes
   - Simultaneous session integration under load

3. **Release Candidate Checklist:**
   - All integration tests pass on all platforms
   - Documentation up to date
   - Debugging outputs and error handling validated
   - No memory leaks or deadlocks
   - Performance benchmarks meet targets

## Core Architecture Overview

### 1. Shared C++ Audio Engine (Current)

- **What**: Core audio processing library (already built)
- **Contains**: MFCC, DTW, Recording, Playback, Analysis
- **Deployment Strategy**:
  - **Web**: Compile to WebAssembly (WASM)
  - **Android**: JNI wrapper
  - **iOS**: Objective-C++ bridge

### 2. Platform-Specific UI Layer

- **Web**: React/Vue.js + Web Audio API
- **Android**: Kotlin/Jetpack Compose
- **iOS**: Swift/SwiftUI

## Feature Implementation Methodology

### Feature 1: Real-Time Audio Level Monitoring

#### Technical Implementation:

```
C++ Core:
├── AudioLevelProcessor
│   ├── calculateRMS(buffer, size) -> float
│   ├── calculatePeak(buffer, size) -> float
│   └── getDecibels(level) -> float

Platform Integration:
├── Web: AudioWorkletProcessor (real-time audio thread)
├── Android: AudioRecord callback → JNI → C++
└── iOS: AVAudioEngine tap → C++ bridge
```

#### Data Flow:

1. **Capture** → Platform audio API

#### Implementation Steps:

- **Web**: Use AudioWorklet for low-latency processing
- **Android**: Use Oboe library for high-performance audio
- **iOS**: Use AVAudioEngine with installTap

---

### **Feature 2: Waveform Visualization**

#### Technical Implementation:

```
C++ Core:
├── WaveformGenerator
│   ├── downsample(audio, targetPoints) -> vector<float>
│   ├── generatePeaks(audio, resolution) -> PeakData
│   └── normalizeForDisplay(peaks) -> vector<Point>

Data Format:
├── PeakData: {min[], max[], rms[]}
#### Platform Rendering:
- **Web**: Canvas API or WebGL for performance
- **Android**: Custom View with Canvas
- **iOS**: CAShapeLayer or Metal for performance

---

### **Feature 3: Spectrogram Display**

#### Technical Implementation:

```

C++ Core:
├── SpectrogramProcessor
│ ├── computeSTFT(audio, windowSize, hopSize) -> Matrix
│ ├── magnitudeToDecibels(magnitude) -> Matrix
│ └── generateColorMap(dbMatrix) -> ImageData

Parameters:
├── Window: Hanning, 2048 samples
├── Hop: 512 samples (75% overlap)

#### Optimization Strategy:

- **Pre-compute**: FFT on C++ side
- **Transfer**: Only magnitude data (not complex)
- **Render**: GPU-accelerated on each platform

---

### **Feature 4: Pitch Detection & Tracking**

#### Technical Implementation:

```
C++ Core:
├── PitchTracker
│   ├── detectPitch(buffer) -> float (using YIN algorithm)
│   ├── smoothPitchCurve(pitches) -> vector<float>
│   └── detectPitchContour(audio) -> PitchCurve

PitchCurve:
├── timestamps[]
├── frequencies[]
#### Display Method:
- Overlay on waveform as line graph
- Color-code by confidence (green=high, red=low)
- Show target pitch range for specific call

---

### **Feature 5: Real-Time Similarity Scoring**

#### Technical Implementation:

```

C++ Core:
├── RealtimeScorer
│ ├── updateScore(newAudioChunk) -> ScoreUpdate
│ ├── getDetailedScores() -> MultiScore
│ └── generateFeedback() -> Feedback

MultiScore:
├── overall: float
├── pitch: float
├── timing: float
├── volume: float
└── tonality: float

````

#### Feedback Generation:

```cpp
Feedback {
    type: enum {PITCH_HIGH, PITCH_LOW, TOO_FAST, TOO_SLOW, etc}
    severity: float (0-1)
    timestamp: float
}
````

---

### **Feature 6: Audio Comparison Overlay**

#### Technical Implementation:

```
C++ Core:
├── AudioAligner
│   ├── alignAudio(master, user) -> AlignmentData
│   ├── computeDifference(aligned) -> DiffData
│   └── generateHeatmap(diff) -> HeatmapData

Visualization Data:
├── DiffData: {timeDiff[], ampDiff[], freqDiff[]}
└── HeatmapData: {width, height, colorValues[]}
```

---

## Platform-Specific Implementation Details

- **Frontend**: React + TypeScript
- **Audio**: Web Audio API + AudioWorklet
- **Visualization**: D3.js / Canvas / WebGL
- **C++ Integration**: Emscripten → WASM

#### Key Files:

```
web/
├── src/
│   ├── audio/
│   │   ├── AudioEngine.ts (WASM wrapper)
│   │   ├── AudioWorklet.js (real-time processing)
│   │   └── Visualizer.ts (rendering)
│   ├── components/
│   │   ├── WaveformDisplay.tsx
│   │   ├── SpectrogramDisplay.tsx
│   │   └── ScoreCard.tsx
│   └── wasm/
│       └── huntmaster.wasm (compiled C++)
```

#### Build Process:

```bash
# Compile C++ to WASM
emcc -O3 -s WASM=1 -s MODULARIZE=1 \
     -s EXPORTED_FUNCTIONS='["_processAudio","_getScore"]' \
     -o huntmaster.js *.cpp
```

---

#### Technology Stack:

- **UI**: Jetpack Compose
- **Audio**: Oboe (C++) + AudioRecord (fallback)
- **C++ Integration**: JNI + CMake

#### Key Files:

```
android/
├── app/src/main/
│   ├── java/com/huntmaster/
│   │   ├── audio/
│   │   │   ├── AudioEngine.kt (JNI wrapper)
│   │   │   └── AudioVisualizer.kt
│   │   └── ui/
│   │       ├── WaveformView.kt
│   │       └── SpectrogramView.kt
│   └── cpp/
│       ├── native-lib.cpp (JNI bridge)
│       └── CMakeLists.txt
```

#### JNI Bridge Example:

```cpp
extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_huntmaster_audio_AudioEngine_processAudioChunk(
    JNIEnv* env, jobject /* this */, jfloatArray input) {

    jfloat* inputBuffer = env->GetFloatArrayElements(input, nullptr);
    jsize length = env->GetArrayLength(input);

    // Process with C++ engine
    auto result = engine.processChunk(inputBuffer, length);

    // Return results
    jfloatArray output = env->NewFloatArray(result.size());
    env->SetFloatArrayRegion(output, 0, result.size(), result.data());

    return output;
}
```

---

#### Technology Stack:

- **UI**: SwiftUI
- **Audio**: AVAudioEngine + Core Audio
- **C++ Integration**: Objective-C++ bridge

#### Key Files:

```
ios/
├── Huntmaster/
│   ├── Audio/
│   │   ├── AudioEngineBridge.mm (Obj-C++)
│   │   ├── AudioProcessor.swift
│   │   └── Visualizer.swift
│   ├── Views/
│   │   ├── WaveformView.swift
│   │   ├── SpectrogramView.swift
│   │   └── ScoreView.swift
│   └── CPP/
│       └── HuntmasterEngine.xcframework
```

#### Objective-C++ Bridge:

```objc
@implementation AudioEngineBridge {
    std::unique_ptr<HuntmasterEngine> _engine;
}
- (float)processAudioBuffer:(float*)buffer length:(int)length {
    return _engine->processChunk(buffer, length);
}

- (NSDictionary*)getDetailedScores {
    auto scores = _engine->getMultiScore();
    return @{
        @"overall": @(scores.overall),
        @"pitch": @(scores.pitch),
        @"timing": @(scores.timing)
}
@end
```

---

## Data Synchronization Strategy

### **Shared Data Format**

```json
{
  "version": "1.0",
  "recording": {
    "id": "uuid",
    "timestamp": "ISO-8601",
    "duration": 2.5,
    "sampleRate": 44100
  },
  "analysis": {
    "mfcc": [[...], [...], ...],
    "pitch": [120.5, 121.0, ...],
    "amplitude": [0.5, 0.6, ...],
    "scores": {
      "overall": 0.85,
      "pitch": 0.90,
      "timing": 0.80
    }
  },
  "comparison": {
    "masterCallId": "buck_grunt",
    "alignment": [...],
    "feedback": [...]
  }
}
```

#### Buffer Sizes:

- **Recording**: 512 samples (11.6ms @ 44.1kHz)

#### Memory Management:

- **Ring buffers** for audio data
- **Object pools** for visualization data
- **Lazy loading** for master call library

---

## Progressive Feature Rollout

### **Phase 1: Core Features** (MVP)

1. Basic recording with level meter
2. Simple waveform display
3. Basic DTW scoring

### **Phase 2: Visual Analysis**

1. Real-time waveform
2. Spectrogram display
3. Pitch tracking overlay
4. A/B comparison

### **Phase 3: Advanced Coaching**

1. Multi-dimensional scoring
2. Specific feedback generation
3. Progress tracking

### **Phase 4: Social & Gamification**

2. Leaderboards
3. Achievements
4. Challenges

## Audio Input Requirements

- **Mono-only support:**

  - The engine currently supports only mono (single channel) audio input for all core features and tests.
  - If multi-channel audio is submitted, the engine will reject the input and return an error.
  - Multi-channel support (e.g., stereo, microphone arrays) is a stretch goal for future releases.
  - Future enhancements may include downmixing, per-channel analysis, or spatial feature extraction in the WaveformGenerator and related components.

---

## Testing Strategy

### **Unit Tests & MFCC Consistency**

The project uses GoogleTest-based C++ unit tests to validate all core features, including MFCC extraction, DTW scoring, and error handling. The MFCC consistency tests include:

- **Sine Wave Consistency:**

  - Generates a 440 Hz sine wave, saves as WAV, and processes it multiple times.
  - Verifies that MFCC and similarity scores are consistent across runs (max deviation < 0.0001).

- **Complex Waveform Consistency:**

  - Generates a composite waveform (220 Hz, 440 Hz, 880 Hz), saves as WAV, and processes in chunks.
  - Ensures scoring consistency across multiple runs.

- **Real Audio File Consistency:**

  - Loads a real wildlife call (e.g., buck_grunt.wav), processes in chunks, and checks score consistency.
  - Skips test if file is not available.

- **Self-Similarity Test:**
  - Compares a master call to itself (with added noise) to ensure high similarity score.
  - Score thresholds: >0.01 = excellent, >0.005 = good, >0.002 = fair.

All tests enforce mono-only input. Multi-channel audio is downmixed to mono for testing, or rejected by the engine if not supported.

### **Platform Tests**

- **Web**: Jest + React Testing Library
- **Android**: JUnit + Espresso
- **iOS**: XCTest + XCUITest

### **Cross-Platform Validation**

All core features and scoring algorithms are validated to produce consistent results across platforms (Web, Android, iOS). Example validation:

```
Input: test_audio.wav
Expected outputs:
├── MFCC: [checksum]
├── DTW Score: 0.854 ± 0.001
└── Pitch: [120.5, 121.0, ...] ± 0.5Hz
```
