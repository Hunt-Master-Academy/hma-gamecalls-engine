# Hunt Master Academy Game Calls Audio Engine – Testing & Validation Guide
Last Updated: August 14, 2025
Status Context: Core Engine Production | Enhanced Analyzers Phase 1 Integrated | Test Suite Reorganized & Archived | Extended Capabilities Phase
Authoritative roadmap: see [docs/mvp_todo.md](mvp_todo.md)

---

## 1. Purpose
Defines the canonical testing strategy (unit, integration, performance, future finalize/overlay/calibration suites) and required practices (Result<T> handling, latency guards, asset use). This file must stay consistent with [docs/mvp_todo.md](mvp_todo.md). Update the MVP TODO first, then revise this file.

---

## 2. Testing Principles
| Principle | Description |
|-----------|-------------|
| Determinism | No data‑dependent randomness without fixed seed |
| Fast Feedback | Majority of tests complete < 10s total |
| Zero Silent Failures | All Result<T> / Status checked; no ignored errors |
| No Long Sleeps | Virtual clock abstraction replaces real waits |
| Asset Isolation | Only sanctioned audio assets in data/master_calls & data/processed_calls |
| No Permanent Skips | 0 skips; readiness API drives assertions |
| Perf Guarded | Latency thresholds asserted for critical paths |
| Idempotent | Re-running tests yields identical outcomes |
| Minimal Logging | Tests run at ERROR log level unless debugging |

---

## 3. Current Test Inventory (Snapshot)
| Category | Examples | Status |
|----------|----------|--------|
| Unit – Core | MFCCProcessor, DTWComparator, VoiceActivityDetector | Passing (121 tests) |
| Unit – Enhanced | PitchTracker, CadenceAnalyzer, HarmonicAnalyzer | Passing |
| Unit – VAD | State machine, thresholds, transitions | Passing |
| Unit – Audio | Player, Recorder, Level processing | Passing |
| Unit – Memory/Security | Access control, memory protection | Passing |
| Unit – I/O Optimization | Async writers, streaming buffers | Passing |
| Unit – DTW | Edge cases, window paths, cost computation | Passing |
| Integration – Engine | Session create/load/process, similarity retrieval | Passing |
| Real Audio Integration | Master call MFCC distance, similarity scaffolding | Passing (0 skips) |
| Performance Guards | Combined enhanced path < target | Passing |

**Test Suite Structure:** Reorganized into focused directories:
- `tests/unit/` - Component-level unit tests (core, analyzers, vad, audio, etc.)
- `tests/integration/` - Cross-component integration tests
- `tests/performance/` - Performance benchmarking and profiling
- `tests/lib/` - Shared test utilities and fixtures

Target: All 121 tests passing with 0 skips; expand extended capabilities coverage.

---

## 4. Result<T> & Status Usage Pattern
Always:
```cpp
auto engineR = UnifiedAudioEngine::create();
ASSERT_TRUE(engineR.isOk());
auto engine = std::move(engineR.value);

auto sessionR = engine->createSession(44100.f);
ASSERT_TRUE(sessionR.isOk());
SessionId s = sessionR.value;

auto st = engine->processAudioChunk(s, std::span<const float>(buffer));
EXPECT_EQ(st, UnifiedAudioEngine::Status::OK);

auto simR = engine->getSimilarityScore(s);
if (simR.isOk()) {
 float score = simR.value;
} else {
 // EXPECT_* or ASSERT_* depending on test intent
}
```
Never access `value` without `.isOk()`.

---

## 5. Fixture Template
```cpp
class EngineFixture : public ::testing::Test {
protected:
 void SetUp() override {
 auto r = UnifiedAudioEngine::create();
 ASSERT_TRUE(r.isOk());
 engine = std::move(r.value);

 auto sr = engine->createSession(44100.f);
 ASSERT_TRUE(sr.isOk());
 session = sr.value;
 }
 void TearDown() override {
 if (engine) {
 engine->destroySession(session);
 }
 }
 std::unique_ptr<UnifiedAudioEngine> engine;
 SessionId session{};
};
```

---

## 6. Master Audio Asset Usage
| Purpose | Asset Source | Policy |
|---------|--------------|--------|
| Similarity & Feature Separation | data/master_calls/** | Must exist; skip only if not present |
| Normalized Feature Validation | data/processed_calls/normalized/** | Use for deterministic MFCC tests |
| Synthetic Precision Tests | Generated sine / noise | Keep short (≤1s) |

Add no new large binaries without approval. Consider Git LFS if >10 MB.

---

## 7. Similarity Readiness Testing
Use `getRealtimeSimilarityState()` to poll until `reliable == true` or frame threshold reached; then assert final similarity. Example:
```cpp
for (int i=0; i<maxChunks && !state.reliable; ++i) feedChunk(...);
EXPECT_TRUE(state.reliable);
```

---

## 8. Finalization & Overlay Tests
| Test | Intent |
|------|--------|
| FinalizeCreatesSegmentAndRefinedSimilarity | finalizeSessionAnalysis populates segment & refined similarity |
| SegmentSelectionEdgeCases | Multiple bursts / trailing silence |
| LoudnessNormalizationAccuracy | RMS deviation ≤ 2% after normalizationScalar |
| SimilarityReadinessDeterministic | Deterministic frame threshold for reliability |
| CalibrationGradeBoundaries | Grade transitions at documented thresholds |
| OverlayExportAlignment | Drift < 1 frame for decimated peaks |

Add only after API fields appear in summary struct.

---

## 9. Performance Guard Strategy
| Guard | Threshold | Rationale |
|-------|-----------|-----------|
| processAudioChunk (enhanced on) | < 12 ms | Maintain real-time UX |
| Individual analyzer (pitch/harmonic/cadence) | < 4 ms | Prevent single analyzer dominance |
| finalizeSessionAnalysis | < 40 ms | One-shot post-stop responsiveness |

Pattern:
```cpp
auto t0 = now();
engine->processAudioChunk(s, span);
auto dt = elapsedMs(t0);
EXPECT_LT(dt, 12.0) << "Enhanced path regression";
```

---

## 10. Virtual Clock
All timing-sensitive tests use a FakeClock to advance millis manually. Do not add real sleeps.

---

## 11. MFCC & DTW Determinism
Ensure:
- Fixed window/hop constants
- No random initialization
- Floating comparisons use tolerant band:
```cpp
EXPECT_NEAR(mfcc[i], expected[i], 1e-4f);
```
For distance metrics, assert monotonic separation (self < different - margin).

---

## 12. Test Naming Conventions
| Pattern | Example |
|---------|---------|
| ComponentBehavior | PitchTracker_BasicSineDetection |
| ComponentEdgeCase | VoiceActivityDetector_BorderlineEnergy |
| IntegrationScenario | MasterCallsComparison_SelfVsDifferent |
| Performance | UnifiedEngine_Performance_EnhancedPathUnderBudget |
| Planned Finalization | FinalizeSession_SegmentExtraction |

Avoid ambiguous names (e.g., Test1, BasicTest).

---

## 13. Failure Triage Checklist
1. Confirm asset availability (missing WAV?).
2. Check readiness vs frames fed.
3. Run single test with TRACE on target component.
4. Inspect performance guard timing (regression?).
5. Compare last commit diff for analyzer logic or smoothing constants.

---

## 14. Logging Policy in Tests
Default log level: ERROR.
Temporary elevation (single test):
```cpp
DebugLogger::getInstance().setComponentLogLevel(Component::PITCH_TRACKER, LogLevel::DEBUG);
```
Never commit TRACE-enabled global logging.

---

## 15. Adding a New Analyzer Test (Template)
Steps:
1. Update [docs/mvp_todo.md](mvp_todo.md) with new analyzer milestone.
2. Create unit test file under `tests/unit/analyzers`.
3. Add integration test only if cross-component behavior (pitch + cadence interplay).
4. Add performance guard assertion if analyzer adds >1 ms typical cost.
5. Run full suite; ensure no new skips/warnings.

---

## 16. Skips Policy
Temporary skip allowed only if:
- Referenced explicitly in [docs/mvp_todo.md](mvp_todo.md)
- Has a concrete removal milestone date
Transform skip to assert once dependency (API / readiness) delivered.

---

## 17. Coverage & Metrics (Optional)
If coverage runs are re-enabled:
- Build with `-O0 -g --coverage`
- Exclude tools & external libs
- Track: lines %, branches %, critical functions 100%
Do not gate deployment on coverage until finalize stage is merged.

---

## 18. WASM Considerations
- Keep audio buffers small (chunked) for test determinism.
- Avoid blocking CPU loops > 16 ms (simulated frame time).
- Reuse same sampleRate between native & WASM tests where possible.

---

## 19. Future Expansion Hooks
| Area | Test Hook |
|------|-----------|
| Calibration Grades | Provide synthetic confidence arrays hitting thresholds |
| Overlay Export | Validate decimation ratio & sample alignment |
| Loudness Normalization | Input amplitude scaling scenarios |
| Coaching Feedback Mapper | Deterministic metric inputs → fixed textual outputs |

---

## 19.1 Analysis & Calibration (Planned – Test Additions)

Planned suites to accompany finalize/readiness/overlay/calibration features (see mvp_todo.md):

- Finalize Stage
 - Segment extraction: multi-burst and trailing-silence selection
 - Refined DTW: similarityAtFinalize monotonic vs provisional
 - Loudness normalization: RMS error ≤ 2%
- Readiness
 - Deterministic threshold loop: reliable within N frames at given sampleRate
 - Negative path: stays unreliable below threshold
- Overlay Export
 - Decimation length ratio within tolerance; drift < 1 frame per block
 - Offset handling (when setOverlayOffset is added)
- Calibration Grades
 - Boundary fixtures for pitch/harmonic/cadence A–F transitions
 - Stability after readiness: grade variance < target band

- Mic Calibration Advisor
 - Synthetic noise/music fixtures to validate noise floor and headroom calculation
 - Recommendation bands map deterministically to documented thresholds

- Latency/Drift Calibrator
 - Synthetic offset fixtures to validate ms error ≤ 1 ms
 - Synthetic drift (ppm) fixtures to validate ppm error ≤ 10 ppm

Acceptance snapshot (add when APIs land):

- 0 skips in similarity/overlay/finalize
- finalize path < 40 ms typical; streaming unchanged
- Export arrays sizes and alignment deterministic across runs

## 20. Exit Criteria (Testing Phase Alignment)

All of:

- 0 skips
- finalizeSessionAnalysis tests green
- Readiness deterministic
- Virtual clock adopted
- Performance guards stable
- New summary fields have coverage

---

## 21. Quick Command Reference

```bash
# Full suite
timeout 60 ./build/bin/RunEngineTests

# Focus master call tests
timeout 60 ./build/bin/RunEngineTests --gtest_filter="*MasterCallsComparison*"

# Single test case (escape dots)
timeout 30 ./build/bin/RunEngineTests --gtest_filter="PitchTracker_BasicSineDetection"

# Performance only (filter pattern)
timeout 30 ./build/bin/RunEngineTests --gtest_filter="*Performance*"
```

---

## 22. Test Failure Example (Similarity Readiness)

Observed:

- getSimilarityScore(): INSUFFICIENT_DATA
Action:
- Feed additional frames until readiness threshold (planned API)
- If threshold passed → assert reliability
Else → fail with diagnostic log extraction.

---

## 23. Maintenance Checklist

| Interval | Action |
|----------|--------|
| Weekly | Confirm no new skips |
| Analyzer Add | Add unit + integration + perf guard |
| Pre-Release | Run full suite with performance instrumentation |
| After Refactor | Compare timing deltas; update guard thresholds only with justification |

---

## 24. Summary

Testing framework ensures correctness, timing guarantees, and future extensibility (finalize, readiness, overlay, calibration). Eliminate skips, enforce `Result<T>` discipline, keep latency guards tight, and align every addition with [docs/mvp_todo.md](mvp_todo.md).

---
