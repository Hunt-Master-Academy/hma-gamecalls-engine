# Hunt Master Academy Game Calls Engine – Project Structure & Status Alignment
Last Updated: August 9, 2025
Status: Core MVP Production Ready | Enhanced Analyzers Phase 1 Integrated | Entering UX Alignment & Finalization

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
| Overlay Export | Data packaging util (likely `WaveformOverlayEngine.*`) | Pending (P2) |
| Calibration (Grades) | Lookup tables + extended summary fields | Pending (P1) |
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

Repository structure is stable, minimal, and roadmap-aligned. All forthcoming enhancements (finalization, readiness, overlay, calibration) are incremental and confined to `src/core` + public header augmentation—no reorganization required. Maintain synchronization discipline: update `mvp_todo.md` first, then reconcile this document
