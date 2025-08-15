# Hunt Master Academy Game Calls Engine – Project Structure & Status Alignmen## 3. Current Test Footprint

| Metric | Value | Notes |
|--------|-------|-------|
| Active test executables | 1 primary (RunEngineTests) + tool self-checks | Unified runner strategy |
| Distinct test source files | 107 organized into focused directories | Post-reorganization structure |
| Active test cases | 121 (all pass) | Comprehensive coverage across all components |
| Skipped cases | 0 | All similarity tests converted to readiness-based asserts |
| Performance guards | Enabled | Enforce <12 ms enhanced path |
| Test suite organization | Complete (archived) | Structured unit/integration/performance hierarchy |

**Test Directory Structure:**
- `tests/unit/core/` - Core engine components
- `tests/unit/analyzers/` - Enhanced analyzer components
- `tests/unit/vad/` - Voice Activity Detection
- `tests/unit/audio/` - Audio processing components
- `tests/unit/memory_security/` - Security and memory management
- `tests/unit/io_opt/` - Optimized I/O components
- `tests/unit/dtw/` - Dynamic Time Warping
- `tests/unit/infra/` - Infrastructure and utilities
- `tests/integration/` - Cross-component integration tests
- `tests/performance/` - Performance and benchmarkinged: August 14, 2025
Status: Core MVP Production Ready | Enhanced Analyzers Phase 1 Integrated | Test Suite Reorganized & Archived | Extended Capabilities Phase

> Authoritative roadmap & phase tracking: see `docs/mvp_todo.md` (single source of truth).
> This document reflects the CURRENT physical repository layout plus active vs archived assets.

---

## 1. Repository Top-Level Layout (Current Clean State)

```
huntmaster-engine/
├── CMakeLists.txt
├── LICENSE
├── README.md                        # (Root summary or redirect if present)
├── docs/
│   ├── README.md                    # Engine overview (synced 2025-08-09)
│   ├── mvp_todo.md                  # Authoritative roadmap / status
│   ├── architecture.md              # System & phase architecture
│   ├── PROJECT_STRUCTURE.md         # (This file)
│   ├── DEBUGGING.md
│   ├── DEPLOYMENT.md
│   ├── TESTING.md
│   └── (additional focused docs)
├── include/
│   └── huntmaster/
│       ├── core/                    # Public core engine headers
│       ├── security/                # Security / validation APIs
│       └── utils/                   # Shared utilities
├── src/
│   ├── core/                        # Engine implementations (UnifiedAudioEngine, analyzers, scoring)
│   ├── security/
│   └── utils/
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/
│   ├── integration/
│   ├── lib/                         # Test utilities (TestFixtureBase, generators)
│   └── benchmarks/
├── tools/                           # CLI & diagnostic tools (auto-discovered)
├── data/
│   ├── master_calls/                # Reference/master call audio (hierarchical species dirs)
│   ├── processed_calls/             # Normalized / prepared assets
│   ├── test_audio/
│   └── recordings/
├── web/                             # WASM bridge + alpha testing interface
├── scripts/                         # Automation (build, test orchestration, cleanup)
├── archive/                         # Historical reports / legacy docs (frozen)
└── cmake/                           # (If present) shared CMake modules
```

Legacy higher-volume / historical documents relocated under `archive/` to reduce noise. Only actively maintained design & roadmap docs remain in `docs/`.

---

## 2. Component Layer Mapping

| Layer | Directory | Key Files / Responsibilities | Status |
|-------|----------|------------------------------|--------|
| Orchestration | src/core | UnifiedAudioEngine.cpp | Production |
| Feature Extraction | src/core | MFCCProcessor.cpp | Production |
| Similarity (Realtime) | src/core | RealtimeScorer.cpp | Production |
| Similarity (Batch) | src/core | DTWComparator.cpp | Production |
| Streaming Gating | src/core | VoiceActivityDetector.cpp | Production |
| Levels / Loudness | src/core | AudioLevelProcessor.cpp | Production |
| Enhanced Analyzers | src/core | PitchTracker.cpp, HarmonicAnalyzer.cpp, CadenceAnalyzer.cpp | Integrated (Phase 1) |
| Summary Aggregation | src/core | Enhanced summary update logic | Active |
| Security / Hardening | src/security | Access control, validation | Production |
| CLI Tools | tools/ | interactive_recorder, debug_dtw_similarity, etc. | Operational |
| WASM Bridge | web/ | UnifiedWASMBridge.js, alpha UI | Operational (UI gaps) |
| Tests | tests/ | Unit + integration + performance | Passing (see skips note) |
| Docs | docs/ | Roadmap, architecture, deployment | Synced |
| Historical | archive/ | Phase reports, cleanup logs | Frozen |

---

## 3. Current Test Footprint

| Metric | Value | Notes |
|--------|-------|-------|
| Active test executables | 1 primary (RunEngineTests) + tool self-checks | Unified runner strategy |
| Distinct test source files | Reduced (lean set) | Post-clean consolidation |
| Active test cases | 19 (all pass) | Core + enhanced analyzer coverage |
| Skipped cases | 2 (master call realtime similarity readiness) | Will convert to asserts after readiness API |
| Performance guards | Enabled | Enforce <12 ms enhanced path |
| Sleep-based tests | 1 (stale invalidation) | Target: replace with virtual clock |

(Older counts in archived docs referencing “80 test files / 130+ tests” are historical and intentionally superseded.)

---

## 4. Directory Hygiene & Ownership

| Directory | Owner (Role) | Maintenance Notes |
|-----------|--------------|-------------------|
| src/core | Engine team | API stability locked except planned finalize additions |
| include/huntmaster | Public interface | Add new fields only with mvp_todo update |
| tests | QA / Engine | No unchecked Result<T> usages (policy) |
| tools | Dev Tooling | Auto-add via CMake glob—keep lean & documented |
| data/master_calls | Content / Audio | Ensure licensing & version control (Git LFS optional) |
| web | Frontend / Bridge | Overlay + countdown features pending |
| docs | All contributors | mvp_todo drives changes; avoid doc drift |
| archive | Read-only | Never modify (append only if archiving new deprecations) |

---

## 5. Build & Configuration Overview

| Aspect | Implementation |
|--------|----------------|
| Build System | Modular CMake (root + src + tests + tools) |
| Options | HUNTMASTER_BUILD_TESTS, HUNTMASTER_BUILD_TOOLS |
| Language Standard | C++20 |
| Dependency Strategy | Minimal (FFT / GTest / Benchmark if enabled) |
| WASM Path | Emscripten build using same core sources |
| Code Style Core Rules | Result<T> no exceptions, spans for audio, session isolation |

---

## 6. Phase Alignment Snapshot (Structural Impact)

| Feature Group | Structural Impact | Status |
|---------------|-------------------|--------|
| Finalize Session Stage | New API + added summary state | Pending (P0) |
| Similarity Readiness Introspection | Additional header & summary hook | Pending (P1) |
| Loudness Normalization | RMS baseline & scalar integration | Pending (P1) |
| Overlay Export | Data packaging util (likely `WaveformOverlayEngine.*`) | Base done (P2) |
| Calibration (Grades) | Lookup tables + extended summary fields | In progress (P1) |
## Feature & Tool Inventory (Engine-Focused)

The following table summarizes major engine features/tools, their purpose, status, interfaces, and operational constraints. Treat `docs/mvp_todo.md` as authoritative for statuses.

| Name | Purpose | Status | Key APIs | Inputs → Outputs | Preconditions | Build flags/Variants (WASM) | Perf budget | Error modes (examples) | JSON export | Thread-safety | Tests (refs) | Dependencies | Notes/Limits |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| UnifiedAudioEngine | Session orchestration and pipeline entry | Implemented | createSession, destroySession, loadMasterCall, processAudioChunk | Audio chunks → internal features, scores | Valid SessionId; master loaded for compare | HUNTMASTER_TEST_HOOKS (tests), WASM parity | <12 ms/hop | INVALID_SESSION, BAD_CONFIG | N/A | Thread-isolated per session | Unit: engine status/error; Integration: end-to-end | Core subsystems | Always check Result/Status |
| Readiness API | Gate realtime outputs until data sufficient | Implemented | getRealtimeSimilarityState | Frames observed → ready state | Session active; some audio processed | WASM parity | O(1) | NOT_READY | N/A | Lock-free | Unit: readiness gating | AudioLevelProcessor | No sleeps; poll-ready only |
| Finalize stage | Refined DTW, loudness normalization, segments | Implemented | finalizeSessionAnalysis | Buffered audio → FinalizedSummary | All audio streamed | WASM parity | <40 ms typical | INSUFFICIENT_DATA, IO_ERROR | Yes (summary) | Single-threaded per session | Unit/Integration: finalize success/error | DTW, Loudness | Call once per recording |
| MFCCProcessor | Spectral features (MFCC frames) | Implemented | (internal) | Frames → MFCC coeffs | Engine configured | WASM parity | Per hop <3 ms | BAD_CONFIG | Included in enhanced summaries | Session-local | Unit: MFCC consistency | FFT/windowing | Tuned for 44.1 kHz |
| PitchTracker | F0 trace | Implemented (streaming) | enableEnhancedAnalyzers, getEnhancedSummary | Audio → pitch series | Enhanced analyzers enabled | WASM parity | Per hop <2 ms | INSUFFICIENT_DATA | In enhanced JSON | Session-local | Unit: pitch contour | MFCC/windowing | Deterministic |
| HarmonicAnalyzer | Tonal quality | Implemented (streaming) | enableEnhancedAnalyzers, getEnhancedSummary | Audio → harmonic metrics | Enhanced analyzers enabled | WASM parity | <2 ms | INSUFFICIENT_DATA | In enhanced JSON | Session-local | Unit: harmonic metrics | FFT | Combines with pitch |
| CadenceAnalyzer | Rhythm/tempo metrics | Implemented (streaming) | enableEnhancedAnalyzers, getEnhancedSummary | Audio → cadence features | Enhanced analyzers enabled | WASM parity | <2 ms | INSUFFICIENT_DATA | In enhanced JSON | Session-local | Unit: cadence | VAD/windowing | Affects DTW inputs |
| VoiceActivityDetector (VAD) | Activity gating | Implemented | setVADConfig (if exposed) | Audio → activity mask | Engine configured | WASM parity | <1 ms | BAD_CONFIG | Optional | Session-local | Unit: VAD edges | Energy thresholds | Reduces false positives |
| AudioLevelProcessor | Level tracking (RMS/peak) | Implemented | (internal) | Audio → RMS/peak | None | WASM parity | <0.5 ms | N/A | In summaries | Session-local | Unit: levels | None | Powers loudness metrics |
| Loudness metrics | normalizationScalar, loudnessDeviation | Implemented | getEnhancedSummary | Features → loudness metrics | Some audio processed | WASM parity | O(1) | INSUFFICIENT_DATA | Yes | Session-local | Unit: loudness | Levels | Inputs for grades/coaching |
| DTWComparator/Processor | Temporal alignment | Implemented | (internal) | Feature sequences → distance/path | Feature frames present | WASM parity | Bounded by frames | BAD_CONFIG | Optional (debug) | Session-local | Unit: DTW edges/window | MFCC/cadence | Window constraints supported |
| RealtimeScorer | Weighted similarity | Implemented | getSimilarityScore | Feature metrics → score | Ready state true | WASM parity | O(1) | NOT_READY | In summaries | Session-local | Unit: scorer | DTW, analyzers | Deterministic, no ML |
| Enhanced summary | Aggregated analyzer outputs | Implemented | getEnhancedSummary | Current analysis → JSON-like struct | Some audio processed | WASM parity | O(1) | INSUFFICIENT_DATA | Yes | Session-local | Unit: summaries | All analyzers | Backward-compatible fields |
| Waveform overlay export | UI peaks/energy | Implemented (base) | exportOverlayData | Buffers → decimated peaks | Enough frames present | WASM parity (planned) | O(n/decim) | INSUFFICIENT_DATA | Yes (planned) | Session-local | Unit: overlay export | Levels | Offset control pending |
| Calibration grades (A–F) | Map metrics → grades | Implemented | (extended summaries) | Metrics → grades | After analysis | WASM parity | O(1) | INSUFFICIENT_DATA | Yes | Session-local | Unit: grades | Scorer | Used by coaching mapper |
| Coaching feedback | Rule-based guidance | Planned | (future getCoachingFeedback) | Grades+loudness → tips | After grades | WASM: rules only | O(1) | N/A | Yes | Session-local | Unit: coaching (planned) | Grades | Deterministic, gated |
| Mic Calibration Advisor | Gain/noise-floor recommendation | Planned | getCalibrationSummary | Audio window → recommendation | Short capture done | Flag-free; optional | O(n) on window | INSUFFICIENT_DATA | Yes | Session-local | Unit: mic calib (planned) | Levels | No device I/O in tests |
| Latency/Drift Calibrator | Offset/ppm drift | Planned | begin/submit/finalizeLatencyDriftCalibration | Impulse(s) → report | Calibration started | Optional feature | O(n) once | INSUFFICIENT_DATA | Yes | Session-local | Unit: drift calib (planned) | DTW | Adjusts DTW band |
| Environment Profiler (NR opt) | Ambient profile, VAD-aware norm | Planned | captureAmbientProfile, setNoiseSuppressionEnabled | Ambient audio → profile | Capture done; NR flag off by default | RNNOISE enabled? (opt) | O(n) once | UNSUPPORTED (if NR off) | Yes | Session-local | Unit: env profile (planned) | VAD | RNNoise behind flags |
| “Why” breakdown | Per-metric contributions | Planned | (extend scorer outputs) | Contributions → total | Score computed | N/A | O(1) | N/A | Yes | Session-local | Unit: contributions (planned) | Scorer | Sum within epsilon |
| OptimizedAudioIO | IO utilities (buffers/writer) | Implemented (tools) | StreamingAudioBuffer, AsyncAudioWriter (internal) | Frames ↔ buffers/files | None | N/A | O(1) ops; writer async | IO_ERROR | N/A | Internal-sync | Unit: IO buffer/writer | dr_wav | Not public API |
| OptimizedAudioRecorder | Memory/file/hybrid capture | Implemented (tools) | (internal; test hooks) | Audio → memory/file buffers | Config set | N/A | O(1) per write; flush O(n) | IO_ERROR | N/A | Internal-sync | Unit: recorder | IO utils | Device-less tests via hooks |
| AudioFormatConverter | PCM/float conversions | Implemented | (internal) | PCM16 ↔ float32 | None | WASM parity | O(n) | BAD_FORMAT | N/A | Thread-safe | Unit: converter | None | Used in ingest |
| SpectrogramProcessor | Visualization support | Implemented | (internal/optional) | Audio → spectrogram | Enough frames | WASM parity | O(n log n) batched | INSUFFICIENT_DATA | Optional | Session-local | Unit: spectrogram | FFT | For UI tooling |
| WaveformGenerator | Synthetic signals | Implemented (tool) | (tests/tools) | Params → samples | None | WASM parity | O(n) | BAD_CONFIG | N/A | Thread-safe | Unit: generator | None | Test helpers |

| Coaching Mapper | New module (non-blocking) | Pending (P2) |

No structural rearrangement required—additions are additive within existing directories.

---

## 7. Summary of Active vs Planned Engine Public Fields

| Area | Current | Planned Additions |
|------|---------|-------------------|
| EnhancedAnalysisSummary | pitchHz, pitchConfidence, cadenceScore, harmonicityScore, tempoConfidence, harmonicConfidence, lastUpdateMs, sessionFrames | similarityAtFinalize, segmentStartMs, segmentDurationMs, loudnessDeviation, normalizationScalar, pitchGrade, harmonicGrade, cadenceGrade |
| APIs | createSession, destroySession, loadMasterCall, processAudioChunk, getSimilarityScore, getEnhancedSummary, enableEnhancedAnalyzers, resetSession | finalizeSessionAnalysis, getRealtimeSimilarityState, exportOverlayData, setOverlayOffset |

(All planned APIs must be reflected in `mvp_todo.md` before header changes.)

---

## 8. Documentation Governance

| Rule | Enforcement |
|------|-------------|
| Single authoritative roadmap | `docs/mvp_todo.md` |
| No new doc proliferation | Add only if value > existing sections |
| Sync cycle | Update mvp_todo → update related docs (README, architecture, structure) |
| Archive policy | Move superseded docs; never silently delete history |
| Version clarity | Each major doc carries “Last Updated” timestamp |

---

## 9. Cleanup & Drift Control

| Risk | Mitigation |
|------|------------|
| Divergent status counts | Always reconcile with mvp_todo before committing |
| Orphaned tools | CMake glob review; prune obsolete binaries |
| Data bloat | Prefer Git LFS for large future master calls |
| Silent API creep | Require mvp_todo diff in PR description |

---

## 10. Quick Commands

```bash
# Configure & build (Release)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests (guarded)
timeout 60 ./build/bin/RunEngineTests

# Focus master call integration tests
timeout 60 ./build/bin/RunEngineTests --gtest_filter="*MasterCallsComparison*"

# WASM (example)
emcmake cmake -B build-wasm
cmake --build build-wasm
```

---

## 11. Pending Structural Additions (Non-Disruptive)

| Planned File | Purpose | Notes |
|--------------|---------|-------|
| src/core/SessionFinalizer.cpp | finalizeSessionAnalysis logic | P0 |
| src/core/SimilarityReadiness.cpp | readiness state computation | P1 |
| src/core/WaveformOverlayEngine.cpp | Overlay exports (decimation) | P2 |
| include/.../SessionFinalizer.h | Public finalize API exposure | After stable internal draft |
| include/.../SimilarityReadiness.h | State struct | Mirror mvp_todo spec |
| include/.../OverlayExport.h | Export structs | Keep POD & cheap |
| src/core/CalibrationAdvisor.cpp | Mic gain/noise floor advisor (analysis-only) | Planned; default-off diagnostics |
| src/core/LatencyDriftCalibrator.cpp | One-shot latency/drift measurement | Planned; off hot path |
| src/core/EnvironmentProfiler.cpp | Ambient profile + optional NR gate | Planned; optional, guarded by flags |
| include/.../CalibrationAdvisor.h | Public summary fields/accessors | Add only after mvp_todo update |
| include/.../LatencyDriftCalibrator.h | Public calibrator API | Add only after mvp_todo update |
| include/.../EnvironmentProfiler.h | Profiler controls & report structs | Optional; JSON export friendly |

---

## 12. Deprecations & Gated Paths

| Item | Status | Action |
|------|--------|--------|
| Heavy cadence diagnostics | Gated | Keep behind config flag |
| Legacy placeholder pitch confidence | Removed | None |
| Advanced harmonic placeholders | Zeroed / dormant | Populate post calibration |
| Sleep-based stale test | Temporary | Replace with virtual clock abstraction |

---

## 13. Exit Criteria (Current Phase – Structural Impact)

All of the following will require updating this file’s sections 7, 11, and 12:

1. finalizeSessionAnalysis implemented & headers published.
2. Similarity readiness API added & test skips removed.
3. Loudness normalization fields appended to summary.
4. Calibration grades integrated & documented.
5. Overlay export module merged.
6. Virtual clock refactor (remove sleep reference).

---

## 14. Change Log (Recent)

| Date | Change |
|------|--------|
| 2025-08-09 | Updated counts (tests, analyzers), added planned structural modules, aligned with new roadmap |
| 2025-08-02 | (Archived) Prior structure doc snapshot (see archive/) |

---

## 15. Summary

Repository structure is stable, minimal, and roadmap-aligned. All forthcoming enhancements (finalization, readiness, overlay, calibration) are incremental and confined to `src/core` + public header augmentation—no reorganization required. Maintain synchronization discipline: update `mvp_todo.md` first, then reconcile this document.
