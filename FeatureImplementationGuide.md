# Huntmaster Cross-Platform Architecture & Feature Implementation Guide

## Core Architecture Overview

### 1. **Shared C++ Audio Engine (Current)**

- **What**: Core audio processing library (already built)
- **Contains**: MFCC, DTW, Recording, Playback, Analysis
- **Deployment Strategy**:
  - **Web**: Compile to WebAssembly (WASM)
  - **Android**: JNI wrapper
  - **iOS**: Objective-C++ bridge

### 2. **Platform-Specific UI Layer**

- **Web**: React/Vue.js + Web Audio API
- **Android**: Kotlin/Jetpack Compose
- **iOS**: Swift/SwiftUI

---

## Feature Implementation Methodology

### **Feature 1: Real-Time Audio Level Monitoring**

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
2. **Process** → C++ engine (every 10ms)
3. **Display** → Platform UI (60 FPS)

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
└── Point: {x, yMin, yMax}
```

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
│   ├── computeSTFT(audio, windowSize, hopSize) -> Matrix
│   ├── magnitudeToDecibels(magnitude) -> Matrix
│   └── generateColorMap(dbMatrix) -> ImageData

Parameters:
├── Window: Hanning, 2048 samples
├── Hop: 512 samples (75% overlap)
└── Frequency bins: 0-8kHz (relevant for calls)
```

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
└── confidences[]
```

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
│   ├── updateScore(newAudioChunk) -> ScoreUpdate
│   ├── getDetailedScores() -> MultiScore
│   └── generateFeedback() -> Feedback

MultiScore:
├── overall: float
├── pitch: float
├── timing: float
├── volume: float
└── tonality: float
```

#### Feedback Generation:

```cpp
Feedback {
    type: enum {PITCH_HIGH, PITCH_LOW, TOO_FAST, TOO_SLOW, etc}
    severity: float (0-1)
    suggestion: string
    timestamp: float
}
```

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
├── AlignmentData: {masterTime[], userTime[], confidence[]}
├── DiffData: {timeDiff[], ampDiff[], freqDiff[]}
└── HeatmapData: {width, height, colorValues[]}
```

---

## Platform-Specific Implementation Details

### **Web Application**

#### Technology Stack:

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
     -s EXPORT_NAME="HuntmasterEngine" \
     -s EXPORTED_FUNCTIONS='["_processAudio","_getScore"]' \
     -o huntmaster.js *.cpp
```

---

### **Android Application**

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

    env->ReleaseFloatArrayElements(input, inputBuffer, 0);
    return output;
}
```

---

### **iOS Application**

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
// AudioEngineBridge.mm
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
    };
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

### **Performance Optimization**

#### Buffer Sizes:

- **Recording**: 512 samples (11.6ms @ 44.1kHz)
- **Analysis**: 2048 samples (46.4ms @ 44.1kHz)
- **Display**: 60 FPS (16.7ms update rate)

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
4. Save/load recordings

### **Phase 2: Visual Analysis**

1. Real-time waveform
2. Spectrogram display
3. Pitch tracking overlay
4. A/B comparison

### **Phase 3: Advanced Coaching**

1. Multi-dimensional scoring
2. Specific feedback generation
3. Practice mode with loops
4. Progress tracking

### **Phase 4: Social & Gamification**

1. Share recordings
2. Leaderboards
3. Achievements
4. Challenges

---

## Testing Strategy

### **Unit Tests** (C++)

```cpp
TEST(AudioEngine, ProcessChunk) {
    AudioEngine engine;
    float input[512] = {0};
    auto result = engine.processChunk(input, 512);
    EXPECT_EQ(result.size(), 512);
}
```

### **Platform Tests**

- **Web**: Jest + React Testing Library
- **Android**: JUnit + Espresso
- **iOS**: XCTest + XCUITest

### **Cross-Platform Validation**

Ensure identical results across platforms:

```
Input: test_audio.wav
Expected outputs:
├── MFCC: [checksum]
├── DTW Score: 0.854 ± 0.001
└── Pitch: [120.5, 121.0, ...] ± 0.5Hz
```
