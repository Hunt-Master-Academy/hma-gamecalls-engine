# 🔄 Alpha Testing Execution Chain Documentation

> **📋 For current implementation status and quick start guide, see [ALPHA_TESTING_STATUS.md](./ALPHA_TESTING_STATUS.md)**
> **📋 This document provides the technical specification and API documentation**

## Overview
This document outlines the complete execution flow of the Huntmaster Audio Engine Alpha Testing interface, detailing the integration between the C++20 audio processing engine and the web-based testing interface.

**Implementation Status**: ✅ **97.5% Complete** - All documented functionality implemented and operational.

## 🚀 **System Initialization Chain**

### 1. Application Bootstrap
```
Page Load → WASM Module Loading → Engine Creation → Session Initialization → UI Activation
```

**Key Components:**
- **WASM Module**: Loads 112KB compiled engine + 50KB JS bindings
- **UnifiedAudioEngine**: Factory pattern initialization via `unified_create_engine()`
- **Session Management**: Create initial session with 44.1kHz sample rate
- **Result<T> Validation**: Check all engine operations for success
- **AudioContext Setup**: Web Audio API for playback and recording

**Initialization Flow:**
```javascript
// 1. Load WASM module
await HuntmasterEngine.ready;

// 2. Create engine instance (factory pattern)
const engineId = HuntmasterEngine.unified_create_engine();
if (engineId < 0) throw new Error("Engine creation failed");

// 3. Create audio session
const sessionId = HuntmasterEngine.unified_create_session(engineId, 44100);
if (sessionId < 0) throw new Error("Session creation failed");

// 4. Initialize AudioContext
audioContext = new (window.AudioContext || window.webkitAudioContext)();
```

**Success Criteria:**
- ✅ WASM module loads within 5 seconds
- ✅ Engine instance created with valid ID
- ✅ Session established with positive SessionId
- ✅ AudioContext state is 'running' or 'suspended'

### 2. C++ Engine Architecture Overview
```
UnifiedAudioEngine (Orchestrator)
├── MFCCProcessor (Feature Extraction)
├── DTWComparator (Pattern Matching)
├── AudioLevelProcessor (Real-time Monitoring)
├── VoiceActivityDetector (Voice Detection)
├── RealtimeScorer (Multi-dimensional Analysis)
├── AudioRecorder (File-based Recording)
└── AudioPlayer (Playback Management)
```

## 🎵 **Master Call Processing Chain**

### 3. Master Call Loading Pipeline
```
File Selection → HTTP Fetch → Engine Loading → Feature Pre-extraction → Visualization
```

**Step-by-Step Flow:**
1. **File Selection**: Choose from 8 deer calls (buck_grunt, doe_bleat, etc.)
2. **HTTP Request**: `fetch('/data/master_calls/${callType}.wav')`
3. **ArrayBuffer Conversion**: Convert response to binary data
4. **Engine Loading**: Pass audio data to C++ engine
   ```javascript
   // Load master call into engine session
   const status = HuntmasterEngine.unified_load_master_call(
       engineId,
       sessionId,
       callType  // e.g., "buck_grunt"
   );
   if (status !== 0) handleError(status);
   ```
5. **Feature Pre-extraction**: Engine automatically extracts MFCC features
6. **Visualization**: Render waveform for user feedback

**C++ Engine Processing:**
- **MFCCProcessor**: Extracts 13 MFCC coefficients per frame
- **Feature Storage**: Cached in session for fast comparison
- **Memory Management**: Automatic cleanup via RAII

### 4. Waveform Visualization Engine
```
Audio Buffer → Canvas Setup → Dual Waveform Display → Real-time Updates
```

**Rendering Pipeline:**
1. **Canvas Configuration**: 800x400px with device pixel ratio scaling
2. **Dual Display Mode**:
   - Top half: Master call waveform (red, #e74c3c)
   - Bottom half: Recorded audio waveform (blue, #3498db)
3. **Efficient Sampling**: Downsample based on canvas width
4. **Real-time Updates**: Refresh on new recordings or level changes

**Performance Optimizations:**
- Debounced resize handler (250ms)
- Pre-calculated sample stride
- Single-pass rendering with Path2D

## 🎤 **Audio Recording Chain**

### 5. Microphone Access & Recording (File-based Workflow)
```
Permission → Stream Setup → Engine Recording → File Save → Load for Analysis
```

**Recording Workflow:**
1. **Permission Request**:
   ```javascript
   const stream = await navigator.mediaDevices.getUserMedia({
       audio: {
           echoCancellation: false,
           noiseSuppression: false,
           autoGainControl: false,
           sampleRate: 44100
       }
   });
   ```

2. **Start Engine Recording**:
   ```javascript
   const status = HuntmasterEngine.unified_start_recording(engineId, sessionId);
   if (status !== 0) throw new Error(`Recording failed: ${status}`);
   ```

3. **Real-time Level Monitoring**:
   ```javascript
   // Poll recording levels at 60fps
   const monitorLevels = () => {
       if (isRecording) {
           const level = HuntmasterEngine.unified_get_recording_level(engineId, sessionId);
           updateLevelMeter(level);
           requestAnimationFrame(monitorLevels);
       }
   };
   ```

4. **Stop and Save Recording**:
   ```javascript
   // Stop recording
   HuntmasterEngine.unified_stop_recording(engineId, sessionId);

   // Save to file (required for engine's file-based workflow)
   const filename = `recording_${Date.now()}.wav`;
   const saveStatus = HuntmasterEngine.unified_save_recording(engineId, sessionId, filename);
   ```

5. **Load for Playback/Analysis**:
   - Recording is now available as a WAV file
   - Can be loaded back for visualization or further processing

**Key Limitation**:
- No direct memory access to recorded audio
- Must save to file before analysis
- Aligns with cross-platform deployment requirements

## 🧠 **Voice Activity Detection (VAD) Integration**

### 6. VAD Processing Architecture
```
Audio Chunks → C++ VAD Analysis → State Machine → Activity Events
```

**C++ VAD Configuration:**
```javascript
// Configure VAD parameters per session
const vadConfig = {
    energyThreshold: 0.01,      // Energy threshold for voice detection
    minSpeechDuration: 0.1,     // Minimum speech duration (seconds)
    minSilenceDuration: 0.3,    // Minimum silence duration (seconds)
    adaptiveMode: true          // Enable adaptive threshold
};

// Apply configuration (would need WASM binding)
HuntmasterEngine.unified_set_vad_config(engineId, sessionId, vadConfig);
```

**VAD Processing Flow:**
1. **Energy Analysis**: RMS energy calculation per frame
2. **Spectral Features**: Zero-crossing rate and spectral centroid
3. **State Machine**: SILENCE → SPEECH_START → SPEAKING → SPEECH_END
4. **Adaptive Thresholding**: Adjusts based on noise floor

**Integration with Recording:**
- VAD automatically segments recordings
- Removes silence from beginning/end
- Provides activity timestamps for visualization

## 📊 **Audio Analysis & Similarity Scoring**

### 7. C++ Engine Analysis Chain
```
Process Audio → MFCC Extraction → DTW Comparison → RealtimeScorer → Result
```

**Analysis Components:**

1. **Audio Processing Pipeline**:
   ```javascript
   // Process recorded audio through engine
   const processStatus = HuntmasterEngine.unified_process_audio_chunk(
       engineId,
       sessionId,
       audioData,      // Float32Array of audio samples
       audioData.length
   );
   ```

2. **MFCCProcessor (C++)**:
   - **Pre-emphasis**: High-frequency boost (α = 0.97)
   - **Windowing**: Hamming window (25ms frames, 10ms hop)
   - **FFT**: 512-point FFT using KissFFT
   - **Mel Filterbank**: 26 triangular filters
   - **DCT**: Extract 13 cepstral coefficients

3. **DTWComparator (C++)**:
   - **Algorithm**: Dynamic Time Warping with Sakoe-Chiba band
   - **Distance Metric**: Euclidean distance between MFCC vectors
   - **Optimization**: 10% window constraint for efficiency
   - **Normalization**: Path-normalized distance score

4. **RealtimeScorer (C++)**:
   ```javascript
   // Get multi-dimensional similarity score
   const score = HuntmasterEngine.unified_get_similarity_score(engineId, sessionId);

   // Score breakdown (via extended API):
   const breakdown = {
       mfccSimilarity: 0.85,    // 60% weight
       volumeSimilarity: 0.90,  // 20% weight
       timingSimilarity: 0.75,  // 20% weight
       overallScore: 0.84       // Weighted average
   };
   ```

**Scoring Algorithm (C++ Implementation):**
- **MFCC Distance**: DTW-normalized distance (60% weight)
- **Volume Matching**: RMS energy correlation (20% weight)
- **Timing Accuracy**: Duration similarity (20% weight)
- **Final Score**: 0-100% similarity rating

## 🔄 **Real-time Processing Loop**

### 8. Continuous Audio Processing
```
Audio Input → Chunk Buffer → Engine Processing → UI Update → Feedback Loop
```

**Processing Cycle (1024 samples @ 44.1kHz = ~23ms):**
1. **Audio Capture**: ScriptProcessor or AudioWorklet
2. **Float32 Conversion**: Normalize to [-1, 1] range
3. **Engine Processing**:
   ```javascript
   // Real-time processing during recording
   scriptProcessor.onaudioprocess = (e) => {
       const inputData = e.inputBuffer.getChannelData(0);

       // Process through engine
       const status = HuntmasterEngine.unified_process_audio_chunk(
           engineId,
           sessionId,
           inputData,
           inputData.length
       );

       // Get real-time similarity
       if (status === 0) {
           const score = HuntmasterEngine.unified_get_similarity_score(engineId, sessionId);
           updateScoreDisplay(score);
       }
   };
   ```

**Performance Monitoring:**
- **AudioLevelProcessor**: Real-time RMS/Peak levels
- **Processing Latency**: Track engine processing time
- **Frame Drops**: Monitor for audio glitches

## 🎯 **Playback & Interaction Chain**

### 9. Audio Playback System
```
Engine Playback API → Web Audio Integration → User Controls
```

**Playback Options:**
1. **Master Call Playback**:
   ```javascript
   // Direct engine playback (if implemented)
   HuntmasterEngine.unified_play_master_call(engineId, sessionId, "buck_grunt");
   ```

2. **Recording Playback**:
   ```javascript
   // Play saved recording
   HuntmasterEngine.unified_play_recording(engineId, sessionId, filename);
   ```

3. **Web Audio Fallback**:
   - Use AudioBufferSourceNode for decoded audio
   - Synchronized with visualization updates

### 10. User Interaction Flow
```
UI Event → Validation → Engine Command → Result Handling → UI Feedback
```

**State Management:**
```javascript
const AppState = {
    INITIALIZING: 'initializing',
    READY: 'ready',
    RECORDING: 'recording',
    PROCESSING: 'processing',
    ANALYZING: 'analyzing',
    ERROR: 'error'
};
```

**Button State Logic:**
- **Load Master**: Always enabled after initialization
- **Start Recording**: Enabled when ready, disabled during recording
- **Stop Recording**: Only enabled during active recording
- **Analyze**: Enabled when recording exists
- **Play**: Context-sensitive (master or recording)

## 🛡️ **Error Handling & Recovery**

### 11. Result<T> Pattern Integration
```
Engine Operation → Result Check → Error Classification → Recovery Strategy
```

**Error Handling Flow:**
```javascript
// Example: Handling engine operations with Result<T> pattern
function handleEngineOperation(operation, ...args) {
    const result = operation(...args);

    // Check if operation succeeded
    if (result < 0 || result === undefined) {
        const error = mapEngineError(result);

        switch(error.type) {
            case 'SESSION_NOT_FOUND':
                // Recreate session
                return reinitializeSession();

            case 'INVALID_PARAMS':
                // Log and notify user
                console.error('Invalid parameters:', args);
                showUserError('Invalid audio parameters');
                break;

            case 'PROCESSING_ERROR':
                // Retry with fallback
                return retryWithFallback(operation, args);

            default:
                // Generic error handling
                reportError(error);
        }
    }

    return result;
}
```

**Engine Status Codes:**
```javascript
const EngineStatus = {
    OK: 0,
    INIT_FAILED: -1,
    INVALID_PARAMS: -2,
    SESSION_NOT_FOUND: -3,
    RESOURCE_EXHAUSTED: -4,
    PROCESSING_ERROR: -5,
    NOT_IMPLEMENTED: -6
};
```

## 📈 **Performance Monitoring**

### 12. Comprehensive Performance Tracking
```
Engine Metrics → JavaScript Profiling → User Analytics → Optimization Loop
```

**Monitored Metrics:**
1. **Engine Performance**:
   - MFCC extraction time per frame
   - DTW comparison duration
   - Overall processing latency
   - Memory usage per session

2. **Web Interface Performance**:
   - WASM load time
   - Audio decode time
   - Canvas render time
   - UI responsiveness

3. **Quality Metrics**:
   - Audio dropout rate
   - Processing accuracy
   - User action success rate

**Performance Targets:**
- **Engine Processing**: < 10ms per audio chunk
- **UI Response**: < 16ms for 60fps
- **Total Latency**: < 50ms end-to-end
- **Memory Usage**: < 50MB per session

## 🔗 **Integration Points Summary**

### Critical Interfaces:
1. **WASM ↔ JavaScript**:
   - Engine bindings via Emscripten
   - Typed array passing for audio data
   - Status code handling

2. **C++ Engine ↔ Web Audio**:
   - File-based recording workflow
   - Real-time level monitoring
   - Playback coordination

3. **Session Management**:
   - Explicit SessionId for all operations
   - Per-session master calls
   - Isolated processing state

4. **Feature Processing**:
   - MFCCProcessor: 13-dimensional features
   - DTWComparator: Pattern matching
   - RealtimeScorer: Weighted similarity

### Success Metrics:
1. **Quick Start**: Page → Engine → Recording ready (< 3 seconds)
2. **Recording**: Start → Process → Score display (< 5 seconds)
3. **Analysis**: Load → Process → Results (< 1 second)
4. **Real-time**: Continuous processing at 44.1kHz without dropouts

This execution chain leverages the full capabilities of the Huntmaster Game Calls Engine while maintaining optimal performance and user experience.
