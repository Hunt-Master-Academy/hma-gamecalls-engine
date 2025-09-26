# Hunt Master Academy Game Calls Engine Debugging Guide
Last Updated: August 9, 2025
Status Context: Core Engine Production | Enhanced Analyzers Phase 1 Integrated | Entering UX Alignment & Finalization (see `docs/mvp_todo.md`)

---

## 1. Purpose
This guide describes the runtime debugging, tracing, and performance instrumentation facilities for the Huntmaster Audio Engine. It is aligned with the current roadmap (finalize session stage, similarity readiness API, loudness normalization, waveform overlay export, calibration). Always reconcile changes with `docs/mvp_todo.md` before modifying this file. Finalize, readiness, loudness, and base overlay export are implemented.

---

## 2. Debugging Architecture Overview
Core elements:
- DebugLogger (singleton; thread-safe; component + global log levels)
- PerformanceMonitor / AutoProfiler (RAII timing of critical blocks)
- Component-scoped log filtering (avoid noise in hot paths)
- Optional file logging (rotatable)
- Gated heavy diagnostic paths (e.g., cadence deep diagnostics disabled by default)

Processing order for logs (per chunk):
1. VoiceActivityDetector (minimal logs unless TRACE)
2. AudioLevelProcessor
3. MFCCProcessor
4. RealtimeScorer
5. Enhanced analyzers (PitchTracker, HarmonicAnalyzer, CadenceAnalyzer) – lazy created
6. Summary aggregation & smoothing
7. (Planned) finalizeSessionAnalysis – invoked post-stop only, not per chunk

---

## 3. Log Levels
Ordered by verbosity (NONE < ERROR < WARN < INFO < DEBUG < TRACE). Recommendations:
- Production baseline: ERROR
- Staging / QA: WARN or INFO for select components
- Deep performance triage: DEBUG for PERFORMANCE component only
- Detailed algorithm exploration: TRACE but on a single component at a time

---

## 4. Components (Current Map)

| Component Enum | Scope |
|----------------|-------|
| GENERAL | Generic / uncategorized |
| UNIFIED_ENGINE | High-level orchestration |
| MFCC_PROCESSOR | Frame & coefficient extraction |
| DTW_COMPARATOR | Batch similarity operations |
| REALTIME_SCORER | Incremental similarity blending |
| VAD | Voice activity gating decisions |
| AUDIO_LEVEL_PROCESSOR | RMS / peak tracking |
| PITCH_TRACKER | YIN pitch detection |
| HARMONIC_ANALYZER | Spectral harmonicity metrics |
| CADENCE_ANALYZER | Inter-onset timing & cadence score |
| ENHANCED_SUMMARY | Summary aggregation & smoothing |
| PERFORMANCE | Timing & profiling |
| TOOLS | CLI / auxiliary utilities |
| OVERLAY_EXPORT (Planned) | Waveform downsampling / peak packing |
| FINALIZE_STAGE (Planned) | Segment extraction + refined DTW |
| CALIBRATION (Planned) | Confidence grade mapping |
| CALIBRATION_LVL (Planned) | Mic gain/noise floor advisor |
| CALIBRATION_LATENCY (Planned) | Latency/drift calibration |
| ENVIRONMENT_PROFILER (Planned) | Ambient profiling / NR flag |

(Older names like AUDIO_ENGINE / FEATURE_EXTRACTION may remain in code until a consolidation pass; keep mappings consistent.)

---

## 5. Quick Usage Examples

### 5.1 Configure Global + Component Log Levels

```cpp
auto& logger = huntmaster::DebugLogger::getInstance();
logger.setGlobalLogLevel(huntmaster::LogLevel::ERROR); // baseline

// Temporarily elevate specific components
logger.setComponentLogLevel(huntmaster::Component::UNIFIED_ENGINE,
 huntmaster::LogLevel::INFO);
logger.setComponentLogLevel(huntmaster::Component::PERFORMANCE,
 huntmaster::LogLevel::DEBUG);
```cpp

### 5.2 File Logging

```cpp
logger.enableFileLogging("debug_session.log");
logger.enableTimestamps(true);
logger.enableThreadIds(true);

```

### 5.3 Scoped Performance

```cpp
{
 AutoProfiler p("processAudioChunk");
 engine->processAudioChunk(session, span);
}
```

---

## 6. Best Practices

| Scenario | Recommended Approach |
|----------|----------------------|
| High-volume streaming debug | Use INFO selectively; avoid TRACE on MFCC & pitch simultaneously |
| Latency investigation | Enable PERFORMANCE + AutoProfiler / summary timing |
| Similarity readiness tuning | (After API added) add DEBUG on REALTIME_SCORER & readiness |
| Analyzer correctness | Enable one analyzer at TRACE at a time |
| Post-stop final analysis | (Planned) Enable FINALIZE_STAGE TRACE only during development |
| Cadence deep analysis | Keep heavy diagnostics gated (do not enable by default) |

---

## 7. Avoiding Performance Regressions

- Never leave TRACE logs active in benchmarks.
- Wrap expensive formatting inside level checks if outside logger’s guard.
- Do not log per-sample data; log per-frame or per-chunk summaries.
- Heavy cadence diagnostics remain behind an explicit config/flag—verify disabled in CI.

---

## 8. Upcoming Debug Hooks (Planned)

| Feature | Debug Signals |
|---------|---------------|
| finalizeSessionAnalysis | Segment boundaries chosen, RMS stats, DTW refinement time |
| Similarity readiness API | framesObserved, minFramesRequired, reliability transitions (implemented) |
| Loudness normalization | masterRMS, userSegmentRMS, normalizationScalar, clipping guard |
| Overlay export | Peak block size, downsample ratio, alignmentOffsetMs (offset control pending) |
| Calibration mapping | Raw confidence → grade transitions with thresholds |
| Mic calibration | peakLevelDbFS, rmsDbFS, noiseFloorDbFS, headroomDb, recommendation |
| Latency/drift calibration | alignmentOffsetMs, driftPpm, compensation applied to DTW band |
| Environment profiler | ambient profile metrics, NR path enabled=false/true |

Add logging only after minimal overhead validated.

---

## 9. Troubleshooting Matrix

| Symptom | Likely Cause | Debug Action |
|---------|--------------|--------------|
| Similarity score stays low / unavailable | Insufficient frames / readiness threshold not reached | Enable REALTIME_SCORER DEBUG; inspect readiness state (planned) |
| Pitch flickers rapidly | Analyzer smoothing disabled or high noise | Enable PITCH_TRACKER TRACE (single chunk) + verify EMA config |
| Cadence score zero | No detected onsets (VAD gating) | VAD INFO + CADENCE_ANALYZER DEBUG |
| High latency spikes | Unintended diagnostic path enabled | PERFORMANCE DEBUG & confirm heavy cadence disabled |
| Summary not updating | Enhanced analyzers disabled or stale invalidation triggered | UNIFIED_ENGINE INFO + ENHANCED_SUMMARY DEBUG |

---

## 10. Conditional / Scoped Logging Patterns

```cpp
if (logger.isEnabled(huntmaster::Component::REALTIME_SCORER,
 huntmaster::LogLevel::DEBUG)) {
 logger.log(huntmaster::Component::REALTIME_SCORER,
 huntmaster::LogLevel::DEBUG,
 "Frames=" + std::to_string(state.framesObserved));
}
```

(Use helper `isEnabled` pattern to avoid constructing large strings unnecessarily.)

---

## 11. Gated Diagnostics Flags (Recommended Conventions)
(Define environment or config usage; implement progressively)

| Flag | Purpose | Default |
|------|---------|---------|
| HUNTMASTER_ENABLE_HEAVY_CADENCE | Deep cadence spectral / IOI clustering detail | OFF |
| HUNTMASTER_ENABLE_OVERLAY_TRACE (future) | Overlay decimation debug | OFF |
| HUNTMASTER_LOG_LEVEL | Override global level (ERROR/INFO/DEBUG/TRACE) | ERROR |
| HUNTMASTER_PROFILING | Enable AutoProfiler blocks globally | OFF |

Sample bootstrap:

```cpp
if (const char* lvl = std::getenv("HUNTMASTER_LOG_LEVEL")) {
 logger.setGlobalLogLevel(parseLogLevel(lvl));
}
```

---

## 12. Integration with Tools

Example CLI (interactive_recorder):

```bash
./interactive_recorder --debug --engine-debug --performance
./interactive_recorder --trace --pitch-debug
```
Ensure each new tool maps CLI flags → component levels without duplicating logic.

---

## 13. Testing & Debugging

- Do not assert on debug log content in standard tests (flaky).
- Add targeted debug-mode tests only if they validate structural invariants (e.g., readiness transition).
- Use environment variable toggles in CI only for specialized debug workflows (never default).

---

## 14. Memory & Resource Verification

Combine valgrind / sanitizers with minimal logging:

```bash
LOG_LEVEL=ERROR timeout 60 valgrind --leak-check=full ./build/bin/RunEngineTests
```
Avoid TRACE under valgrind (excess noise, slowdown).

---

## 15. Planned Additions Checklist (Update After Implementation)

| Item | Added | Notes |
|------|-------|-------|
| FINALIZE_STAGE component | [ ] | After finalizeSessionAnalysis implemented |
| Readiness state logging helper | [ ] | Tied to SimilarityRealtimeState API |
| Loudness normalization debug lines | [ ] | Log only scalar + bounded correction |
| Calibration grade transition logs | [ ] | Single line per metric grade change |
| Overlay decimation stats | [ ] | One line on export, not per frame |

---

## 16. Common Pitfalls

| Pitfall | Impact | Fix |
|---------|--------|-----|
| TRACE everywhere | Massive latency | Restrict & recompile |
| Logging inside tight inner loops | Dropped real-time guarantees | Move outside loops |
| Large string concatenations pre-check | Unneeded CPU | Guard with isEnabled() |
| Leaving heavy diagnostics on in CI | Failing perf tests | Enforce CI log-level policy |

---

## 17. Minimal Production Template

```cpp
auto& logger = huntmaster::DebugLogger::getInstance();
logger.setGlobalLogLevel(huntmaster::LogLevel::ERROR);
logger.enableTimestamps(true);
// Optional: file logging only when incident debugging
```

---

## 18. Update Policy

Any new component or analyzer:

1. Add component enum
2. Provide targeted log messages at INFO (state transitions) only
3. Avoid TRACE until profiling impact measured
4. Document in this file + `mvp_todo.md`

---

## 19. Summary

The debugging system balances observability with real-time performance. Maintain discipline: minimal logs in critical loops, targeted component activation, readiness & finalize instrumentation added only when implemented, and all evolutions tracked via `docs/mvp_todo.md`.

---
