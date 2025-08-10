# Hunt Master Academy – Game Calls Engine Documentation Index

Last Updated: August 9, 2025
Authoritative Roadmap: [mvp_todo.md](mvp_todo.md) (update there first)
Scope: This /docs index covers ONLY the Game Calls Engine (Unified Audio Engine core). Platform‑wide (other pillars) features are informational only in the root README.

---

## 1. Purpose
Central navigation hub for engine documentation. The root repository README provides product/platform context; this index focuses on engineering references & implementation status.

---

## 2. Status Snapshot (Sync with mvp_todo.md)
Phase: Enhanced Platform Integration & UX Alignment
Core Pipeline: Production stable (sessions, MFCC, realtime + DTW similarity)
Enhanced Analyzers: Pitch / Harmonic / Cadence integrated (Phase 1)
Immediate Focus: finalizeSessionAnalysis, similarity readiness API, loudness normalization, overlay export, calibration grades, coaching mapper

---

## 3. Quick Links
| Area | Document |
|------|----------|
| Roadmap / Tasks | [mvp_todo.md](mvp_todo.md) |
| Architecture & Flow | [architecture.md](architecture.md) |
| Project Layout | [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) |
| Testing Strategy | [TESTING.md](TESTING.md) |
| Debug & Instrumentation | [DEBUGGING.md](DEBUGGING.md) |
| Deployment & Packaging | [DEPLOYMENT.md](DEPLOYMENT.md) |
| Root Platform Context | [../README.md](../README.md) |

---

## 4. Core Concepts (Pointers)
- Session Lifecycle & APIs: architecture.md (sections 3–6)
- Realtime vs DTW Similarity: architecture.md (Similarity Architecture)
- EnhancedAnalysisSummary Fields: architecture.md (Enhanced Analysis Summary)
- Planned Additions (finalize, readiness, overlay): mvp_todo.md sections 2–4

---

## 5. Active Work (High-Level)
| Workstream | Goal | Doc Source |
|------------|------|------------|
| Finalize Stage | Segment extraction + refined DTW + loudness normalization | mvp_todo.md 3.1 |
| Readiness API | Deterministic realtime similarity availability | mvp_todo.md 3.2 |
| Loudness Metrics | Deviation + normalization scalar | mvp_todo.md 3.3 |
| Overlay Export | Master/user waveform peak arrays | mvp_todo.md 3.4 |
| Calibration | Map confidences → graded bands | mvp_todo.md 3.5 |
| Coaching Mapper | Metric → structured feedback | mvp_todo.md 3.7 |
| Virtual Clock | Remove sleep-based tests | mvp_todo.md 3.6 / TESTING.md |

---

## 6. Update Order (Governance)
1. Modify feature/task → update mvp_todo.md.
2. Adjust architecture.md if structural or pipeline changes.
3. Update PROJECT_STRUCTURE.md for new public files/APIs.
4. Reflect testing impacts in TESTING.md.
5. (Optional) Adjust root README if user-facing narrative shifts.

No direct edits here that conflict with mvp_todo.md — this file summarizes.

---

## 7. Naming & Conventions
- Primary entry docs in uppercase (README.md, LICENSE); technical references may remain lowercase (architecture.md).
- Avoid adding new top-level docs without explicit roadmap value.
- All new public API fields: document in mvp_todo.md first.

---

## 8. Performance Targets (Reference)
| Path | Target |
|------|--------|
| Streaming enhanced chunk | <12 ms |
| finalizeSessionAnalysis (one-shot) | <40 ms |
| Memory / session (post-finalize) | <15 MB |

Guard assertions live in tests; adjust only with justification.

---

## 9. Pending API Additions (Planned)
(Do not expose until implemented)
- finalizeSessionAnalysis(SessionId)
- getRealtimeSimilarityState(SessionId)
- exportOverlayData(SessionId, decimation)
- Added summary fields: similarityAtFinalize, segmentStartMs, segmentDurationMs, loudnessDeviation, normalizationScalar, pitchGrade, harmonicGrade, cadenceGrade

---

## 10. Skips & Debt (To Eliminate)
| Item | Status | Removal Trigger |
|------|--------|-----------------|
| Similarity readiness test skips | Present (2) | Readiness API implemented |
| Sleep-based stale test | Present | Virtual clock |
| Uncalibrated confidence grades | Present | Calibration tables |

---

## 11. Contributing Reminder
- Always check Result<T>/Status; no unchecked `.value`.
- No new blocking sleeps; prepare for virtual clock.
- Keep heavy diagnostics (cadence deep path) disabled by default.
- Provide tests + performance guard for each new analyzer or finalize feature.

---

## 12. External Pillars (Informational)
Other Academy pillars (Strategy, Stealth, Tracking, Marksmanship) are NOT implemented here. Do not introduce cross-pillar code until Game Calls exit criteria are met (see mvp_todo.md “Exit Criteria”).

---

## 13. Change Log (This Index)
| Date | Change |
|------|--------|
| 2025-08-09 | Converted docs/README.md to structured index (removed prior redirect) |

---

## 14. Summary
This index centralizes navigation for the Game Calls Engine. Treat mvp_todo.md as authoritative; all statuses here are derivative. Keep scope disciplined: finalize reliability → visualization & calibration → coaching →
