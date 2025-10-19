# GameCalls Engine - Microservices Architecture Diagrams

**Last Updated:** October 19, 2025

---

## Current State (C++ Core Only)

```
┌──────────────────────────────────────────────────────┐
│                                                      │
│         GameCalls Engine (C++20)                     │
│                                                      │
│  ┌────────────────────────────────────────────┐     │
│  │   UnifiedAudioEngine                       │     │
│  │                                            │     │
│  │   • createSession()                        │     │
│  │   • loadMasterCall()                       │     │
│  │   • processAudioChunk()                    │     │
│  │   • finalizeSessionAnalysis()              │     │
│  │   • getEnhancedSummary()                   │     │
│  │                                            │     │
│  │   Components:                              │     │
│  │   - MFCC Processor                         │     │
│  │   - DTW Comparator                         │     │
│  │   - Pitch Tracker (YIN)                    │     │
│  │   - Harmonic Analyzer                      │     │
│  │   - Cadence Analyzer                       │     │
│  │   - Voice Activity Detector                │     │
│  └────────────────────────────────────────────┘     │
│                                                      │
│  In-Process API (C++ headers)                       │
│  • No network interface                             │
│  • Direct linking required                          │
│  • Used by: Native apps only                        │
│                                                      │
└──────────────────────────────────────────────────────┘
```

**Limitations**:
- ❌ Cannot be used by HMA Academy Web (JavaScript/React)
- ❌ No centralized session management
- ❌ No lesson integration with Academy Brain
- ❌ No server-side processing for analytics
- ❌ Requires native build for each platform

---

## Target State (Microservices Architecture)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                            CLIENT LAYER                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌───────────────────────────────┐    ┌─────────────────────────────┐  │
│  │   HMA Academy Web             │    │   HMFG Mobile Apps          │  │
│  │   (React 18 + Vite)           │    │   (React Native / Flutter)  │  │
│  │                               │    │                             │  │
│  │   • Lesson player             │    │   • Field mode              │  │
│  │   • Progress tracking         │    │   • Offline capable         │  │
│  │   • Achievement display       │    │   • GPS integration         │  │
│  │   • Coaching feedback UI      │    │   • Native audio recording  │  │
│  │                               │    │                             │  │
│  │   Port: 3004                  │    │   Hybrid Integration:       │  │
│  └───────────────┬───────────────┘    │   - REST API (lessons)      │  │
│                  │                     │   - Native C++ (real-time)  │  │
│                  │                     └───────────┬─────────────────┘  │
│                  │                                 │                     │
└──────────────────┼─────────────────────────────────┼─────────────────────┘
                   │                                 │
                   │                                 │
                   ▼                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                        API GATEWAY LAYER                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│           HMA Academy API Gateway (Port 3000)                           │
│                                                                         │
│   • JWT Authentication                                                  │
│   • Rate Limiting (per user/IP)                                         │
│   • Request Routing                                                     │
│   • CORS Management                                                     │
│   • Monitoring & Logging                                                │
│                                                                         │
│   Routes:                                                               │
│   /api/v1/strategy/*  → Hunt Strategy Engine (Port 8000)                │
│   /api/v1/gamecalls/* → GameCalls Engine (Port 5005)  ⬅ NEW            │
│                                                                         │
└──────────────────────────────────┬──────────────────────────────────────┘
                                   │
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                    GAMECALLS ENGINE SERVICE (Port 5005)                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  REST API Layer (Node.js + Express)                              │  │
│  │                                                                  │  │
│  │  Endpoints:                                                      │  │
│  │  ┌────────────────────────────────────────────────────────┐     │  │
│  │  │ Session Management                                     │     │  │
│  │  │  POST   /gamecalls/sessions                            │     │  │
│  │  │  DELETE /gamecalls/sessions/:id                        │     │  │
│  │  └────────────────────────────────────────────────────────┘     │  │
│  │  ┌────────────────────────────────────────────────────────┐     │  │
│  │  │ Real-Time Analysis                                     │     │  │
│  │  │  POST /gamecalls/sessions/:id/analyze                  │     │  │
│  │  └────────────────────────────────────────────────────────┘     │  │
│  │  ┌────────────────────────────────────────────────────────┐     │  │
│  │  │ Finalization & Results                                 │     │  │
│  │  │  POST /gamecalls/sessions/:id/finalize                 │     │  │
│  │  │  GET  /gamecalls/sessions/:id/results                  │     │  │
│  │  │  GET  /gamecalls/sessions/:id/overlay                  │     │  │
│  │  └────────────────────────────────────────────────────────┘     │  │
│  │  ┌────────────────────────────────────────────────────────┐     │  │
│  │  │ Master Call Library                                    │     │  │
│  │  │  GET /gamecalls/masters                                │     │  │
│  │  │  GET /gamecalls/masters/:id                            │     │  │
│  │  └────────────────────────────────────────────────────────┘     │  │
│  │  ┌────────────────────────────────────────────────────────┐     │  │
│  │  │ Academy Integration                                    │     │  │
│  │  │  POST /gamecalls/lessons/:id/session                   │     │  │
│  │  │  POST /gamecalls/lessons/:id/complete                  │     │  │
│  │  └────────────────────────────────────────────────────────┘     │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                   │                                     │
│                                   ▼                                     │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  Service Layer                                                   │  │
│  │                                                                  │  │
│  │  • EngineService (C++ binding wrapper)                           │  │
│  │  • SessionRepository (Redis cache)                               │  │
│  │  • ResultRepository (PostgreSQL)                                 │  │
│  │  • CoachingService (feedback generation)                         │  │
│  │  • AcademyIntegrationService (Brain API client)                  │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                   │                                     │
│                                   ▼                                     │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  Node-API Bindings Layer (C++ ⟷ JavaScript)                     │  │
│  │                                                                  │  │
│  │  JavaScript Functions → C++ Methods:                             │  │
│  │  • createSession(sampleRate)                                     │  │
│  │  • loadMasterCall(sessionId, masterId)                           │  │
│  │  • processChunk(sessionId, audioData)                            │  │
│  │  • getRealtimeState(sessionId)                                   │  │
│  │  • getSummary(sessionId)                                         │  │
│  │  • finalize(sessionId)                                           │  │
│  │  • destroySession(sessionId)                                     │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                   │                                     │
│                                   ▼                                     │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  C++ Core Engine (UnifiedAudioEngine)                            │  │
│  │                                                                  │  │
│  │  • MFCC Processor                                                │  │
│  │  • DTW Comparator                                                │  │
│  │  • Pitch Tracker (YIN)                                           │  │
│  │  • Harmonic Analyzer                                             │  │
│  │  • Cadence Analyzer                                              │  │
│  │  • Voice Activity Detector                                       │  │
│  │  • Finalize Stage (segmentation, refined DTW)                    │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
                   │                                   │
                   ▼                                   ▼
┌──────────────────────────────┐    ┌─────────────────────────────────┐
│     Redis (Port 6379)        │    │   PostgreSQL (Port 5432)        │
│                              │    │                                 │
│  Session State Cache:        │    │  Persistent Storage:            │
│  • session:${id}             │    │  • gamecalls_sessions           │
│  • analysis:${id}:latest     │    │  • gamecalls_results            │
│  • history:${id}             │    │  • gamecalls_masters            │
│                              │    │                                 │
│  TTL: 1 hour                 │    │  Relations:                     │
│  Use: Hot session data       │    │  • Session → Results (1:1)      │
│                              │    │  • Session → Lesson (N:1)       │
└──────────────────────────────┘    └─────────────────────────────────┘
                                                     │
                                                     ▼
                                    ┌─────────────────────────────────┐
                                    │   MinIO (Port 9000)             │
                                    │                                 │
                                    │  Audio File Storage:            │
                                    │  • Master call audio files      │
                                    │  • Waveform data                │
                                    │  • User recordings (optional)   │
                                    │                                 │
                                    │  Bucket: hma-gamecalls-assets   │
                                    └─────────────────────────────────┘
```

---

## Data Flow: Session Creation → Analysis → Finalization

```
┌──────────────┐
│  HMA Academy │ 1. POST /gamecalls/sessions
│  Web Client  │    { masterId: "whitetail-grunt-01" }
└──────┬───────┘
       │
       ▼
┌──────────────────────────────────────────────────────────────┐
│  GameCalls REST API (Node.js)                                │
│                                                              │
│  2. Validate request                                         │
│  3. Call C++ binding: createSession(44100)                   │
│  4. Call C++ binding: loadMasterCall(sessionId, masterId)    │
│  5. Store session metadata in Redis:                         │
│     {                                                        │
│       sessionId: "sess_abc123",                              │
│       masterId: "whitetail-grunt-01",                        │
│       status: "active",                                      │
│       createdAt: "2025-10-19T14:30:00Z"                      │
│     }                                                        │
│  6. Return: { sessionId: "sess_abc123", status: "ready" }   │
└──────────────────────────────────────────────────────────────┘
       │
       ▼
┌──────────────┐
│  Web Client  │ Loop: For each audio chunk (every 0.5 seconds)
└──────┬───────┘
       │
       ▼
┌──────────────────────────────────────────────────────────────┐
│  7. POST /gamecalls/sessions/sess_abc123/analyze             │
│     { audioChunk: "base64..." }                              │
│                                                              │
│  8. Decode base64 → Float32Array                             │
│  9. Call C++ binding: processChunk(sessionId, audioData)     │
│ 10. Call C++ binding: getRealtimeState(sessionId)            │
│ 11. Call C++ binding: getEnhancedSummary(sessionId)          │
│ 12. Cache latest analysis in Redis (TTL: 5 min)              │
│ 13. Append to history list in Redis                          │
│ 14. Return real-time metrics:                                │
│     {                                                        │
│       similarity: { score: 0.72, reliable: true },          │
│       pitch: { frequency: 245.5, confidence: 0.78 },        │
│       harmonic: { richness: 0.65 },                          │
│       cadence: { tempo: 120, consistency: 0.82 },           │
│       loudness: { rms: -12.5, deviation: 0.15 }             │
│     }                                                        │
└──────────────────────────────────────────────────────────────┘
       │
       ▼ (User stops recording)
┌──────────────┐
│  Web Client  │ POST /gamecalls/sessions/sess_abc123/finalize
└──────┬───────┘
       │
       ▼
┌──────────────────────────────────────────────────────────────┐
│ 15. Call C++ binding: finalizeSessionAnalysis(sessionId)     │
│     • Extract best segment (VAD + pitch stability)           │
│     • Run refined DTW on segment                             │
│     • Calculate loudness normalization                       │
│     • Compute final similarity score                         │
│                                                              │
│ 16. Generate coaching feedback (rule-based):                 │
│     • Map pitch/harmonic/cadence → grades (A-F)              │
│     • Match grades to feedback templates                     │
│     • Prioritize actionable tips                             │
│                                                              │
│ 17. Store results in PostgreSQL:                             │
│     INSERT INTO gamecalls_results (                          │
│       session_id, overall_similarity, pitch_grade,           │
│       coaching_feedback, created_at                          │
│     )                                                        │
│                                                              │
│ 18. Update session status in Redis: "finalized"              │
│                                                              │
│ 19. Return final results:                                    │
│     {                                                        │
│       finalMetrics: {                                        │
│         overallSimilarity: 0.78,                             │
│         similarityAtFinalize: 0.82,                          │
│         pitchGrade: "B", harmonicGrade: "B+",                │
│         cadenceGrade: "A-", loudnessDeviation: 0.12          │
│       },                                                     │
│       coaching: [                                            │
│         {                                                    │
│           category: "pitch",                                 │
│           message: "Good pitch control. Minor improvement...",│
│           actionable: "Focus on smooth call initiation."     │
│         }                                                    │
│       ],                                                     │
│       visualizations: {                                      │
│         overlayDataUrl: "/sessions/sess_abc123/overlay"      │
│       }                                                      │
│     }                                                        │
└──────────────────────────────────────────────────────────────┘
       │
       ▼
┌──────────────┐
│  Web Client  │ Display results UI
│              │ • Score badge (82%)
│              │ • Grade report card (B, B+, A-)
│              │ • Coaching tips (expandable)
│              │ • Waveform overlay visualization
│              │ • "Try Again" or "Next Lesson" buttons
└──────────────┘
```

---

## Academy Integration Flow: Lesson Completion

```
┌──────────────────┐
│  HMA Academy Web │ 1. User starts lesson "Basic Buck Grunt"
└────────┬─────────┘
         │
         ▼
┌────────────────────────────────────────────────────────────────┐
│  Academy Brain (Port 3001)                                     │
│                                                                │
│  2. GET /api/lessons/lesson_gamecalls_101                      │
│     Returns: { lesson details, masterId, passingGrade: 75 }   │
└────────────────────────────────────────────────────────────────┘
         │
         ▼
┌──────────────────┐
│  Frontend        │ 3. POST /gamecalls/lessons/lesson_gamecalls_101/session
└────────┬─────────┘    { userId: "user123" }
         │
         ▼
┌────────────────────────────────────────────────────────────────┐
│  GameCalls Engine                                              │
│                                                                │
│  4. Load lesson configuration from Academy Brain               │
│  5. Create session with lesson context                         │
│  6. Return: { sessionId, lesson: {...}, previousAttempts: 0 }  │
└────────────────────────────────────────────────────────────────┘
         │
         ▼ (User practices and finalizes)
┌──────────────────┐
│  Frontend        │ POST /gamecalls/lessons/lesson_gamecalls_101/complete
└────────┬─────────┘    { sessionId, userId }
         │
         ▼
┌────────────────────────────────────────────────────────────────┐
│  GameCalls Engine                                              │
│                                                                │
│  7. Retrieve final metrics from PostgreSQL                     │
│  8. Compare to lesson passing criteria (75% similarity)        │
│  9. Determine: PASSED (score: 82 >= 75)                        │
│ 10. Call Academy Brain:                                        │
│     POST /api/lessons/lesson_gamecalls_101/complete            │
│     { userId, score: 82, passed: true }                        │
│                                                                │
│     Academy Brain:                                             │
│     • Updates user progress                                    │
│     • Awards achievements (if first pass)                      │
│     • Unlocks next lesson                                      │
│     • Returns: { achievements: [...], nextLesson: {...} }      │
│                                                                │
│ 11. Return to frontend:                                        │
│     {                                                          │
│       passed: true, score: 82, passingGrade: 75,              │
│       achievements: [{                                         │
│         id: "first_passing_call",                              │
│         name: "Call Master Apprentice"                         │
│       }],                                                      │
│       nextLesson: { id: "lesson_gamecalls_102", title: "..." }│
│     }                                                          │
└────────────────────────────────────────────────────────────────┘
         │
         ▼
┌──────────────────┐
│  Frontend        │ Display success UI:
│                  │ • "Lesson Passed! 82/100"
│                  │ • Achievement badge animation
│                  │ • "Next Lesson" button
└──────────────────┘
```

---

## HMFG Mobile: Hybrid Integration (REST + Native)

```
┌──────────────────────────────────────────────────────────────┐
│                  HMFG Mobile App                             │
│                  (React Native)                              │
└──────────────────┬───────────────────────────────────────────┘
                   │
       ┌───────────┴───────────┐
       │                       │
       ▼                       ▼
┌──────────────────┐    ┌────────────────────────────┐
│  Lesson Mode     │    │  Field Mode (Real-Time)    │
│  (REST API)      │    │  (Native C++ Bindings)     │
└──────┬───────────┘    └────────┬───────────────────┘
       │                         │
       ▼                         ▼
┌──────────────────────────────────────────────────────────────┐
│  GameCalls REST API (Port 5005)                              │
│                                                              │
│  • User starts lesson in HMFG                                │
│  • REST API used for:                                        │
│    - Lesson session creation                                 │
│    - Progress tracking                                       │
│    - Completion & achievements                               │
│    - Server-side analytics                                   │
│                                                              │
│  Benefits:                                                   │
│  • Consistent lesson experience with HMA Academy             │
│  • Centralized progress tracking                             │
│  • No offline mode needed for lessons                        │
└──────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────┐
│  Native C++ GameCalls Engine (Direct Integration)           │
│                                                              │
│  • User records call in field (offline)                      │
│  • Native bindings used for:                                 │
│    - Zero-latency real-time analysis                         │
│    - Offline capability (no network required)                │
│    - Battery efficiency (no network requests)                │
│    - Full audio pipeline on device                           │
│                                                              │
│  Benefits:                                                   │
│  • Works without internet connection                         │
│  • Sub-10ms latency for real-time feedback                   │
│  • No bandwidth usage                                        │
│  • Privacy (audio never leaves device)                       │
└──────────────────────────────────────────────────────────────┘

Synchronization:
┌──────────────────────────────────────────────────────────────┐
│  When device comes online:                                   │
│                                                              │
│  1. Field recordings uploaded to MinIO (optional)            │
│  2. Field session metadata sent to REST API                  │
│  3. Analytics aggregated server-side                         │
│  4. User profile updated with field practice stats           │
└──────────────────────────────────────────────────────────────┘
```

---

## Comparison: Before vs After

### Before (Current State)
```
Limitations:
✗ No web access (React/JavaScript can't use C++ directly)
✗ Each platform builds separately (iOS, Android, Web)
✗ No centralized session management
✗ No lesson integration
✗ No server-side analytics
✗ No collaborative features possible
```

### After (Microservices)
```
Benefits:
✓ Web access via REST API (HMA Academy lessons)
✓ Centralized deployment (Docker container)
✓ Session state in Redis (scalable, recoverable)
✓ Deep Academy integration (lessons, achievements)
✓ Server-side analytics & monitoring
✓ Enables future features (leaderboards, shared recordings)
✓ Mobile can choose: REST (lessons) or Native (field)
✓ Single C++ codebase, multiple access patterns
```

---

## Performance Characteristics

### REST API Latency Budget
```
Client → API Gateway → GameCalls Engine → C++ Core

Step 1: Network (client → gateway)       ~10ms
Step 2: Gateway routing                   ~2ms
Step 3: Network (gateway → engine)        ~1ms (Docker internal)
Step 4: Express routing + validation      ~3ms
Step 5: Node-API binding overhead         ~1ms
Step 6: C++ processing (MFCC + analysis)  ~8ms
Step 7: Response serialization            ~2ms
Step 8: Network return                    ~3ms

Total: ~30ms (well under 50ms target ✓)
```

### Native Binding Latency Budget
```
HMFG Mobile App → Native C++ Engine (Direct)

Step 1: React Native bridge              ~2ms
Step 2: C++ processing (MFCC + analysis) ~8ms
Step 3: Bridge return                    ~2ms

Total: ~12ms (67% faster than REST ✓)

Use case: Real-time field analysis where every ms counts
```

---

## Security Layers

```
┌─────────────────────────────────────────────────────────┐
│  1. API Gateway (Port 3000)                             │
│     • JWT Authentication                                │
│     • Rate Limiting (100 req/min per user)              │
│     • CORS (allowed origins only)                       │
│     • Request size limits (10MB max)                    │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  2. GameCalls REST API (Port 5005)                      │
│     • Input validation (Joi schemas)                    │
│     • Session ownership checks                          │
│     • SQL injection prevention (parameterized queries)  │
│     • Error sanitization (no stack traces to client)    │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  3. C++ Core Engine                                     │
│     • Memory safety (bounds checking)                   │
│     • Result<T> error handling (no exceptions)          │
│     • Resource limits (max session count)               │
│     • Timeout guards (processing limits)                │
└─────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────┐
│  4. Data Layer                                          │
│     • Redis: TTL-based auto-cleanup                     │
│     • PostgreSQL: Row-level security (RLS)              │
│     • MinIO: Presigned URLs (temporary access)          │
│     • Encryption at rest (all databases)                │
└─────────────────────────────────────────────────────────┘
```

---

**Document Purpose**: Visual reference for architecture discussions  
**Audience**: Development team, stakeholders  
**Next**: See MICROSERVICES_ARCHITECTURE_GUIDE.md for implementation details
