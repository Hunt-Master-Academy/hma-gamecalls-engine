# GameCalls Engine - Microservices Architecture Guide

**Last Updated:** October 19, 2025  
**Status:** Planning & Design  
**Reference Implementation:** `hma-hunt-strategy-engine/backend`

---

## Executive Summary

This document outlines the microservices architecture for the **GameCalls Engine**, enabling it to serve both:
1. **Hunt Master Academy (HMA)** - Educational platform with courses and lessons
2. **Hunt Master Field Guide (HMFG)** - Mobile-first field application

The approach mirrors the successful **Hunt Strategy Engine** architecture, which wraps Flutter/Dart domain logic with a Node.js/Express REST API layer.

---

## Architecture Overview

### Current State (C++ Core Engine)
```
┌─────────────────────────────────────────┐
│     GameCalls Engine (C++20)            │
│  ┌──────────────────────────────────┐   │
│  │ UnifiedAudioEngine               │   │
│  │  - Session Management            │   │
│  │  - MFCC Processing               │   │
│  │  - Similarity Scoring (DTW)      │   │
│  │  - Enhanced Analyzers            │   │
│  │    (Pitch, Harmonic, Cadence)    │   │
│  └──────────────────────────────────┘   │
│                                          │
│  In-Process API (C++ headers)           │
│  - No network interface                 │
│  - Native bindings only                 │
└─────────────────────────────────────────┘
```

### Target Microservices Architecture
```
┌──────────────────────────────────────────────────────────────┐
│                     CLIENT LAYER                             │
├──────────────────────────────────────────────────────────────┤
│  HMA Academy Web        │       HMFG Mobile Apps             │
│  (React + Vite)         │  (React Native / Flutter)          │
└────────────┬────────────┴────────────┬──────────────────────┘
             │                         │
             ▼                         ▼
┌──────────────────────────────────────────────────────────────┐
│              HMA API GATEWAY (Port 3000)                     │
│         (Express.js - Auth, Rate Limiting, Routing)          │
└────────────┬─────────────────────────────────────────────────┘
             │
             ▼
┌──────────────────────────────────────────────────────────────┐
│         GAMECALLS ENGINE REST API (Port 5005)                │
│              (Node.js + Express.js)                          │
│  ┌────────────────────────────────────────────────┐          │
│  │  REST Endpoints:                               │          │
│  │  - POST /gamecalls/sessions                    │          │
│  │  - POST /gamecalls/sessions/:id/analyze        │          │
│  │  - GET  /gamecalls/sessions/:id/results        │          │
│  │  - POST /gamecalls/sessions/:id/finalize       │          │
│  │  - GET  /gamecalls/masters                     │          │
│  └────────────────────────────────────────────────┘          │
│                         │                                     │
│                         ▼                                     │
│  ┌────────────────────────────────────────────────┐          │
│  │    C++ Core Engine (Native Addon)              │          │
│  │    - UnifiedAudioEngine                        │          │
│  │    - MFCC, DTW, Analyzers                      │          │
│  │    - Performance-critical processing           │          │
│  └────────────────────────────────────────────────┘          │
└──────────────────────────────────────────────────────────────┘
```

---

## Learning from Hunt Strategy Engine

### Key Architectural Patterns

#### 1. **Backend REST API Wrapper** (`hma-hunt-strategy-engine/backend/`)
- **Technology Stack**: Node.js + Express.js
- **Port**: 8000 (Hunt Strategy), **5005 (GameCalls - from architecture spec)**
- **Purpose**: Wrap domain logic with REST endpoints
- **Structure**:
  ```
  backend/
  ├── src/
  │   ├── index.js              # Express app setup
  │   ├── routes/
  │   │   ├── strategy.js       # Main router
  │   │   ├── predictions.js    # Prediction endpoints
  │   │   ├── waypoints.js      # Waypoint CRUD
  │   │   ├── plans.js          # Hunt plan management
  │   │   └── validations.js    # Input validation
  │   ├── controllers/          # Business logic
  │   ├── services/             # Domain services
  │   ├── repositories/         # Data persistence
  │   ├── middleware/           # Auth, logging, errors
  │   └── utils/                # Helpers
  ├── docs/
  │   └── API_CONTRACT.md       # OpenAPI/REST spec
  ├── package.json
  └── Dockerfile.dev
  ```

#### 2. **Dual-Mode Operation**
Hunt Strategy Engine supports:
- **Deterministic Mode**: Seeded, reproducible results for HMA Academy lessons
- **Production Mode**: Real-world data (Google Earth Engine integration)

**GameCalls Equivalent**:
- **Educational Mode**: Pre-recorded master calls, consistent grading for lessons
- **Field Mode**: Real-time analysis with adaptive feedback

#### 3. **Service Integration Pattern**
From `hma-academy-brain/src/config/index.ts`:
```typescript
engines: {
  gameCalls: {
    url: process.env.GAME_CALLS_ENGINE_URL || 'http://localhost:5005',
    apiKey: process.env.GAME_CALLS_ENGINE_API_KEY || 'game-calls-api-key',
    timeout: parseInt(process.env.GAME_CALLS_ENGINE_TIMEOUT || '30000', 10)
  }
}
```

#### 4. **Docker-First Deployment**
From `hma-infra/docker/docker-compose.yml`:
```yaml
hma-gamecalls-engine:
  build: ../../hma-gamecalls-engine/backend
  ports:
    - "5005:5005"
  environment:
    - NODE_ENV=development
    - PORT=5005
    - POSTGRES_HOST=postgres
    - REDIS_HOST=redis
  depends_on:
    - postgres
    - redis
  volumes:
    - ../../hma-gamecalls-engine/backend:/app
    - /app/node_modules
```

---

## Proposed GameCalls REST API Design

### Core Endpoints

#### Session Management

**POST /gamecalls/sessions**
```json
// Request
{
  "mode": "educational" | "field",
  "masterId": "whitetail-grunt-01",
  "sampleRate": 44100,
  "userId": "user123"  // Optional, for HMA Academy tracking
}

// Response (201 Created)
{
  "sessionId": "sess_abc123",
  "status": "initialized",
  "master": {
    "id": "whitetail-grunt-01",
    "name": "Adult Buck Grunt",
    "species": "whitetail-deer",
    "duration": 2.5,
    "difficulty": "intermediate"
  },
  "config": {
    "sampleRate": 44100,
    "enhancedAnalyzers": true,
    "mode": "educational"
  },
  "createdAt": "2025-10-19T14:30:00.000Z"
}
```

**DELETE /gamecalls/sessions/:sessionId**
```json
// Response (204 No Content)
```

---

#### Real-Time Analysis

**POST /gamecalls/sessions/:sessionId/analyze**
```json
// Request
{
  "audioChunk": "base64_encoded_pcm_data",
  "chunkIndex": 0,
  "timestamp": "2025-10-19T14:30:01.500Z"
}

// Response (200 OK)
{
  "sessionId": "sess_abc123",
  "chunkIndex": 0,
  "analysis": {
    "similarity": {
      "score": 0.72,
      "confidence": 0.85,
      "reliable": true,
      "framesObserved": 120,
      "minFramesRequired": 100
    },
    "pitch": {
      "frequency": 245.5,
      "confidence": 0.78,
      "stability": "good"
    },
    "harmonic": {
      "richness": 0.65,
      "clarity": 0.72
    },
    "cadence": {
      "tempo": 120,
      "consistency": 0.82
    },
    "loudness": {
      "rms": -12.5,
      "peak": -6.2,
      "deviation": 0.15
    }
  },
  "timestamp": "2025-10-19T14:30:01.520Z"
}
```

---

#### Session Finalization

**POST /gamecalls/sessions/:sessionId/finalize**
```json
// Request (empty body)
{}

// Response (200 OK)
{
  "sessionId": "sess_abc123",
  "status": "finalized",
  "finalMetrics": {
    "overallSimilarity": 0.78,
    "similarityAtFinalize": 0.82,  // Refined DTW on best segment
    "pitchGrade": "B",
    "harmonicGrade": "B+",
    "cadenceGrade": "A-",
    "loudnessDeviation": 0.12,
    "normalizationScalar": 0.95,
    "segment": {
      "startMs": 500,
      "durationMs": 1200,
      "quality": "good"
    }
  },
  "coaching": [
    {
      "category": "pitch",
      "severity": "info",
      "message": "Good pitch control. Minor stability improvement at onset.",
      "actionable": "Focus on smooth initiation of the call."
    },
    {
      "category": "cadence",
      "severity": "success",
      "message": "Excellent rhythm consistency.",
      "actionable": null
    }
  ],
  "visualizations": {
    "overlayDataUrl": "/gamecalls/sessions/sess_abc123/overlay",
    "spectrogramUrl": "/gamecalls/sessions/sess_abc123/spectrogram"
  },
  "finalizedAt": "2025-10-19T14:30:05.000Z"
}
```

---

#### Results Retrieval

**GET /gamecalls/sessions/:sessionId/results**
```json
// Response (200 OK)
{
  "sessionId": "sess_abc123",
  "status": "finalized",
  "master": { /* master call info */ },
  "metrics": { /* finalMetrics from finalize */ },
  "history": [
    {
      "timestamp": "2025-10-19T14:30:01.500Z",
      "similarity": 0.65,
      "pitch": 240.2
    },
    // ... time series data
  ],
  "coaching": [ /* feedback array */ ]
}
```

**GET /gamecalls/sessions/:sessionId/overlay**
```json
// Response (200 OK)
{
  "sessionId": "sess_abc123",
  "decimation": 100,
  "master": {
    "peaks": [0.2, 0.5, 0.8, /* ... */],
    "duration": 2500  // ms
  },
  "user": {
    "peaks": [0.1, 0.4, 0.7, /* ... */],
    "duration": 2600  // ms
  },
  "alignment": {
    "offsetMs": 50,
    "stretchFactor": 1.04
  }
}
```

---

#### Master Call Library

**GET /gamecalls/masters**
```json
// Query params: ?species=whitetail-deer&difficulty=intermediate&limit=20

// Response (200 OK)
{
  "masters": [
    {
      "id": "whitetail-grunt-01",
      "name": "Adult Buck Grunt",
      "species": "whitetail-deer",
      "callType": "grunt",
      "difficulty": "intermediate",
      "duration": 2.5,
      "sampleRate": 44100,
      "description": "Common vocalization used by mature bucks...",
      "tags": ["rut", "dominance", "contact"],
      "audioUrl": "/gamecalls/masters/whitetail-grunt-01/audio",
      "waveformUrl": "/gamecalls/masters/whitetail-grunt-01/waveform"
    }
    // ...
  ],
  "total": 45,
  "page": 1,
  "limit": 20
}
```

**GET /gamecalls/masters/:masterId**
```json
// Response (200 OK)
{
  "id": "whitetail-grunt-01",
  "name": "Adult Buck Grunt",
  "species": "whitetail-deer",
  "callType": "grunt",
  "difficulty": "intermediate",
  "duration": 2.5,
  "sampleRate": 44100,
  "format": "wav",
  "channels": 1,
  "bitDepth": 16,
  "description": "Common vocalization used by mature bucks during the rut season...",
  "usage": {
    "timing": ["pre-rut", "rut"],
    "distance": "close-to-medium",
    "windConditions": "light-to-moderate"
  },
  "tags": ["rut", "dominance", "contact"],
  "audioUrl": "/gamecalls/masters/whitetail-grunt-01/audio",
  "waveformUrl": "/gamecalls/masters/whitetail-grunt-01/waveform",
  "createdAt": "2025-06-15T10:00:00.000Z"
}
```

---

### HMA Academy Integration Endpoints

These endpoints provide lesson/course-specific functionality.

**POST /gamecalls/lessons/:lessonId/session**
```json
// Request
{
  "userId": "user123",
  "attemptNumber": 1
}

// Response (201 Created)
{
  "sessionId": "sess_lesson_abc123",
  "lessonId": "lesson_gamecalls_101",
  "lesson": {
    "title": "Basic Buck Grunt",
    "objectives": [
      "Achieve 75% similarity score",
      "Maintain pitch stability",
      "Match cadence timing"
    ],
    "passingGrade": 75,
    "masterId": "whitetail-grunt-01"
  },
  "previousAttempts": 0,
  "bestScore": null
}
```

**POST /gamecalls/lessons/:lessonId/complete**
```json
// Request
{
  "sessionId": "sess_lesson_abc123",
  "userId": "user123"
}

// Response (200 OK)
{
  "lessonId": "lesson_gamecalls_101",
  "completed": true,
  "passed": true,
  "finalScore": 82,
  "passingGrade": 75,
  "metrics": {
    "similarity": 82,
    "pitchGrade": "B+",
    "harmonicGrade": "A-",
    "cadenceGrade": "A"
  },
  "achievements": [
    {
      "id": "first_passing_call",
      "name": "Call Master Apprentice",
      "description": "Pass your first game call lesson",
      "earnedAt": "2025-10-19T14:30:05.000Z"
    }
  ],
  "nextLesson": {
    "id": "lesson_gamecalls_102",
    "title": "Advanced Buck Grunt Variations"
  }
}
```

---

## Implementation Roadmap

### Phase 1: Backend REST API Foundation (Week 1-2)

**Milestone 1.1: Project Setup**
- [ ] Create `hma-gamecalls-engine/backend/` directory structure
- [ ] Initialize Node.js project (`package.json`)
- [ ] Install dependencies:
  - `express` - REST API framework
  - `cors` - Cross-origin resource sharing
  - `helmet` - Security headers
  - `compression` - Response compression
  - `winston` - Logging
  - `joi` - Input validation
  - `redis` - Session caching
- [ ] Create Dockerfile.dev
- [ ] Configure ESLint and Prettier

**Milestone 1.2: Basic Express Server**
- [ ] Create `src/index.js` with Express app
- [ ] Configure middleware (CORS, helmet, compression, JSON parsing)
- [ ] Add health check endpoint: `GET /health`
- [ ] Implement error handling middleware
- [ ] Add request logging middleware
- [ ] Test: Server starts on port 5005

**Milestone 1.3: C++ Bindings POC**
- [ ] Research Node.js native addon approaches:
  - **Option A**: Node-API (N-API) - Recommended (stable ABI)
  - **Option B**: node-addon-api (C++ wrapper for N-API)
  - **Option C**: Emscripten/WebAssembly (future-proof)
- [ ] Create `bindings/` directory
- [ ] Implement proof-of-concept binding:
  - Wrap `UnifiedAudioEngine::create()`
  - Expose `createSession()` to JavaScript
  - Test session creation from Node.js
- [ ] Document binding architecture in `docs/bindings.md`

---

### Phase 2: Core Session Management (Week 3-4)

**Milestone 2.1: Session Routes**
- [ ] Create `src/routes/sessions.js`
- [ ] Implement `POST /gamecalls/sessions`
  - Validate request (mode, masterId, sampleRate)
  - Call C++ `createSession()` via bindings
  - Store session metadata in Redis
  - Return session ID
- [ ] Implement `DELETE /gamecalls/sessions/:sessionId`
  - Call C++ `destroySession()`
  - Clear Redis cache
  - Return 204 No Content
- [ ] Add session validation middleware
- [ ] Write integration tests

**Milestone 2.2: Master Call Management**
- [ ] Create `src/routes/masters.js`
- [ ] Implement `GET /gamecalls/masters`
  - Query master call database (PostgreSQL)
  - Filter by species, difficulty, tags
  - Paginate results
- [ ] Implement `GET /gamecalls/masters/:masterId`
  - Return master call metadata
  - Include audio/waveform URLs
- [ ] Create `src/services/masterCallService.js`
- [ ] Implement master call loading in C++:
  - `loadMasterCall(sessionId, masterId)` binding
  - File I/O and validation

**Milestone 2.3: Redis Session Store**
- [ ] Create `src/repositories/sessionRepository.js`
- [ ] Implement session persistence:
  - Store session metadata (user, mode, timestamps)
  - Cache C++ engine state reference
  - TTL: 1 hour (configurable)
- [ ] Add session retrieval by ID
- [ ] Add session cleanup (expired sessions)

---

### Phase 3: Real-Time Analysis API (Week 5-6)

**Milestone 3.1: Analysis Endpoint**
- [ ] Create `src/routes/analysis.js`
- [ ] Implement `POST /gamecalls/sessions/:sessionId/analyze`
  - Accept base64-encoded audio chunk
  - Decode to PCM float array
  - Call C++ `processAudioChunk()` via binding
  - Retrieve realtime metrics:
    - `getRealtimeSimilarityState()`
    - `getEnhancedSummary()`
  - Format response JSON
  - Return 200 with metrics
- [ ] Add chunk size validation
- [ ] Add audio format validation
- [ ] Performance target: < 50ms processing time

**Milestone 3.2: Streaming Optimization**
- [ ] Implement binary audio streaming (alternative to base64)
- [ ] Add WebSocket support for lower latency (optional)
- [ ] Implement chunk buffering in Redis (for replay/debug)
- [ ] Add rate limiting per session

**Milestone 3.3: C++ Binding - Processing**
- [ ] Wrap `processAudioChunk(sessionId, audioData)`
- [ ] Wrap `getRealtimeSimilarityState(sessionId)`
- [ ] Wrap `getEnhancedSummary(sessionId)`
- [ ] Handle C++ Result<T> to Node.js Promise conversion
- [ ] Error mapping (C++ Status codes → HTTP status codes)

---

### Phase 4: Finalization & Results (Week 7-8)

**Milestone 4.1: Finalize Endpoint**
- [ ] Create `src/routes/finalize.js`
- [ ] Implement `POST /gamecalls/sessions/:sessionId/finalize`
  - Call C++ `finalizeSessionAnalysis()`
  - Retrieve final metrics
  - Generate coaching feedback (rule-based)
  - Store results in PostgreSQL
  - Return comprehensive results
- [ ] Add finalization idempotency (can call multiple times)
- [ ] Performance target: < 100ms finalization

**Milestone 4.2: Coaching Feedback System**
- [ ] Create `src/services/coachingService.js`
- [ ] Implement rule-based feedback generation:
  - Pitch analysis → pitch tips
  - Cadence analysis → timing tips
  - Harmonic analysis → tone tips
  - Loudness analysis → volume tips
- [ ] Map grades (A-F) to actionable feedback
- [ ] Format coaching messages (severity, category, action)
- [ ] Add feedback templates for common issues

**Milestone 4.3: Results Endpoint**
- [ ] Implement `GET /gamecalls/sessions/:sessionId/results`
  - Retrieve from PostgreSQL if finalized
  - Retrieve from Redis cache if in-progress
  - Include time-series history
  - Include coaching feedback
- [ ] Implement `GET /gamecalls/sessions/:sessionId/overlay`
  - Call C++ `exportOverlayData()`
  - Format for frontend waveform visualization
  - Include alignment metadata
- [ ] Add results pagination (for long sessions)

---

### Phase 5: HMA Academy Integration (Week 9-10)

**Milestone 5.1: Lesson-Specific Endpoints**
- [ ] Create `src/routes/lessons.js`
- [ ] Implement `POST /gamecalls/lessons/:lessonId/session`
  - Load lesson configuration from Academy Brain
  - Validate user enrollment
  - Create session with lesson context
  - Return lesson objectives
- [ ] Implement `POST /gamecalls/lessons/:lessonId/complete`
  - Validate passing criteria
  - Update user progress (call to Academy Brain)
  - Award achievements
  - Return completion status

**Milestone 5.2: Academy Brain Integration**
- [ ] Create `src/services/academyIntegrationService.js`
- [ ] Implement API client for `hma-academy-brain`:
  - `POST /api/lessons/:lessonId/progress` - Update progress
  - `POST /api/achievements/award` - Award achievements
  - `GET /api/users/:userId/profile` - User context
- [ ] Add JWT authentication for Academy requests
- [ ] Add retry logic and error handling

**Milestone 5.3: Dual-Mode Support**
- [ ] Implement educational mode:
  - Use seeded master calls
  - Consistent grading (deterministic)
  - Lesson-specific constraints
- [ ] Implement field mode:
  - Real-world analysis
  - Adaptive thresholds
  - No lesson constraints
- [ ] Add mode switching in configuration
- [ ] Document mode differences

---

### Phase 6: Docker & Infrastructure (Week 11-12)

**Milestone 6.1: Docker Configuration**
- [ ] Create `backend/Dockerfile.dev`
  - Multi-stage build (build C++ bindings, then Node.js app)
  - Install C++ build tools (CMake, GCC/Clang)
  - Copy and build C++ core
  - Install Node.js dependencies
  - Expose port 5005
- [ ] Create `backend/.dockerignore`
- [ ] Test Docker build locally

**Milestone 6.2: Docker Compose Integration**
- [ ] Add service to `hma-infra/docker/docker-compose.yml`:
  ```yaml
  hma-gamecalls-engine:
    build: ../../hma-gamecalls-engine/backend
    ports:
      - "5005:5005"
    environment:
      - NODE_ENV=development
      - PORT=5005
      - POSTGRES_HOST=postgres
      - REDIS_HOST=redis
      - ACADEMY_BRAIN_URL=http://hma-academy-brain:3001
    depends_on:
      - postgres
      - redis
      - hma-academy-brain
    volumes:
      - ../../hma-gamecalls-engine/backend:/app
      - /app/node_modules
      - ../../hma-gamecalls-engine/build:/app/cpp_build
  ```
- [ ] Test full stack startup
- [ ] Verify service connectivity

**Milestone 6.3: Database Schema**
- [ ] Create migration: `backend/migrations/001_gamecalls_schema.sql`
  - `gamecalls_masters` table
  - `gamecalls_sessions` table
  - `gamecalls_results` table
  - Indexes and constraints
- [ ] Implement migration runner in `src/db/migrate.js`
- [ ] Document schema in `docs/database.md`

---

### Phase 7: Testing & Documentation (Week 13-14)

**Milestone 7.1: API Testing**
- [ ] Set up Jest test framework
- [ ] Write unit tests for controllers
- [ ] Write integration tests for endpoints:
  - Session creation/destruction
  - Real-time analysis
  - Finalization
  - Master call retrieval
- [ ] Write C++ binding tests
- [ ] Add test coverage reporting (target: 80%)

**Milestone 7.2: Load Testing**
- [ ] Install Artillery or k6
- [ ] Create load test scenarios:
  - Concurrent session creation
  - Streaming analysis under load
  - Finalization performance
- [ ] Performance targets:
  - Session creation: < 100ms P95
  - Analysis chunk: < 50ms P95
  - Finalization: < 200ms P95
- [ ] Document results in `docs/performance.md`

**Milestone 7.3: API Documentation**
- [ ] Create OpenAPI 3.0 specification
- [ ] Document all endpoints with examples
- [ ] Add Swagger UI for interactive docs
- [ ] Create Postman collection
- [ ] Write integration guide for frontend developers

---

## Technical Considerations

### C++ to Node.js Binding Strategy

**Recommended Approach: Node-API (N-API)**

Advantages:
- ✅ Stable ABI across Node.js versions
- ✅ No recompilation needed for Node.js upgrades
- ✅ Official Node.js support
- ✅ C++ wrapper available (`node-addon-api`)

**Binding Architecture**:
```cpp
// bindings/gamecalls_addon.cpp
#include <napi.h>
#include "huntmaster/core/UnifiedAudioEngine.h"

// Session handle management
std::unordered_map<std::string, SessionId> sessions;
std::mutex sessionsMutex;

// Create session wrapper
Napi::Value CreateSession(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // Extract parameters
    float sampleRate = info[0].As<Napi::Number>().FloatValue();
    
    // Call C++ engine
    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        Napi::Error::New(env, "Failed to create engine")
            .ThrowAsJavaScriptException();
        return env.Null();
    }
    
    auto sessionResult = engineResult.value->createSession(sampleRate);
    if (!sessionResult.isOk()) {
        Napi::Error::New(env, "Failed to create session")
            .ThrowAsJavaScriptException();
        return env.Null();
    }
    
    // Store session
    std::string sessionId = generateSessionId();
    {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        sessions[sessionId] = sessionResult.value;
    }
    
    return Napi::String::New(env, sessionId);
}

// Initialize addon
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("createSession", Napi::Function::New(env, CreateSession));
    exports.Set("processChunk", Napi::Function::New(env, ProcessChunk));
    exports.Set("finalize", Napi::Function::New(env, Finalize));
    return exports;
}

NODE_API_MODULE(gamecalls_addon, Init)
```

**JavaScript Usage**:
```javascript
// src/services/engineService.js
const addon = require('../../build/Release/gamecalls_addon.node');

class EngineService {
    createSession(sampleRate) {
        return new Promise((resolve, reject) => {
            try {
                const sessionId = addon.createSession(sampleRate);
                resolve(sessionId);
            } catch (error) {
                reject(new Error(`Engine error: ${error.message}`));
            }
        });
    }
    
    async processChunk(sessionId, audioData) {
        return new Promise((resolve, reject) => {
            try {
                const result = addon.processChunk(sessionId, audioData);
                resolve(result);
            } catch (error) {
                reject(new Error(`Processing error: ${error.message}`));
            }
        });
    }
}

module.exports = new EngineService();
```

---

### Audio Data Formats

**Chunk Transmission Options**:

1. **Base64 Encoded (Simple)**
   - ✅ Easy to implement
   - ✅ Works with JSON
   - ❌ 33% size overhead
   - ❌ Encoding/decoding CPU cost

2. **Binary WebSocket (Efficient)**
   - ✅ No encoding overhead
   - ✅ Lower latency
   - ❌ More complex implementation
   - ✅ Recommended for HMFG field app

3. **Multipart Form Data (Hybrid)**
   - ✅ Good for large files
   - ✅ Standard HTTP
   - ❌ Not suitable for streaming

**Recommendation**: 
- HMA Academy (lessons): Base64 JSON (simpler integration)
- HMFG (field): Binary WebSocket (performance-critical)

---

### Session State Management

**Redis Schema**:
```javascript
// Session metadata (TTL: 1 hour)
{
  "session:sess_abc123": {
    "userId": "user123",
    "masterId": "whitetail-grunt-01",
    "mode": "educational",
    "createdAt": "2025-10-19T14:30:00.000Z",
    "status": "active",
    "chunksProcessed": 45,
    "lastActivityAt": "2025-10-19T14:32:15.000Z"
  }
}

// Session analysis cache (TTL: 5 minutes)
{
  "analysis:sess_abc123:latest": {
    "similarity": 0.72,
    "pitch": { /* ... */ },
    "harmonic": { /* ... */ },
    "cadence": { /* ... */ },
    "timestamp": "2025-10-19T14:32:15.000Z"
  }
}

// Session history (TTL: 1 hour, list)
{
  "history:sess_abc123": [
    { "t": 1000, "similarity": 0.65, /* ... */ },
    { "t": 2000, "similarity": 0.70, /* ... */ }
  ]
}
```

**PostgreSQL Schema** (persistent results):
```sql
CREATE TABLE gamecalls_sessions (
    session_id VARCHAR(64) PRIMARY KEY,
    user_id VARCHAR(64),
    master_id VARCHAR(64) NOT NULL,
    lesson_id VARCHAR(64),
    mode VARCHAR(20) NOT NULL CHECK (mode IN ('educational', 'field')),
    status VARCHAR(20) NOT NULL CHECK (status IN ('active', 'finalized', 'expired')),
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    finalized_at TIMESTAMPTZ,
    total_duration_ms INTEGER,
    chunks_processed INTEGER DEFAULT 0
);

CREATE TABLE gamecalls_results (
    result_id SERIAL PRIMARY KEY,
    session_id VARCHAR(64) REFERENCES gamecalls_sessions(session_id),
    overall_similarity DECIMAL(5,2),
    similarity_at_finalize DECIMAL(5,2),
    pitch_grade CHAR(2),
    harmonic_grade CHAR(2),
    cadence_grade CHAR(2),
    loudness_deviation DECIMAL(5,2),
    normalization_scalar DECIMAL(5,3),
    segment_start_ms INTEGER,
    segment_duration_ms INTEGER,
    coaching_feedback JSONB,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE gamecalls_masters (
    master_id VARCHAR(64) PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    species VARCHAR(64) NOT NULL,
    call_type VARCHAR(64) NOT NULL,
    difficulty VARCHAR(20) CHECK (difficulty IN ('beginner', 'intermediate', 'advanced')),
    duration_sec DECIMAL(5,2),
    sample_rate INTEGER,
    audio_file_path TEXT NOT NULL,
    waveform_data JSONB,
    description TEXT,
    tags TEXT[],
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_sessions_user ON gamecalls_sessions(user_id);
CREATE INDEX idx_sessions_lesson ON gamecalls_sessions(lesson_id);
CREATE INDEX idx_results_session ON gamecalls_results(session_id);
CREATE INDEX idx_masters_species ON gamecalls_masters(species);
```

---

### Error Handling Strategy

**HTTP Status Code Mapping**:

| C++ Status | HTTP Status | Example Scenario |
|------------|-------------|------------------|
| `OK` | 200 OK | Successful analysis |
| `INVALID_SESSION` | 404 Not Found | Session doesn't exist |
| `BAD_CONFIG` | 400 Bad Request | Invalid parameters |
| `NOT_READY` | 409 Conflict | Analysis not ready yet |
| `INSUFFICIENT_DATA` | 400 Bad Request | Not enough audio data |
| `UNSUPPORTED` | 501 Not Implemented | Feature not available |
| `IO_ERROR` | 500 Internal Server Error | File read/write failure |

**Error Response Format**:
```json
{
  "error": {
    "code": "INSUFFICIENT_DATA",
    "message": "Not enough audio frames for reliable similarity calculation",
    "details": {
      "framesObserved": 50,
      "minFramesRequired": 100
    },
    "timestamp": "2025-10-19T14:30:05.000Z",
    "requestId": "req_xyz789"
  }
}
```

---

## Integration Examples

### HMA Academy Web (React)

```typescript
// src/services/gameCallsApi.ts
import axios from 'axios';

const API_BASE = process.env.VITE_GAMECALLS_API_URL || 'http://localhost:5005';

class GameCallsAPI {
    async createSession(masterId: string, mode: 'educational' | 'field') {
        const response = await axios.post(`${API_BASE}/gamecalls/sessions`, {
            mode,
            masterId,
            sampleRate: 44100
        });
        return response.data;
    }

    async analyzeChunk(sessionId: string, audioChunk: Float32Array) {
        // Convert Float32Array to base64
        const base64 = this.audioToBase64(audioChunk);
        
        const response = await axios.post(
            `${API_BASE}/gamecalls/sessions/${sessionId}/analyze`,
            {
                audioChunk: base64,
                chunkIndex: this.chunkCounter++,
                timestamp: new Date().toISOString()
            }
        );
        return response.data;
    }

    async finalizeSession(sessionId: string) {
        const response = await axios.post(
            `${API_BASE}/gamecalls/sessions/${sessionId}/finalize`
        );
        return response.data;
    }

    private audioToBase64(audio: Float32Array): string {
        // Convert to 16-bit PCM, then base64
        const int16Array = new Int16Array(audio.length);
        for (let i = 0; i < audio.length; i++) {
            int16Array[i] = Math.max(-32768, Math.min(32767, audio[i] * 32768));
        }
        const buffer = Buffer.from(int16Array.buffer);
        return buffer.toString('base64');
    }
}

export default new GameCallsAPI();
```

---

### HMFG Mobile (React Native)

```typescript
// src/services/gameCallsService.ts
import { NativeModules } from 'react-native';

// Native module for C++ engine (direct integration)
const { GameCallsEngine } = NativeModules;

// REST API client (fallback or complementary)
import axios from 'axios';

class GameCallsService {
    private useNativeEngine: boolean = true;  // Prefer native for performance
    
    async createSession(masterId: string): Promise<string> {
        if (this.useNativeEngine) {
            // Direct C++ integration (no REST overhead)
            return await GameCallsEngine.createSession(masterId, 44100);
        } else {
            // REST API fallback
            const response = await axios.post(
                `${API_BASE}/gamecalls/sessions`,
                { masterId, mode: 'field', sampleRate: 44100 }
            );
            return response.data.sessionId;
        }
    }
    
    async analyzeChunk(sessionId: string, audioData: Float32Array) {
        if (this.useNativeEngine) {
            // Process locally (zero network latency)
            return await GameCallsEngine.processChunk(sessionId, audioData);
        } else {
            // Send to backend
            const base64 = this.audioToBase64(audioData);
            const response = await axios.post(
                `${API_BASE}/gamecalls/sessions/${sessionId}/analyze`,
                { audioChunk: base64 }
            );
            return response.data.analysis;
        }
    }
}

export default new GameCallsService();
```

**Note**: HMFG will likely use **direct native bindings** for real-time performance, but the REST API provides:
1. Backup/fallback mechanism
2. Server-side processing for less powerful devices
3. Unified analytics and logging
4. Consistent lesson integration with HMA Academy

---

## Deployment Architecture

### Development Environment
```
Developer Laptop
├── HMA Academy Web (Vite dev server, port 3004)
├── HMA Academy Brain (Node.js, port 3001)
├── API Gateway (Express, port 3000)
├── GameCalls Engine REST API (Node.js, port 5005)
│   └── C++ Core (native addon)
├── PostgreSQL (port 5432)
└── Redis (port 6379)
```

### Docker Compose (Full Stack)
```bash
cd /home/xbyooki/projects/hma-infra/docker
docker-compose up -d

# Services:
# - hma-academy-web: http://localhost:3004
# - hma-academy-brain: http://localhost:3001
# - hma-academy-api: http://localhost:3000
# - hma-gamecalls-engine: http://localhost:5005
# - postgres: localhost:5432
# - redis: localhost:6379
# - minio: http://localhost:9000
```

### Production (AWS - Future)
```
┌─────────────────────────────────────────┐
│      CloudFront CDN                     │
│      (Static Assets)                    │
└────────────┬────────────────────────────┘
             │
┌────────────▼────────────────────────────┐
│   Application Load Balancer             │
│   (SSL Termination, Routing)            │
└────┬───────────────────────┬────────────┘
     │                       │
     ▼                       ▼
┌─────────────┐      ┌──────────────────┐
│  ECS Fargate│      │  ECS Fargate     │
│  Academy    │      │  GameCalls API   │
│  Brain      │      │  (Multi-container)│
└─────────────┘      └──────────────────┘
                            │
                     ┌──────┴──────┐
                     │ C++ Engine  │
                     │ Sidecar     │
                     └─────────────┘
```

---

## Success Criteria

### Phase 1 Success (Backend Foundation)
- ✅ Node.js Express server running on port 5005
- ✅ Health check endpoint responds
- ✅ C++ binding POC successful (can create session from JS)
- ✅ Docker container builds successfully

### Phase 2 Success (Session Management)
- ✅ Can create/destroy sessions via REST API
- ✅ Session metadata stored in Redis
- ✅ Master call library queryable via API
- ✅ Master calls loadable into C++ engine

### Phase 3 Success (Real-Time Analysis)
- ✅ Audio chunks processable via REST API
- ✅ Real-time metrics returned within 50ms
- ✅ Similarity, pitch, harmonic, cadence data accurate
- ✅ Multiple concurrent sessions supported (load test)

### Phase 4 Success (Finalization)
- ✅ Session finalization completes within 200ms
- ✅ Coaching feedback generated correctly
- ✅ Results persisted to PostgreSQL
- ✅ Overlay data exportable for visualization

### Phase 5 Success (Academy Integration)
- ✅ Lesson-specific sessions work end-to-end
- ✅ User progress updates sent to Academy Brain
- ✅ Achievements awarded correctly
- ✅ Lesson completion flow validated

### Phase 6 Success (Infrastructure)
- ✅ Full stack runs via `docker-compose up`
- ✅ All services communicate correctly
- ✅ Database migrations run automatically
- ✅ Redis caching functional

### Phase 7 Success (Production Ready)
- ✅ 80%+ test coverage
- ✅ Load tests pass performance targets
- ✅ API documentation complete (OpenAPI spec)
- ✅ Frontend integration guide published
- ✅ Monitoring and logging configured

---

## Next Steps

1. **Review this document** with the development team
2. **Validate C++ binding strategy** - POC the Node-API approach
3. **Define sprint boundaries** - Break phases into 2-week sprints
4. **Assign ownership** - Who owns backend, bindings, integration?
5. **Set up project board** - Track milestones in GitHub Projects
6. **Create initial PR** - Backend directory structure + package.json
7. **Schedule architecture review** - Align with Academy Brain integration

---

## References

### Hunt Strategy Engine (Reference Implementation)
- Backend: `/home/xbyooki/projects/hma-hunt-strategy-engine/backend/`
- API Contract: `/home/xbyooki/projects/hma-hunt-strategy-engine/backend/docs/API_CONTRACT.md`
- Express Server: `/home/xbyooki/projects/hma-hunt-strategy-engine/backend/src/index.js`

### GameCalls Engine (Current State)
- Core Engine: `/home/xbyooki/projects/hma-gamecalls-engine/src/`
- Architecture Doc: `/home/xbyooki/projects/hma-gamecalls-engine/docs/architecture.md`
- Public API: `/home/xbyooki/projects/hma-gamecalls-engine/include/huntmaster/core/UnifiedAudioEngine.h`

### System Documentation
- Architecture: `/home/xbyooki/projects/hma-docs/architecture/system-verification.md`
- Integration Map: `/home/xbyooki/projects/hma-academy-api/docs/system-connectivity-map.md`

### Node.js Native Addons
- Node-API Docs: https://nodejs.org/api/n-api.html
- node-addon-api: https://github.com/nodejs/node-addon-api
- Examples: https://github.com/nodejs/node-addon-examples

---

**Document Status**: ✅ Complete  
**Next Review**: After Phase 1 completion  
**Maintainer**: AI/ML Team + Backend Team
