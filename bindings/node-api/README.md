# [20251028-BINDINGS-047] Node-API Bindings README
# GameCalls Engine Native Bindings for Node.js

Node-API (N-API) native addon providing JavaScript/TypeScript bindings for the Hunt Master Academy GameCalls Engine (C++20).

## Features

- **Session Management**: Create, manage, and destroy audio analysis sessions
- **Realtime Processing**: Process audio buffers with low-latency (<12ms)
- **Enhanced Analysis**: Pitch tracking, harmonic analysis, cadence detection
- **Similarity Scoring**: DTW-based similarity comparison with master calls
- **Thread-Safe**: Multiple concurrent sessions supported
- **Promise-Based API**: Modern async/await JavaScript interface

## Installation

```bash
# Install dependencies
npm install

# Build native addon
npm run build

# Run tests
npm test
```

## Prerequisites

- Node.js 18+ (LTS recommended)
- CMake 3.20+
- C++20 compatible compiler (GCC 11+, Clang 14+, MSVC 2022+)
- GameCalls Engine C++ library built in `../../build/`

## Quick Start

```javascript
const gameCallsEngine = require('@hma/gamecalls-engine-bindings');

async function analyzeCall() {
    // Initialize engine
    await gameCallsEngine.initialize();
    
    // Create session with master call
    const sessionId = await gameCallsEngine.createSession(
        '/path/to/master_call.wav',
        { sampleRate: 44100, enableEnhancedAnalysis: true }
    );
    
    // Process audio buffer (from microphone, file, etc.)
    const audioBuffer = new Float32Array(4096);
    // ... fill buffer with audio samples ...
    
    const results = await gameCallsEngine.processAudio(sessionId, audioBuffer);
    console.log('Similarity Score:', results.similarityScore);
    console.log('Pitch:', results.pitch.pitch, 'Hz');
    console.log('Readiness:', results.readiness);
    
    // Finalize analysis
    const finalAnalysis = await gameCallsEngine.finalizeSession(sessionId);
    console.log('Overall Score:', finalAnalysis.overallScore);
    console.log('Best Segment:', finalAnalysis.segment);
    
    // Cleanup
    await gameCallsEngine.destroySession(sessionId);
}
```

## API Reference

See [lib/index.d.ts](lib/index.d.ts) for complete TypeScript definitions.

### Core Methods

#### `initialize(): Promise<boolean>`
Initialize the engine (call once at startup).

#### `createSession(masterCallPath, options): Promise<number>`
Create new audio analysis session.

**Parameters:**
- `masterCallPath` (string): Path to master call audio file
- `options` (object):
  - `sampleRate` (number): Sample rate in Hz (default: 44100)
  - `enableEnhancedAnalysis` (boolean): Enable pitch/harmonic/cadence analyzers (default: true)

**Returns:** Session ID (number)

#### `processAudio(sessionId, audioBuffer): Promise<AnalysisResults>`
Process audio buffer for realtime analysis.

**Parameters:**
- `sessionId` (number): Session identifier
- `audioBuffer` (Float32Array): Audio samples

**Returns:** Analysis results with similarity, pitch, harmonic, cadence metrics

#### `finalizeSession(sessionId): Promise<FinalAnalysis>`
Finalize session analysis (segment selection, refined DTW).

**Returns:** Complete analysis with segment info and final scores

#### `destroySession(sessionId): Promise<boolean>`
Destroy session and free resources.

## Building from Source

```bash
# Clean build
npm run clean

# Debug build (for development)
npm run build:debug

# Release build (optimized)
npm run build
```

## Architecture

```
JavaScript API (lib/index.js)
        ↓
Node-API Addon (src/gamecalls_addon.cc)
        ↓
SessionWrapper (src/session_wrapper.cc)
AudioProcessor (src/audio_processor.cc)
TypeConverters (src/type_converters.cc)
        ↓
UnifiedAudioEngine (C++ core)
```

## Performance

- **Session Creation**: ~50-100ms (includes master call MFCC cache)
- **Audio Processing**: <12ms per 4096-sample buffer @ 44.1kHz
- **Finalization**: <40ms for typical 3-5 second user call

## Troubleshooting

### Build Errors

**"Cannot find huntmaster_core library"**
```bash
# Build C++ engine first
cd ../../
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j8
```

**"node-gyp: not found"**
```bash
npm install -g node-gyp
```

### Runtime Errors

**"Session not found"**
- Ensure session ID is valid
- Check if session was destroyed

**"Master call file not found"**
- Verify file path exists
- Check file permissions

## Integration with Backend

```javascript
// backend/src/services/sessionsService.js
const gamecallsEngine = require('@hma/gamecalls-engine-bindings');

class SessionsService {
    static async createSession(masterCallId, options) {
        // Load master call from MinIO
        const masterCallPath = await minio.getPresignedUrl(masterCallId);
        
        // Create C++ engine session
        const sessionId = await gamecallsEngine.createSession(
            masterCallPath, 
            options
        );
        
        // Cache session state in Redis
        await redis.set(`session:${sessionId}`, JSON.stringify({
            masterCallId,
            createdAt: Date.now()
        }), 'EX', 3600);
        
        return { sessionId };
    }
}
```

## License

MIT - Hunt Master Academy

## Support

- GitHub Issues: https://github.com/Hunt-Master-Academy/hma-gamecalls-engine/issues
- Documentation: https://docs.huntmasteracademy.com/gamecalls-engine
