# Hunt Master Academy – Game Calls Engine Documentation Index

Last Updated: August 14, 2025
Authoritative Roadmap: [mvp_todo.md](mvp_todo.md) (update there first)
Scope: This /docs index covers ONLY the Game Calls Engine (Unified Audio Engine core). Platform‑wide (other pillars) features are informational only in the root README.

---

## 1. Purpose
Central navigation hub for engine documentation. The root repository README provides product/platform context; this index focuses on engineering references & implementation status.

---

## 2. Status Snapshot (Sync with mvp_todo.md)
Phase: Extended Capabilities Development (Pitch Mastery Suite)
Core Pipeline: Production stable (sessions, MFCC, realtime + DTW similarity)
Enhanced Analyzers: Pitch / Harmonic / Cadence integrated (Phase 1 COMPLETE)
Test Suite: 121 tests passing, reorganized structure archived
Immediate Focus: Pitch mastery suite implementation (micro-drift, onset glide, vibrato quality)

---

## 3. Quick Links
| Area | Document |
|------|----------|
| Roadmap / Tasks | [mvp_todo.md](mvp_todo.md) |
| Extended Capabilities | [extended_capabilities_roadmap.md](extended_capabilities_roadmap.md) |
| Architecture & Flow | [architecture.md](architecture.md) |
| Project Layout | [project_structure.md](project_structure.md) |
| Testing Strategy | [testing.md](testing.md) |
| Debug & Instrumentation | [debugging.md](debugging.md) |
| Deployment & Packaging | [deployment.md](deployment.md) |
| Root Platform Context | [../README.md](../README.md) |

---

## 4. Core Concepts (Pointers)
- Session Lifecycle & APIs: architecture.md (sections 3–6)
- Realtime vs DTW Similarity: architecture.md (Similarity Architecture)
- EnhancedAnalysisSummary Fields: architecture.md (Enhanced Analysis Summary)
- Recently Added (finalize, readiness, overlay): see mvp_todo.md sections 2–4

---

## 5. Active Work (High-Level)
| Workstream | Goal | Doc Source |
|------------|------|------------|
| Pitch Mastery Suite | Micro-drift analysis, onset glide detection, vibrato quality metrics | extended_capabilities_roadmap.md Phase 1 |
| Test Framework Enhancement | Support for extended capability testing patterns | testing.md |
| Performance Optimization | Maintain <12ms streaming path with new features | mvp_todo.md 7 |
| Documentation Alignment | Update all docs for extended capabilities phase | extended_capabilities_roadmap.md |

### Recently Completed (Archived):
| Workstream | Goal | Status |
|------------|------|--------|
| Finalize Stage | Segment extraction + refined DTW + loudness normalization | COMPLETE |
| Readiness API | Deterministic realtime similarity availability | COMPLETE |
| Test Suite Organization | Comprehensive test restructuring and cleanup | COMPLETE (archived) |
| Calibration Framework | Confidence grades (A–F) mapping infrastructure | COMPLETE |
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

## 9. Recent API Additions (Implemented)
- finalizeSessionAnalysis(SessionId)
- getRealtimeSimilarityState(SessionId)
- exportOverlayData(SessionId, decimation)
- Enhanced summary fields: similarityAtFinalize, segmentStartMs, segmentDurationMs, loudnessDeviation, normalizationScalar

## 9.1 Upcoming APIs (Planned)
- setOverlayOffset(SessionId, ms)
- Calibration grades exposure (pitchGrade, harmonicGrade, cadenceGrade) where not already surfaced in summaries
- Calibration advisors: MicCalibrationSummary, LatencyDriftReport, EnvironmentProfile (guarded)

---

## 9.2 Coaching Feedback API (Planned)

- getCoachingFeedback(SessionId): returns a simple list of actionable tips derived from EnhancedAnalysisSummary (grades + loudnessDeviation).
- exportCoachingFeedbackToJson(SessionId): returns a JSON string: { "suggestions": ["..."] }.

Status: Design complete; enable only after grade tables stabilized.

---

## 10. Skips & Debt (Current)

| Item | Status | Removal Trigger |
|------|--------|-----------------|
| Similarity readiness test skips | None | — |
| Sleep-based stale test | None (virtual clock abstraction in tests) | — |
| Uncalibrated confidence grades | None (A–F mapping active) | — |

---

## 11. Contributing Reminder

- Always check Result&lt;T&gt;/Status; no unchecked `.value`.
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
| 2025-08-13 | Synced statuses (finalize/readiness/loudness/overlay), fixed links, clarified pending APIs |
| 2025-08-09 | Converted docs/README.md to structured index (removed prior redirect) |

---

## 14. Summary
This index centralizes navigation for the Game Calls Engine. Treat mvp_todo.md as authoritative; all statuses here are derivative. Keep scope disciplined: finalize reliability → visualization & calibration → coaching →
