# Hunt Master Academy Game Calls Engine – Architecture & Status
Last Updated: October 19, 2025
Status: MVP COMPLETE | Enhanced Analyzers Phase 1 Integrated | UX Alignment & Calibration Mapping

---

## 🆕 Microservices Architecture Planning

**NEW DOCUMENTATION**: The GameCalls Engine is being designed for microservices deployment to serve both Hunt Master Academy (educational) and Hunt Master Field Guide (mobile field app).

📚 **Key Resources**:
- **[Microservices Architecture Guide](MICROSERVICES_ARCHITECTURE_GUIDE.md)** - Complete REST API design, implementation roadmap, and technical specifications
- **[Hunt Strategy Comparison](HUNT_STRATEGY_COMPARISON.md)** - Side-by-side comparison with Hunt Strategy Engine's proven microservices pattern
- **Reference Implementation**: `/home/xbyooki/projects/hma-hunt-strategy-engine/backend/` (Node.js + Express wrapper)

**Target Architecture**:
```
C++ Core Engine (UnifiedAudioEngine)
         ↓
   Node-API Bindings
         ↓
REST API Wrapper (Node.js + Express, Port 5005)
         ↓
HMA API Gateway (Port 3000)
         ↓
    ┌─────────────┴──────────────┐
    ▼                            ▼
HMA Academy Web           HMFG Mobile Apps
(Lessons & Courses)       (Field Operations)
```

**Status**: Planning phase - See roadmap in MICROSERVICES_ARCHITECTURE_GUIDE.md

---

## 1. Current Phase Snapshot

| Dimension | Status | Notes |
|-----------|--------|-------|
| Core Engine | Production | Stable session orchestration & streaming pipeline |
| Enhanced Analyzers | Integrated (Pitch/Harmonic/Cadence) | All live; confidence smoothing active |
| Similarity | Realtime + DTW fallback | Readiness API present; finalize pass active |
| Visualization | Overlay export (data) | Offset control, countdown pending |
| Post-Processing | Finalize stage implemented | Segment-scoped refined DTW active |
| Loudness | Normalization active | Deviation & scalar populated in summary |
| Confidence Calibration | Partially calibrated | Grade table mapping in progress |
| Tests | Passing (0 skips) | Readiness removed sleeps; finalize tests in place |
| Performance | <12 ms enhanced path | Heavy cadence diagnostic path gated |
| Security | 100% | Complete & stable |
| Documentation | Realigned | This file + mvp_todo authoritative |

---

## 2. Immediate Goals (Execution Order)

1. Calibration tables: pitch/harmonic/cadence → graded bands.
2. Coaching feedback mapper (metrics → structured tips).
3. Waveform overlay offset control (aligned envelopes + offset).
4. Virtual clock instrumentation already present in tests.
8. Countdown integration (UI-level; provide engine timing hints).
9. Coaching feedback mapper (metrics → structured tips).
10. Heavy cadence diagnostic flag (explicit engine config gate).

---

## 3. Detailed Task Breakdown

### 3.1 Finalize Stage

| Task | Description | Priority |
|------|-------------|----------|
| Segment Extraction | Choose best contiguous active segment (VAD + pitch stability) | P0 (DONE) |
| Refined DTW | Run DTW on extracted segment only | P0 (DONE) |
| Normalization Scalar | Compute user gain to match master RMS | P0 (DONE) |
| Store Final Metrics | similarityAtFinalize, segmentStartMs, segmentDurationMs | P0 (DONE) |
| API | Status finalizeSessionAnalysis(SessionId) | P0 (DONE) |

### 3.2 Similarity Readiness

| Task | Description | Priority |
|------|-------------|----------|
| Compute Minimum Frames | Derived from MFCC window/hop constants | DONE |
| Expose State Struct | framesObserved, minFramesRequired, usingRealtimePath, reliable | DONE |
| Update Tests | Convert skip → assert | DONE |

### 3.3 Loudness Enhancements

| Task | Description | Priority |
|------|-------------|----------|
| Capture Master Loudness | RMS/peak at load | DONE |
| User Rolling Loudness | Per segment RMS | DONE |
| Deviation Metric | (userRMS - masterRMS)/masterRMS | DONE |
| Summary Fields | loudnessDeviation, normalizationScalar | DONE |

### 3.4 Waveform Overlay

| Task | Description | Priority |
|------|-------------|----------|
| Unified Timeline Model | Master length reference; user progress mapping | P2 |
| Export Overlay Data | Downsampled user & master peak arrays | P2 (BASE DONE) |
| Alignment Offset Control | Provide setOverlayOffset(SessionId, ms) | P3 |
| Visual Markers | Segment boundaries, pitch contour (future) | P3 |

### 3.5 Calibration

| Metric | Raw Source | Calibration Method | Output |
|--------|------------|-------------------|--------|
| Pitch Confidence | YIN | Empirical scale curves (lookup) | pitchGrade (A-F) |
| Harmonic Confidence | HarmonicAnalyzer | Percentile mapping | harmonicGrade |
| Cadence Consistency | Event intervals | Std deviation normalization | cadenceGrade |

### 3.6 Test & Infra

| Area | Action |
|------|--------|
| Similarity Tests | Replace skip with readiness loop (frame pacing) |
| Stale Summary Test | Inject virtual clock / advance by ms |
| Performance Guards | Add finalize path timing assertion |
| Calibration Tests | Golden fixtures vs expected grade |
| Segment Extraction Tests | Synthetic + real master pair cases |
| Loudness Tests | Controlled amplitude variation scenarios |

### 3.7 Coaching Feedback (Phase Edge)

| Input | Threshold Logic | Feedback Example |
|-------|-----------------|------------------|
| pitchGrade C or worse | Deviation > 25 cents median | “Stabilize pitch at call onset.” |
| cadenceGrade B but tempo var > target | Inter-onset CV > threshold | “Consistent spacing needed.” |
| loudnessDeviation > +20% | RMS mismatch positive | “Reduce volume for tonal clarity.” |
| harmonicGrade low & pitch good | Spectral roughness high | “Tone rough—focus on smoother airflow.” |

---

## 4. API Additions (Current + Planned)

```cpp
struct SimilarityRealtimeState {
    uint32_t framesObserved;
    uint32_t minFramesRequired;
    bool usingRealtimePath;
    bool reliable;
    float provisionalScore;
};

Result<SimilarityRealtimeState> getRealtimeSimilarityState(SessionId);

Status finalizeSessionAnalysis(SessionId); // Populates final metrics & locks segment scope

// Overlay export (implemented)
Result<OverlayData> exportOverlayData(SessionId, uint32_t decimation);
```

EnhancedAnalysisSummary fields (current + planned):
```cpp
float loudnessDeviation;
float normalizationScalar;
float similarityAtFinalize;
uint64_t segmentStartMs;
uint64_t segmentDurationMs;
char pitchGrade;      // 'A'..'F'
char harmonicGrade;
char cadenceGrade;
```

---

## 5. Acceptance Criteria (Near-Term)

| Category | Criteria |
|----------|----------|
| Finalize Stage | finalizeSessionAnalysis returns OK; segment metrics non-zero; similarityAtFinalize stable |
| Readiness | readiness.reliable true within N frames (configurable) < 250 ms audio |
| Tests | 0 skips; new finalize & readiness tests passing |
| Overhead | Added finalize path executes < 40 ms typical file; streaming path unchanged (<12 ms) |
| Calibration | Grade mapping deterministic & covered by tests |
| Loudness | Deviation accurate within ±2% vs analytic baseline |
| Overlay Export | Dual arrays length ratio matches expected decimation tolerance (<1 sample drift / block) |

---

## 6. Risk Log

| Risk | Impact | Mitigation |
|------|--------|------------|

---

## 7. Analysis & Calibration (Planned – Engine Scope)

Purpose

- Add deterministic calibration primitives that improve robustness across devices and environments while preserving performance and explainability. Orchestration (security/UX/CI) remains out of scope here.

Planned Components

- LoudnessCalibration
    - Computes peakLevelDbFS, rmsDbFS, noiseFloorDbFS, headroomDb
    - Recommends input gain: {increase|decrease|ok}
- LatencyDriftCalibrator
    - Estimates alignmentOffsetMs and driftPpm via loopback/clap test
    - Provides compensation hints to DTWComparator window/band
- EnvironmentProfiler
    - Captures ambient profile (noiseFloorDbFS, spectralTilt, bandEnergy)
    - Optional RNNoise path behind build/runtime flags (default: disabled)
- ScoreContributions
    - RealtimeScorer exposes contributions {mfcc,pitch,harmonic,cadence,loudness}
    - Provides why[]: stable factor IDs for UI mapping

Public API (planned, additive)

---

## Appendix: API Call Maps (Realtime, Finalize, Calibration)

Shared notes
- All calls return Status or Result<T>; check `.isOk()` before using `.value`.
- Common errors: INVALID_SESSION, BAD_CONFIG, NOT_READY, INSUFFICIENT_DATA, UNSUPPORTED, IO_ERROR.
- Perf budgets: streaming hop <12 ms; finalize <40 ms typical.

Realtime similarity (implemented)
- createSession(sampleRateHz) → Result<SessionId>
- loadMasterCall(session, masterIdOrUri) → Status
- enableEnhancedAnalyzers(session, true) → Status (optional)
- Loop per chunk:
    - processAudioChunk(session, span<const float>) → Status
    - getRealtimeSimilarityState(session) → Result<State{framesObserved,minFramesRequired,ready}>
    - If ready: getSimilarityScore(session), getEnhancedSummary(session), exportOverlayData(session)
- destroySession(session) → Status

Finalize path (implemented)
- finalizeSessionAnalysis(session) → Result<FinalizedSummary{similarityAtFinalize,segmentStartMs,segmentDurationMs,loudnessDeviation,normalizationScalar}>
- Optionally read getEnhancedSummary(session) after finalize

Calibration flows (planned)
- Mic Calibration Advisor: getCalibrationSummary(session) → Result<LoudnessCalibrationSummary>
- Latency/Drift: begin → submitImpulse → finalizeLatencyDriftCalibration(session) → Result<LatencyDriftReport>
- Environment Profiler: captureAmbientProfile(session,durationMs); setNoiseSuppressionEnabled(session,bool)

Mermaid overview

```mermaid
flowchart TD
    subgraph Realtime[Realtime Similarity (Implemented)]
        A1[createSession] --> A2[loadMasterCall]
        A2 --> A3[enableEnhancedAnalyzers (opt)]
        A3 --> A4[processAudioChunk (loop)]
        A4 --> A5{getRealtimeSimilarityState.ready?}
        A5 -- no --> A4
        A5 -- yes --> A6[getSimilarityScore]
        A6 --> A7[getEnhancedSummary]
        A7 --> A8[exportOverlayData]
    end

    subgraph Finalize[Finalize Path (Implemented)]
        F1[processAudioChunk (all audio)]
        F1 --> F2[finalizeSessionAnalysis]
    end

    subgraph Calibration[Analysis & Calibration (Planned)]
        C1[Mic Gain Advisor] --> C1a[getCalibrationSummary]
        C2[Latency/Drift] --> C2a[begin → submit → finalize]
        C3[Environment] --> C3a[captureAmbientProfile → setNoiseSuppressionEnabled]
    end
```


```cpp
struct LoudnessCalibrationSummary {
        float peakLevelDbFS;
        float rmsDbFS;
        float noiseFloorDbFS;
        float headroomDb;
        enum class Recommendation { Increase, Decrease, Ok } recommendation;
};

struct LatencyDriftReport {
        float alignmentOffsetMs; // may be positive or negative
        float driftPpm;          // parts per million
};

Result<LoudnessCalibrationSummary> getCalibrationSummary(SessionId);
Status beginLatencyDriftCalibration(SessionId);
Status submitCalibrationImpulse(SessionId, std::span<const float>);
Result<LatencyDriftReport> finalizeLatencyDriftCalibration(SessionId);

// Optional NR control (flag guarded, default false)
Status setNoiseSuppressionEnabled(SessionId, bool enabled);
Result<EnvironmentProfile> captureAmbientProfile(SessionId, uint32_t durationMs);

// Contributions/Why breakdown via enhanced summary JSON
```

Testing & Hooks

- Use HUNTMASTER_TEST_HOOKS for synthetic injection (zero/quiet/loud; offset/drift) without device I/O.
- Ensure sums of contributions ≈ totalScore within epsilon.
- Keep calibration off hot loops; verify perf guard remains <12 ms.

WASM Notes

- Expose summaries and JSON only; NR flag remains false by default.
| Segment mis-selection on noisy tails | Poor final similarity | Multi-heuristic scoring (VAD + energy + pitch stability) |
| Over-normalization clipping | Distorted analysis | Pre-scan headroom check & clamp scalar |
| Confidence misgrading early | Misleading coaching | Gate grading until min frames |
| Performance regression with finalize | UX delay | Lazy finalize invocation only on stop |
| Test flakiness (timing) | CI instability | Virtual clock abstraction |

---

## 7. Performance Guard Targets (Maintain)

| Path | Target |
|------|--------|
| processAudioChunk (enhanced) | <12 ms |
| Realtime Pitch | <4 ms |
| Cadence Incremental | <4 ms |
| finalizeSessionAnalysis | <40 ms typical (single segment) |
| Memory / Session | <15 MB upper bound after finalize |

---

## 8. Backlog (Deferred Until Core Gaps Closed)

| Item | Rationale |
|------|-----------|
| VolumeEnvelopeTracker | Depends on finalize & normalization |
| WaveformOverlayEngine advanced gestures | Need base overlay first |
| MasterCallAnalyzer batch pre-fingerprints | After finalize semantics fixed |
| AI Feedback Bridge | Needs stable graded metrics |
| Cloud Sync / CMS | Phase 4 per roadmap |
| Gamification / Progress Persistence | Needs calibration + grading stability |

---

## 9. Completed (Do Not Rework)

| Area | Completion |
|------|------------|
| PitchTracker YIN core + real confidence | Done |
| HarmonicAnalyzer base spectral pass | Done |
| CadenceAnalyzer base interval score | Done |
| EMA smoothing for confidence fields | Done |
| Enhanced summary integration path | Done |
| Analyzer lazy enable/disable + reset | Done |
| Master call MFCC distance baseline test | Done |
| Signed/unsigned warning cleanup | Done |
| Real pitch confidence exposure | Done |

---

## 10. Deprecations / Gated

| Feature | Status |
|---------|--------|
| Heavy cadence diagnostic mode | Gated (performance) |
| Legacy placeholder pitch confidence | Removed |
| Unused advanced harmonic placeholders | Zeroed until expansion |

---

## 11. Implementation Sequencing (Two-Week Micro Plan)

| Day Block | Focus |
|-----------|-------|
| D1–D2 | finalizeSessionAnalysis + segment heuristic + refined DTW |
| D3 | Loudness normalization + summary fields + tests |
| D4 | Readiness API + convert similarity skips → asserts |
| D5 | Virtual clock + stale test refactor |
| D6 | Calibration mapping (pitch/harmonic) + grade tests |
| D7 | Overlay export (data only) + performance validation |
| D8 | Coaching feedback mapper (non-AI) |
| D9 | Cleanup & Result<T> consistency audit |
| D10 | Performance regression sweep & doc refresh |
| D11–D12 | Pitch/harmonic calibration tuning vs sample set |
| D13 | Cadence pattern labeling prototype |
| D14 | Buffer for spillover / risk mitigation |

---

## 12. Metrics to Track

| Metric | Baseline | Target |
|--------|----------|--------|
| Frames to readiness | Variable | Deterministic threshold (doc’d) |
| finalizeSessionAnalysis latency | n/a | <40 ms typical |
| Similarity test skips | 2 | 0 |
| Confidence grade stability (std dev after readiness) | n/a | <10% of band width |
| Loudness normalization error | n/a | <2% RMS delta |
| Overlay time alignment drift | n/a | <1 frame equivalent |

---

## 13. Test Additions Required

| Test Name | Purpose |
|-----------|---------|
| FinalizeCreatesSegmentAndRefinedSimilarity | Validate finalize metrics population |
| LoudnessNormalizationAccuracy | Ensure RMS alignment post-scalar |
| SimilarityReadinessDeterministic | Frame threshold reliability |
| CalibrationGradeBoundaries | Grade edge correctness |
| OverlayExportAlignment | Master vs user time array alignment |
| SegmentSelectionEdgeCases | Multiple bursts / silence tails |
| VirtualClockStaleInvalidation | Time jump correctness without sleep |

---

## 14. Data / Assets Needed

| Asset | Use |
|-------|-----|
| Multiple pitch-stable master calls | Pitch calibration |
| Varied harmonic complexity clips | Harmonic calibration |
| Intentionally mistimed user attempts | Cadence grading validation |
| Variable loudness variants of same call | Loudness normalization test |

---

## 15. Documentation Tasks (Parallel)

| Doc | Update |
|-----|--------|
| README.md | (Completed – synced) |
| API docs (public headers) | Add finalize/readiness once implemented |
| Architecture | Add finalize stage & overlay pipeline box |
| Testing Guide | Add virtual clock + new test categories |
| Deployment | Performance guard update if finalize affects latency |

---

## 16. Out-of-Scope (For Now)

- Cloud synchronization
- CMS ingestion
- AI narrative generation
- Gamified scoring loops
- Mobile platform energy tuning

These depend on stable finalized metrics and calibration pipeline.

---

## 17. Exit Criteria for Current Phase

All of the following:
1. finalizeSessionAnalysis implemented & tested (segment + refined DTW).
2. No skipped tests; readiness API drives assertions.
3. Loudness normalization & deviation metric in summary.
4. Calibration grades (pitch/harmonic) appear & stable.
5. Overlay export available (engine data format).
6. Virtual clock removes sleep delays.
7. Performance thresholds maintained (<12 ms streaming path).
8. README + mvp_todo reflect new APIs & metrics.

---

## 18. Quick Progress Checklist (Tick as Completed)

- [ ] finalizeSessionAnalysis core
- [ ] Segment heuristic integration
- [ ] Refined DTW result stored
- [ ] Loudness normalization applied
- [ ] Readiness API exposed
- [ ] Similarity tests assert (0 skips)
- [ ] Virtual clock implemented
- [ ] Calibration mapping & tests
- [ ] Overlay export + test
- [ ] Coaching mapper (initial)
- [ ] Performance revalidation
- [ ] Documentation sync (headers + architecture)
- [ ] Result<T> usage audit complete

---

## 19. Notes & Constraints

- Maintain zero-copy spans (no buffering copies beyond existing MFCC windows).
- Finalize must not mutate original raw audio (analysis-only transformations).
- Normalization scalar applied only in analysis domain (not persisted).
- Grades must be reproducible (no random seeds).
- Keep finalize idempotent; repeat calls return last computed metrics.

---

## 20. Summary

Core engine & enhanced analyzers are stable. The next critical evolution is user-practice fidelity: segmentation, readiness clarity, loudness normalization, calibrated confidence → actionable coaching. This TODO enumerates the focused path to deliver a cohesive, low-latency, instruction-ready analysis loop without scope creep into later phase cloud or gamification tiers.
