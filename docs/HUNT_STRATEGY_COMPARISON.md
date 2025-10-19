# Hunt Strategy vs GameCalls Engine - Microservices Pattern Comparison

**Last Updated:** October 19, 2025  
**Purpose:** Quick reference for understanding how Hunt Strategy Engine's microservices approach applies to GameCalls Engine

---

## Side-by-Side Comparison

| Aspect | Hunt Strategy Engine | GameCalls Engine |
|--------|---------------------|------------------|
| **Core Technology** | Flutter/Dart (multi-platform) | C++20 (performance-critical) |
| **Port** | 8000 | 5005 |
| **Backend Wrapper** | Node.js + Express | Node.js + Express (planned) |
| **Primary Data Source** | Google Earth Engine, Weather APIs | Audio files, master call library |
| **Real-Time Requirement** | No (batch predictions) | Yes (streaming audio analysis) |
| **Native Integration** | Flutter compiled to web | C++ via Node-API bindings |

---

## Architectural Pattern Match

### Hunt Strategy: Domain Logic → REST Wrapper

```
Flutter/Dart Domain Logic (in-process)
         ↓
    Service Layer
         ↓
REST API Wrapper (Node.js/Express)
         ↓
    Endpoints:
    - POST /strategy/predictions/heatmap
    - POST /strategy/waypoints
    - POST /strategy/plans
```

### GameCalls: C++ Core → REST Wrapper

```
C++ Core Engine (UnifiedAudioEngine)
         ↓
   Node-API Bindings
         ↓
REST API Wrapper (Node.js/Express)
         ↓
    Endpoints:
    - POST /gamecalls/sessions
    - POST /gamecalls/sessions/:id/analyze
    - POST /gamecalls/sessions/:id/finalize
```

---

## Key Similarities (Patterns to Reuse)

### 1. Express Server Structure
**Hunt Strategy** (`backend/src/index.js`):
```javascript
const express = require('express');
const app = express();
const PORT = 8000;

app.use(helmet());
app.use(cors());
app.use(express.json());

app.use('/strategy', strategyRoutes);

app.listen(PORT, () => {
    console.log(`🚀 Hunt Strategy Engine API running on port ${PORT}`);
});
```

**GameCalls** (planned):
```javascript
const express = require('express');
const app = express();
const PORT = 5005;

app.use(helmet());
app.use(cors());
app.use(express.json());

app.use('/gamecalls', gameCallsRoutes);

app.listen(PORT, () => {
    console.log(`🚀 GameCalls Engine API running on port ${PORT}`);
});
```

### 2. Route Organization
**Hunt Strategy**:
```
backend/src/routes/
├── strategy.js          # Main router
├── predictions.js       # POST /strategy/predictions/*
├── waypoints.js         # CRUD /strategy/waypoints
├── plans.js             # CRUD /strategy/plans
└── validations.js       # Input validation
```

**GameCalls** (planned):
```
backend/src/routes/
├── gamecalls.js         # Main router
├── sessions.js          # POST /gamecalls/sessions
├── analysis.js          # POST /gamecalls/sessions/:id/analyze
├── finalize.js          # POST /gamecalls/sessions/:id/finalize
├── masters.js           # GET /gamecalls/masters
└── lessons.js           # Academy integration
```

### 3. Health Check Pattern
**Both engines**:
```javascript
app.get('/health', (req, res) => {
    res.json({
        status: 'ok',
        service: 'hunt-strategy-engine', // or 'gamecalls-engine'
        version: '1.0.0',
        timestamp: new Date().toISOString()
    });
});
```

### 4. Error Handling Middleware
**Both use centralized error handling**:
```javascript
// middleware/errorHandler.js
function errorHandler(err, req, res, next) {
    const status = err.status || 500;
    const code = err.code || 'INTERNAL_ERROR';
    
    res.status(status).json({
        error: {
            code,
            message: err.message,
            timestamp: new Date().toISOString()
        }
    });
}
```

### 5. Docker Configuration
**Hunt Strategy** (`Dockerfile.dev`):
```dockerfile
FROM node:22-alpine
WORKDIR /app
COPY package*.json ./
RUN npm install
COPY . .
EXPOSE 8000
CMD ["npm", "run", "dev"]
```

**GameCalls** (planned):
```dockerfile
FROM node:22-alpine
# Install C++ build tools
RUN apk add --no-cache cmake g++ make
WORKDIR /app

# Build C++ core
COPY ../CMakeLists.txt ../src ../include ./cpp/
RUN cd cpp && cmake -B build && cmake --build build

# Install Node.js dependencies
COPY backend/package*.json ./
RUN npm install

COPY backend/ ./
EXPOSE 5005
CMD ["npm", "run", "dev"]
```

---

## Key Differences (Adaptations Needed)

### 1. Native Binding Layer
**Hunt Strategy**: No native bindings needed (Flutter compiles to JavaScript for web)

**GameCalls**: Requires Node-API bindings to call C++ functions:
```cpp
// bindings/gamecalls_addon.cpp
#include <napi.h>
#include "huntmaster/core/UnifiedAudioEngine.h"

Napi::Value CreateSession(const Napi::CallbackInfo& info) {
    // Wrap C++ call
    auto result = UnifiedAudioEngine::create();
    // Convert to JavaScript value
    return Napi::String::New(info.Env(), sessionId);
}

NODE_API_MODULE(gamecalls_addon, Init)
```

### 2. Real-Time Streaming
**Hunt Strategy**: Stateless request/response (predictions generated on-demand)

**GameCalls**: Stateful streaming (audio chunks processed sequentially):
```javascript
// Hunt Strategy: Simple POST
POST /strategy/predictions/heatmap
Body: { bounds, conditions }
Response: { heatmap, factors }

// GameCalls: Streaming session
POST /gamecalls/sessions → { sessionId }
POST /gamecalls/sessions/:id/analyze (repeat for each chunk)
POST /gamecalls/sessions/:id/finalize → { results }
```

### 3. Session State Management
**Hunt Strategy**: No persistent sessions (stateless)

**GameCalls**: Session state in Redis:
```javascript
// Store session metadata
redis.set(`session:${sessionId}`, {
    userId,
    masterId,
    chunksProcessed: 0,
    status: 'active'
}, 'EX', 3600);  // 1 hour TTL

// Update on each chunk
redis.hincrby(`session:${sessionId}`, 'chunksProcessed', 1);
```

### 4. Data Persistence
**Hunt Strategy**: Predictions are ephemeral (not stored)

**GameCalls**: Results stored in PostgreSQL:
```sql
CREATE TABLE gamecalls_results (
    session_id VARCHAR(64),
    similarity_score DECIMAL(5,2),
    pitch_grade CHAR(2),
    coaching_feedback JSONB,
    created_at TIMESTAMPTZ
);
```

### 5. Academy Integration
**Hunt Strategy**: No direct lesson integration (predictions consumed by Academy Brain)

**GameCalls**: Deep lesson integration:
```javascript
// Lesson-specific endpoint
POST /gamecalls/lessons/:lessonId/session
Body: { userId }
Response: { sessionId, lesson: { title, objectives, passingGrade } }

// Completion endpoint
POST /gamecalls/lessons/:lessonId/complete
Body: { sessionId, userId }
Response: { passed, score, achievements, nextLesson }
```

---

## Dual-Mode Support Pattern

### Hunt Strategy: Deterministic vs Production
```javascript
// Deterministic mode (HMA Academy)
POST /strategy/predictions/heatmap
Body: {
    mode: 'deterministic',
    bounds: { ... },
    seed: 12345  // Reproducible results
}

// Production mode (HMFG Field)
POST /strategy/predictions/heatmap
Body: {
    mode: 'production',
    bounds: { ... }
    // Uses real Google Earth Engine data
}
```

### GameCalls: Educational vs Field
```javascript
// Educational mode (HMA Academy lessons)
POST /gamecalls/sessions
Body: {
    mode: 'educational',
    masterId: 'whitetail-grunt-01',
    lessonId: 'lesson_101'  // Lesson-specific grading
}

// Field mode (HMFG real-world use)
POST /gamecalls/sessions
Body: {
    mode: 'field',
    masterId: 'whitetail-grunt-01'
    // Adaptive thresholds, no lesson constraints
}
```

---

## API Gateway Integration

### Both engines integrate through HMA API Gateway (port 3000)

**Gateway Routing** (`hma-academy-api`):
```javascript
// Hunt Strategy routes
app.use('/api/v1/strategy/*', proxy({
    target: 'http://hunt-strategy:8000',
    changeOrigin: true
}));

// GameCalls routes (planned)
app.use('/api/v1/gamecalls/*', proxy({
    target: 'http://gamecalls-engine:5005',
    changeOrigin: true
}));
```

**Client Usage**:
```typescript
// Frontend calls via gateway
const response = await fetch('http://localhost:3000/api/v1/gamecalls/sessions', {
    method: 'POST',
    headers: {
        'Authorization': `Bearer ${token}`,
        'Content-Type': 'application/json'
    },
    body: JSON.stringify({ masterId: 'whitetail-grunt-01' })
});
```

---

## Docker Compose Configuration

### Hunt Strategy Service (Existing)
```yaml
# hma-infra/docker/docker-compose.yml
hma-hunt-strategy-engine:
  build: ../../hma-hunt-strategy-engine/backend
  ports:
    - "8000:8000"
  environment:
    - NODE_ENV=development
    - PORT=8000
    - POSTGRES_HOST=postgres
  depends_on:
    - postgres
```

### GameCalls Service (Planned)
```yaml
# hma-infra/docker/docker-compose.yml
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
```

---

## Performance Targets Comparison

| Metric | Hunt Strategy | GameCalls |
|--------|--------------|-----------|
| **Endpoint Latency (P95)** | | |
| Session creation | N/A | < 100ms |
| Prediction/Analysis | < 750ms | < 50ms |
| Finalization | N/A | < 200ms |
| **Throughput** | | |
| Concurrent predictions | 100 req/s | N/A |
| Concurrent sessions | N/A | 50 active |
| **Resource Usage** | | |
| Memory per request | ~50 MB | ~20 MB per session |
| CPU per request | ~10% (1 core) | ~30% (1 core, streaming) |

---

## Testing Strategy Comparison

### Hunt Strategy: Contract + Integration Tests
```javascript
// tests/predictions.test.js
describe('Predictions API', () => {
    it('should generate heatmap for valid bounds', async () => {
        const response = await request(app)
            .post('/strategy/predictions/heatmap')
            .send({ bounds: { ... } });
        
        expect(response.status).toBe(200);
        expect(response.body.tiles).toBeDefined();
    });
});
```

### GameCalls: Streaming + Stateful Tests
```javascript
// tests/sessions.test.js
describe('GameCalls Session Flow', () => {
    let sessionId;
    
    it('should create session', async () => {
        const response = await request(app)
            .post('/gamecalls/sessions')
            .send({ masterId: 'whitetail-grunt-01' });
        
        expect(response.status).toBe(201);
        sessionId = response.body.sessionId;
    });
    
    it('should process audio chunk', async () => {
        const response = await request(app)
            .post(`/gamecalls/sessions/${sessionId}/analyze`)
            .send({ audioChunk: 'base64...' });
        
        expect(response.status).toBe(200);
        expect(response.body.analysis.similarity).toBeDefined();
    });
    
    it('should finalize session', async () => {
        const response = await request(app)
            .post(`/gamecalls/sessions/${sessionId}/finalize`);
        
        expect(response.status).toBe(200);
        expect(response.body.finalMetrics).toBeDefined();
    });
});
```

---

## Migration Path: From C++ Core to Microservice

### Phase 1: Standalone C++ Engine (Current)
```
Developer → Includes C++ headers → Compiles with engine → Native app
```

### Phase 2: REST API Wrapper (Target)
```
Developer → REST API calls → GameCalls Backend → C++ Engine (via bindings)
```

### Phase 3: Hybrid Deployment (HMFG)
```
Mobile App:
├── REST API (for lessons, analytics) → Backend Service
└── Native Bindings (for real-time field use) → Local C++ Engine
```

**Benefit**: Flexibility to use backend for lessons/analytics while maintaining native performance for field operations.

---

## Key Takeaways

### ✅ Patterns to Adopt from Hunt Strategy
1. **Express.js REST wrapper** - Proven, scalable pattern
2. **Route organization** - Clear separation of concerns
3. **Docker-first deployment** - Works in full stack
4. **Health checks & monitoring** - Essential for microservices
5. **Dual-mode support** - Educational vs Field
6. **API Gateway integration** - Centralized entry point

### ⚠️ GameCalls-Specific Considerations
1. **C++ bindings required** - Use Node-API for stability
2. **Session state management** - Redis for active sessions
3. **Real-time streaming** - Different from request/response
4. **Performance critical** - Audio processing has strict latency requirements
5. **Academy integration** - Tighter coupling with lessons/achievements

### 🎯 Success Criteria
- **Week 4**: Basic REST API + C++ binding working
- **Week 8**: Full session lifecycle (create → analyze → finalize)
- **Week 12**: HMA Academy integration complete
- **Week 16**: Docker deployment + load testing passed

---

## Next Actions

1. ✅ **Read this comparison** - Understand patterns
2. ⬜ **Review Hunt Strategy backend code** - Study implementation
3. ⬜ **POC C++ binding** - Validate Node-API approach
4. ⬜ **Create backend directory** - Start with structure
5. ⬜ **Implement basic routes** - Session create/destroy
6. ⬜ **Add to docker-compose** - Full stack integration
7. ⬜ **Write integration tests** - Validate end-to-end

---

## References

- **Full Guide**: `MICROSERVICES_ARCHITECTURE_GUIDE.md`
- **Hunt Strategy Backend**: `/home/xbyooki/projects/hma-hunt-strategy-engine/backend/`
- **System Architecture**: `/home/xbyooki/projects/hma-docs/architecture/system-verification.md`
- **Node-API Docs**: https://nodejs.org/api/n-api.html

---

**Status**: ✅ Reference Complete  
**Audience**: Development Team  
**Next Review**: After backend POC
