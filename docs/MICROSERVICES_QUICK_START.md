# GameCalls Engine - Microservices Quick Start

**Last Updated:** October 19, 2025  
**Status:** Planning Phase  
**Purpose:** Quick reference for starting microservices implementation

---

## 🎯 Goal

Wrap the C++ GameCalls Engine with a REST API to enable:
1. **HMA Academy** - Course lessons with game call analysis
2. **HMFG Mobile** - Real-time field analysis for hunters

---

## 📚 Documentation Guide

### Start Here
1. **[HUNT_STRATEGY_COMPARISON.md](HUNT_STRATEGY_COMPARISON.md)** - Understand the pattern (10 min read)
2. **[MICROSERVICES_ARCHITECTURE_GUIDE.md](MICROSERVICES_ARCHITECTURE_GUIDE.md)** - Full implementation guide (30 min read)
3. **[architecture.md](architecture.md)** - Updated with microservices context

### Reference Implementation
- **Hunt Strategy Backend**: `/home/xbyooki/projects/hma-hunt-strategy-engine/backend/`
- **Express Server**: `backend/src/index.js`
- **API Routes**: `backend/src/routes/`
- **Docker Config**: `backend/Dockerfile.dev`

---

## 🚀 Quick Start Checklist

### Week 1-2: Foundation
- [ ] Read comparison doc (HUNT_STRATEGY_COMPARISON.md)
- [ ] Review Hunt Strategy backend code
- [ ] Create `hma-gamecalls-engine/backend/` directory
- [ ] Initialize Node.js project (`npm init`)
- [ ] Install dependencies (express, cors, helmet, etc.)
- [ ] Create basic Express server (port 5005)
- [ ] Add health check endpoint (`GET /health`)
- [ ] Test: `curl http://localhost:5005/health`

### Week 3-4: C++ Binding POC
- [ ] Research Node-API (N-API) approach
- [ ] Create `bindings/` directory
- [ ] Write minimal binding: `createSession()` wrapper
- [ ] Test: Call C++ from JavaScript
- [ ] Document binding architecture
- [ ] Validate performance (latency acceptable?)

### Week 5-6: Session Management
- [ ] Implement `POST /gamecalls/sessions`
- [ ] Implement `DELETE /gamecalls/sessions/:id`
- [ ] Add Redis for session state
- [ ] Add session validation middleware
- [ ] Write integration tests
- [ ] Test: Create and destroy sessions via REST

### Week 7-8: Real-Time Analysis
- [ ] Implement `POST /gamecalls/sessions/:id/analyze`
- [ ] Add base64 audio chunk decoding
- [ ] Call C++ `processAudioChunk()` via binding
- [ ] Format and return analysis metrics
- [ ] Performance test: < 50ms P95 latency
- [ ] Test: Stream audio chunks, get real-time feedback

### Week 9-10: Finalization & Results
- [ ] Implement `POST /gamecalls/sessions/:id/finalize`
- [ ] Implement `GET /gamecalls/sessions/:id/results`
- [ ] Add coaching feedback generation
- [ ] Store results in PostgreSQL
- [ ] Test: Full session lifecycle works

### Week 11-12: HMA Academy Integration
- [ ] Implement `POST /gamecalls/lessons/:id/session`
- [ ] Implement `POST /gamecalls/lessons/:id/complete`
- [ ] Integrate with Academy Brain API
- [ ] Test: Lesson flow end-to-end
- [ ] Test: Achievement awards work

### Week 13-14: Docker & Testing
- [ ] Create `Dockerfile.dev` (multi-stage build)
- [ ] Add to `hma-infra/docker/docker-compose.yml`
- [ ] Write comprehensive tests (80% coverage target)
- [ ] Load test (50 concurrent sessions)
- [ ] Document API (OpenAPI spec)
- [ ] Test: Full stack via `docker-compose up`

---

## 🏗️ Directory Structure

```
hma-gamecalls-engine/
├── include/                    # C++ headers (existing)
├── src/                        # C++ source (existing)
├── bindings/                   # NEW - Node-API bindings
│   ├── gamecalls_addon.cpp
│   └── binding.gyp
├── backend/                    # NEW - REST API
│   ├── src/
│   │   ├── index.js           # Express app
│   │   ├── routes/
│   │   │   ├── gamecalls.js   # Main router
│   │   │   ├── sessions.js    # Session CRUD
│   │   │   ├── analysis.js    # Real-time analysis
│   │   │   ├── finalize.js    # Finalization
│   │   │   ├── masters.js     # Master call library
│   │   │   └── lessons.js     # Academy integration
│   │   ├── controllers/
│   │   ├── services/
│   │   │   ├── engineService.js    # C++ binding wrapper
│   │   │   ├── coachingService.js  # Feedback generation
│   │   │   └── academyIntegrationService.js
│   │   ├── repositories/
│   │   │   ├── sessionRepository.js  # Redis
│   │   │   └── resultRepository.js   # PostgreSQL
│   │   ├── middleware/
│   │   │   ├── errorHandler.js
│   │   │   ├── logger.js
│   │   │   └── validation.js
│   │   └── utils/
│   ├── tests/
│   │   ├── unit/
│   │   ├── integration/
│   │   └── load/
│   ├── migrations/
│   │   └── 001_gamecalls_schema.sql
│   ├── docs/
│   │   ├── API_CONTRACT.md
│   │   └── BINDINGS.md
│   ├── package.json
│   ├── Dockerfile.dev
│   └── .dockerignore
└── docs/
    ├── MICROSERVICES_ARCHITECTURE_GUIDE.md  # NEW
    ├── HUNT_STRATEGY_COMPARISON.md          # NEW
    └── architecture.md                      # UPDATED
```

---

## 🔑 Key Endpoints

### Session Management
```bash
# Create session
POST /gamecalls/sessions
Body: { "masterId": "whitetail-grunt-01", "mode": "educational" }
Response: { "sessionId": "sess_abc123" }

# Destroy session
DELETE /gamecalls/sessions/:sessionId
Response: 204 No Content
```

### Real-Time Analysis
```bash
# Analyze audio chunk
POST /gamecalls/sessions/:sessionId/analyze
Body: { "audioChunk": "base64...", "chunkIndex": 0 }
Response: { "analysis": { "similarity": 0.72, "pitch": {...}, ... } }
```

### Finalization
```bash
# Finalize session
POST /gamecalls/sessions/:sessionId/finalize
Response: { "finalMetrics": {...}, "coaching": [...] }

# Get results
GET /gamecalls/sessions/:sessionId/results
Response: { "metrics": {...}, "history": [...], "coaching": [...] }
```

### Master Calls
```bash
# List master calls
GET /gamecalls/masters?species=whitetail-deer
Response: { "masters": [...] }

# Get master details
GET /gamecalls/masters/:masterId
Response: { "id": "...", "name": "...", "audioUrl": "..." }
```

### Academy Integration
```bash
# Start lesson session
POST /gamecalls/lessons/:lessonId/session
Body: { "userId": "user123" }
Response: { "sessionId": "...", "lesson": {...} }

# Complete lesson
POST /gamecalls/lessons/:lessonId/complete
Body: { "sessionId": "...", "userId": "..." }
Response: { "passed": true, "score": 82, "achievements": [...] }
```

---

## 🔧 Essential Commands

### Development
```bash
# Start backend server
cd backend
npm install
npm run dev

# Test health
curl http://localhost:5005/health

# Run tests
npm test

# Check coverage
npm run coverage
```

### Docker
```bash
# Build backend
cd backend
docker build -t hma-gamecalls-engine:dev -f Dockerfile.dev .

# Run backend container
docker run -p 5005:5005 \
  -e NODE_ENV=development \
  -e REDIS_HOST=redis \
  hma-gamecalls-engine:dev

# Full stack (from hma-infra)
cd /home/xbyooki/projects/hma-infra/docker
docker-compose up -d hma-gamecalls-engine
docker-compose logs -f hma-gamecalls-engine
```

### Testing
```bash
# Unit tests
npm test

# Integration tests
npm run test:integration

# Load tests
npm run test:load

# Test specific endpoint
curl -X POST http://localhost:5005/gamecalls/sessions \
  -H "Content-Type: application/json" \
  -d '{"masterId":"whitetail-grunt-01","mode":"educational"}'
```

---

## 📊 Performance Targets

| Operation | Target (P95) | Notes |
|-----------|-------------|-------|
| Session creation | < 100ms | Includes C++ engine init |
| Audio chunk analysis | < 50ms | Real-time requirement |
| Session finalization | < 200ms | Includes DTW refinement |
| Master call list | < 100ms | Database query |
| Concurrent sessions | 50+ | Load test target |

---

## 🚨 Common Pitfalls

### 1. C++ Memory Management
❌ **Don't**: Let JavaScript garbage collect C++ objects  
✅ **Do**: Explicitly destroy sessions via `DELETE /sessions/:id`

### 2. Audio Chunk Size
❌ **Don't**: Send massive chunks (>10MB base64)  
✅ **Do**: Stream small chunks (1-2 seconds, ~100KB)

### 3. Session Timeouts
❌ **Don't**: Keep sessions indefinitely  
✅ **Do**: Set Redis TTL (1 hour) and cleanup expired sessions

### 4. Error Handling
❌ **Don't**: Expose C++ error details to clients  
✅ **Do**: Map C++ Status codes to HTTP status + generic messages

### 5. Performance
❌ **Don't**: Encode/decode base64 on hot path  
✅ **Do**: Consider binary WebSocket for HMFG field app

---

## 🎓 Learning Resources

### Node.js Native Addons
- **Node-API Docs**: https://nodejs.org/api/n-api.html
- **node-addon-api**: https://github.com/nodejs/node-addon-api
- **Examples**: https://github.com/nodejs/node-addon-examples

### Express.js Best Practices
- **Express Docs**: https://expressjs.com/
- **Error Handling**: https://expressjs.com/en/guide/error-handling.html
- **Performance**: https://expressjs.com/en/advanced/best-practice-performance.html

### Microservices Patterns
- **Hunt Strategy Reference**: `/home/xbyooki/projects/hma-hunt-strategy-engine/backend/`
- **System Architecture**: `/home/xbyooki/projects/hma-docs/architecture/system-verification.md`
- **API Gateway**: `/home/xbyooki/projects/hma-academy-api/docs/system-connectivity-map.md`

---

## ✅ Success Criteria

### Phase 1 (Weeks 1-2)
- ✅ Express server running on port 5005
- ✅ Health check returns 200 OK
- ✅ Docker container builds successfully

### Phase 2 (Weeks 3-4)
- ✅ C++ binding POC works
- ✅ Can create session from JavaScript
- ✅ Performance acceptable (< 10ms overhead)

### Phase 3 (Weeks 5-8)
- ✅ Full session lifecycle via REST API
- ✅ Real-time analysis returns accurate metrics
- ✅ Load test: 50 concurrent sessions pass

### Phase 4 (Weeks 9-12)
- ✅ HMA Academy integration complete
- ✅ Lessons work end-to-end
- ✅ Achievements awarded correctly

### Phase 5 (Weeks 13-14)
- ✅ Docker Compose full stack works
- ✅ 80%+ test coverage
- ✅ API documentation complete

---

## 📞 Getting Help

### Documentation
1. Check **MICROSERVICES_ARCHITECTURE_GUIDE.md** (detailed specs)
2. Review **HUNT_STRATEGY_COMPARISON.md** (pattern examples)
3. Inspect Hunt Strategy backend code (reference implementation)

### Common Questions
- **Q**: How do I wrap C++ functions?  
  **A**: See "C++ to Node.js Binding Strategy" in architecture guide

- **Q**: How do I handle audio streaming?  
  **A**: See "Audio Data Formats" section for base64 vs binary WebSocket

- **Q**: How do I integrate with HMA Academy?  
  **A**: See "HMA Academy Integration Endpoints" section + Phase 5 roadmap

- **Q**: What database schema do I need?  
  **A**: See "Session State Management" section for Redis + PostgreSQL schemas

---

## 🎯 Next Steps

1. ✅ Read this quick start (you're here!)
2. ⬜ Read [HUNT_STRATEGY_COMPARISON.md](HUNT_STRATEGY_COMPARISON.md)
3. ⬜ Skim [MICROSERVICES_ARCHITECTURE_GUIDE.md](MICROSERVICES_ARCHITECTURE_GUIDE.md)
4. ⬜ Review Hunt Strategy backend code
5. ⬜ Start Week 1-2 checklist (create backend directory)
6. ⬜ Schedule team meeting to assign ownership
7. ⬜ Create GitHub project board for tracking milestones

---

**Ready to begin?** Start with the Week 1-2 checklist above!

**Questions?** Review the full architecture guide for detailed answers.

**Blocked?** Reference Hunt Strategy backend code for proven patterns.
