# Hunt Master Academy Game Calls Engine – Updated Comprehensive TODO & Roadmap
Last Updated: August 10, 2025
Status: MVP COMPLETE | Enhanced Analyzers Phase 1 Integrated | Entering UX Alignment & Finalization

---

## 1. Current Phase Snapshot

| Dimension | Status | Notes |
|-----------|--------|-------|
| Core Engine | Production | Stable session orchestration & streaming pipeline |
| Enhanced Analyzers | Integrated (Pitch/Harmonic/Cadence) | All live; confidence smoothing active |
| Similarity | Realtime + DTW fallback | Need readiness introspection & finalize pass |
| Visualization | Waveform (single) | Overlay, alignment, countdown pending |
| Post-Processing | Missing finalize stage | Segmentation + refined DTW not yet added |
| Loudness | Monitoring only | Normalization & deviation metrics pending |
| Confidence Calibration | Not calibrated | Raw → grade mapping required |
| Tests | Passing (19, 2 skips) | Skips due to similarity readiness opaque |
| Performance | <12 ms enhanced path | Heavy cadence diagnostic path gated |
| Security | 100% | Complete & stable |
| Documentation | Being realigned | This file authoritative reference |

---

## 2. Immediate Goals (Execution Order)

1. finalizeSessionAnalysis() – API & basic full-range pass implemented (segment heuristic, true RMS, loudnessDeviation still pending).
2. Similarity readiness API – getRealtimeSimilarityState().
3. Loudness normalization factor & summary fields (loudnessDeviation, segmentStartMs, segmentDurationMs).
4. Convert similarity skips → asserts (after readiness instrumentation).
5. Virtual clock abstraction (remove sleep-based stale invalidation test).
6. Calibration tables: pitchConfidence & harmonicConfidence → graded bands.
7. Waveform overlay data export (aligned envelopes + offset).
8. Countdown integration (UI-level; provide engine timing hints).
9. Coaching feedback mapper (metrics → structured tips).
10. Heavy cadence diagnostic flag (explicit engine config gate).

---

## 3. Detailed Task Breakdown

### 3.1 Finalize Stage

| Task | Description | Priority |
|------|-------------|----------|
| Segment Extraction | Choose best contiguous active segment (VAD + pitch stability) | P0 (BASIC ENERGY WINDOW IMPLEMENTED) |
| Refined DTW | Run DTW on extracted segment only | P0 (SCOPED TO SEGMENT FRAMES IMPLEMENTED) |
| Normalization Scalar | Compute user gain to match master RMS | P0 (USER RMS TRUE; MASTER RMS PROXY) |
| Store Final Metrics | similarityAtFinalize, segmentStartMs, segmentDurationMs | P0 (DONE) |
| API | Status finalizeSessionAnalysis(SessionId) | P0 (DONE) |

### 3.2 Similarity Readiness

| Task | Description | Priority |
|------|-------------|----------|
| Compute Minimum Frames | Derived from MFCC window/hop constants | P1 |
| Expose State Struct | framesObserved, minFramesRequired, usingRealtimePath, reliable | P1 |
| Update Tests | Convert skip → assert | P1 |

### 3.3 Loudness Enhancements

| Task | Description | Priority |
|------|-------------|----------|
| Capture Master Loudness | RMS/peak at load | P1 |
| User Rolling Loudness | Per segment RMS | P1 |
| Deviation Metric | (userRMS - masterRMS)/masterRMS | P1 |
| Summary Fields | loudnessDeviation, normalizationApplied | P1 |

### 3.4 Waveform Overlay

| Task | Description | Priority |
|------|-------------|----------|
| Unified Timeline Model | Master length reference; user progress mapping | P2 |
| Export Overlay Data | Downsampled user & master peak arrays | P2 |
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

## 4. API Additions (Planned Specifications)

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
```

EnhancedAnalysisSummary additions (post-implementation):
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
| Finalize Stage | finalizeSessionAnalysis returns OK; segment metrics non-zero; similarityAtFinalize stable (PARTIAL: energy heuristic working, refined DTW scoping pending) |
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
| finalizeSessionAnalysis | <40 ms typical (single segment) (PRELIM: full-session pass baseline) |
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
| PitchTracker vibrato detection (extent/rate/regularity) | Done |
| HarmonicAnalyzer kiss FFT optimization fallback (replaced naive DFT) | Done |

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
| FinalizeCreatesSegmentAndRefinedSimilarity | Validate finalize metrics population (ADDED: basic idempotency + metrics test) |
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

## 20. Diagnostics & Instrumentation (Active)

| Aspect | Detail | Gating |
|--------|--------|--------|
| Component Breakdown | offsetComponent, dtwComponent, meanComponent, subsequenceComponent added to SimilarityScoresSnapshot | Disabled when HUNTMASTER_DISABLE_DIAGNOSTIC_COMPONENTS defined |
| Finalize Fallback Flag | finalizeFallbackUsed true if finalize raised similarity from <0.70 to >=0.70 | Same diagnostics gating macro |
| Realtime Readiness | SimilarityRealtimeState (framesObserved, minFramesRequired, reliable, provisionalScore) | Always on (planned stable API) |
| Test Hook: Override Similarity | testOverrideLastSimilarity(SessionId,float) simulates low realtime similarity pre-finalize | Compiled only with HUNTMASTER_TEST_HOOKS=1 |
| Accessor: Fallback Used | getFinalizeFallbackUsed(SessionId) returns Result<bool> | Always available |

Implementation Notes:
- Test build propagates HUNTMASTER_TEST_HOOKS to library target to emit hook symbols.
- finalizeSessionAnalysis captures preFinalizeSimilarity then performs refinement; flag set only if threshold crossing occurs.
- Positive & negative tests validate fallback instrumentation behavior.
- Diagnostic fields appended after core scores to minimize ABI churn when disabled.

Planned Enhancements:
1. Timing counters for finalize sub-steps (segment selection, refined DTW) – gated.
2. Optional JSON diagnostic snapshot export – gated.
3. Per-component normalization weights exposure (read-only) – gated.

Exit Criteria:
- [ ] Fallback flag reliably set only on genuine improvements (≥3 scenarios)
- [ ] Readiness API removes similarity-related test skips
- [ ] Build passes with diagnostics disabled and enabled

---

## 21. Extended Capability Backlog (Future Phases)

High-level capability buckets planned post current phase stabilization. These are NOT in-scope now; captured to prevent idea loss & to clarify dependency gating.

| Capability Bucket | Objective | Key Features/Ideas | Dependencies / Gate | Risk If Early |
|-------------------|-----------|--------------------|---------------------|--------------|
| Pitch Mastery Suite | Deep pitch stability coaching | Micro-interval drift analysis, onset glide detection, sustained vibrato quality metric | Calibrated pitchGrade stability | Noise & false negatives in unstable baseline |
| Harmonic Richness Explorer | Tonal quality & resonance feedback | Formant energy balance, spectral centroid trend, roughness index | HarmonicAnalyzer v2 + normalized loudness | Misleading tonal advice pre-normalization |
| Cadence Precision Lab | Advanced rhythm pattern coaching | Adaptive metrical grid, swing / rubato detection, phrase boundary inference | Stable cadenceGrade & segment timing | Overfitting to noisy intervals |
| Fusion Similarity Enhancements | Multi-component similarity blending | Weighted MFCC+Pitch+Cadence fusion, adaptive weighting per call archetype | finalizeSessionAnalysis completeness & component reliability stats | Premature weight tuning yields regressions |
| Loudness Dynamics Coaching | Expressive envelope guidance | Attack/decay profiling, dynamic range score, clipping prediction | VolumeEnvelopeTracker + normalization scalar | Incorrect dynamics scoring without tracker |
| Adaptive Difficulty & Progression | Personalized progression system | Skill curve modeling, dynamic goal setting, streak & plateau detection | Stable grading (A–F) over >N sessions | Volatile goals create user churn |
| Master Call Library Intelligence | Smart content surfacing | Similar call recommendations, archetype clustering, metadata enrichment | Robust feature embeddings & index | Poor rec quality with immature embeddings |
| Cloud & Sync Layer | Cross-device continuity | Auth, secure session state sync, profile & progress storage | Finalized summary schema + security hardening | Schema churn & migration overhead |
| AI Coaching Semantics | Natural language guidance | LLM summarization of metrics, targeted improvement narratives | Deterministic metric stability & guardrails | Hallucinated or unstable advice |
| Mobile Performance Optimization | Battery & thermal aware tuning | Dynamic analyzer throttling, low-power FFT paths | Baseline performance telemetry & thresholds | Premature micro-optimizations obscure regressions |

Readiness Gates (must all be GREEN to begin Extended Buckets):

1. Current phase exit criteria fully met (Section 17).
2. All new metrics documented & frozen (no structural changes for 2 consecutive sprints).
3. Performance guard adherence verified under stress (multi-session load).
4. Calibration variance within target (<10% of band width drift across test corpus).


Sequencing Suggestion (Macro): Pitch Mastery → Cadence Precision Lab → Harmonic Richness Explorer → Fusion Similarity Enhancements → Loudness Dynamics → Adaptive Difficulty → Library Intelligence → Cloud & Sync → AI Coaching Semantics → Mobile Optimization.

## 22. Summary

Core engine & enhanced analyzers are stable. The next critical evolution is user-practice fidelity: segmentation, readiness clarity, loudness normalization, calibrated confidence → actionable coaching. This TODO enumerates the focused path to deliver a cohesive, low-latency, instruction-ready analysis loop without scope creep into later phase cloud or gamification tiers.
