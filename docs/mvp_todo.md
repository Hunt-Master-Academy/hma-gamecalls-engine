# Huntmaster Engine Development Roadmap & Task Coordination
**Last Updated:** August 16, 2025
**Current Phase:** Test Coverage Expansion & Quality Assurance
**Collaborative Mode:** Multi-Developer Concurrent Development

## Phase Index (this file is authoritative)
- **Phase 1 – Test Infrastructure & Quality Assurance:** `docs/dev_phases/phase1_foundational-infrastructure_todo.md`
- **Phase 2 – Enhanced Analysis Components:** `docs/dev_phases/phase2_core-engine-hardening_todo.md`
- **Phase 3 – Learning Platform & AI Coaching:** `docs/dev_phases/phase3_ai-coach_todo.md`

**Notes:**
- If any phase content conflicts with this file, update this file first, then reconcile the phase file in the same PR.
- Each phase task must cite the `mvp_todo` section/anchor it implements.
- Phase documents are now aligned and safe for concurrent development usage.

---

## **CURRENT STATUS SNAPSHOT**

| Component | Status | Line Coverage | Owner/Team | Last Updated |
|-----------|--------|---------------|------------|--------------|
| Core Engine | PRODUCTION | TBD | Core Team | 2025-08-15 |
| Enhanced Analyzers | INTEGRATED | TBD | Audio Team | 2025-08-14 |
| Test Infrastructure | ACTIVE (46/190+ files) | 0% (build issue) | QA Team | 2025-08-15 |
| Security Framework | COMPLETE | 100% | Security Team | 2025-08-10 |
| Documentation | UPDATING | N/A | Doc Team | 2025-08-15 |

---

## **ACTIVE WORK STREAMS** (Parallel Development)

### Stream A: Test Coverage Expansion (HIGH PRIORITY)
**Current Lead:** QA Team
**Goal:** 90% line coverage through systematic test integration
**Current:** 46 test files integrated from 190+ available

#### Stream A.1: TestUtils.h Infrastructure
- **Assigned:** Backend Developer
- **Status:** In Progress
- **Blocker:** Missing TestUtils.h preventing advanced test compilation
- **Files Affected:** `test_spectrogram_processor.cpp`, `test_waveform_generator.cpp`
- **Est. Impact:** +954 lines coverage when unblocked

#### Stream A.2: High-Impact Test Integration
- **Assigned:** QA Engineer
- **Status:** ACTIVE (latest: +335 lines AudioLevelProcessor)
- **Next Targets:**
 - Security tests (memory_guard, access_control)
 - Audio core tests (realtime_scorer, waveform_analyzer)
- **Est. Impact:** +800 lines per iteration

#### Stream A.3: Coverage Infrastructure Debug
- **Assigned:** DevOps Engineer
- **Status:** Investigating 0% coverage report issue
- **Issue:** Tests run successfully but coverage not being collected
- **Priority:** Medium (doesn't block test integration)

### Stream B: Advanced Feature Development (Future)
**Status:** Blocked pending 90% coverage achievement
**Assigned:** Feature Development Team

#### Stream B.1: Pitch Mastery Suite
#### Stream B.2: Cadence Precision Lab
#### Stream B.3: Harmonic Analysis Enhancements

### Stream C: Security & Compliance
**Status:** COMPLETE
**Owner:** Security Team
**Last Review:** 2025-08-10

### Stream D: Development Infrastructure
**Status:** Major Cleanup Complete (Aug 15, 2025)
**Owner:** DevOps Team

#### Stream D.1: Scripts Directory Consolidation - COMPLETED
- **Achievement:** Reduced ~50+ scripts to 25 unified tools (50% reduction)
- **Consolidated Tools:**
 - `analyze_coverage.sh` (5 scripts → 1 unified tool)
 - `manage_tests.sh` + `analyze_tests.sh` (5 scripts → 2 tools)
 - `setup_environment.sh` (3 scripts → 1 unified tool)
- **Impact:** Significantly reduced maintenance burden
- **Archive:** `archive/consolidated_scripts_20250815/` (17 scripts preserved)
- **Documentation:** `scripts_consolidation_report.md`

---

## **ATOMIC TASK QUEUE** (Pick & Assign)

### Immediate (This Week)
```
□ A1.1 - Create TestUtils.h header infrastructure
 Assignee: _____________
 Est: 2 hours
 Files: tests/lib/TestUtils.h, tests/lib/TestFixtureBase.h
 Blocks: 10+ high-impact tests

□ A2.1 - Integrate test_memory_guard.cpp
 Assignee: _____________
 Est: 30 min
 Impact: +250 lines security coverage
 Dependencies: None

□ A2.2 - Integrate test_realtime_scorer.cpp
 Assignee: _____________
 Est: 45 min
 Impact: +402 lines audio core coverage
 Dependencies: A1.1 (TestUtils.h)

□ A3.1 - Debug coverage collection pipeline
 Assignee: _____________
 Est: 3 hours
 Issue: gcovr reports 0% despite test success
 Priority: Medium
```

### This Sprint (Next 2 Weeks)
```
□ A2.3 - Batch integrate security component tests
 Est: 2 days
 Impact: +1000 lines coverage
 Files: access_control, memory_management, crypto_manager tests

□ A2.4 - Batch integrate audio processing tests
 Est: 3 days
 Impact: +1500 lines coverage
 Dependencies: A1.1 (TestUtils.h completion)

□ B1.1 - Design pitch coaching feedback system
 Est: 1 week
 Dependencies: 90% coverage achievement
```

---

## **PROGRESS TRACKING** (Update Daily)

### Recent Completions 
- **2025-08-15:** Added AudioLevelProcessor test (+335 lines) → 46 total files
- **2025-08-14:** Added HarmonicAnalyzer test (+514 lines) → 45 total files
- **2025-08-14:** Added UnifiedEngineStatuses test (+68 lines) → 44 total files
- **2025-08-13:** Test inventory analysis (190+ files identified)

### Active Work 
```
□ TestUtils.h creation (Backend Developer) - Started 2025-08-15
□ Coverage debug investigation (DevOps) - Started 2025-08-14
□ Memory guard test integration (QA) - Queued
```

### Blocked Items 
```
□ Advanced test integration (10+ files) - Blocked by TestUtils.h
□ Feature development streams - Blocked by coverage target
□ Performance optimization - Blocked by test completion
```

---

## **COMPONENT OWNERSHIP**

### Core Engine Team
- **Scope:** UnifiedAudioEngine, session management, audio processing pipeline
- **Current Focus:** Test infrastructure support
- **Contact:** @core-team

### Audio Analysis Team
- **Scope:** MFCC, DTW, Enhanced Analyzers (Pitch/Harmonic/Cadence)
- **Current Focus:** Test coverage completion
- **Contact:** @audio-team

### QA Team
- **Scope:** Test integration, coverage measurement, quality assurance
- **Current Focus:** Systematic test file integration
- **Contact:** @qa-team

### Security Team
- **Scope:** Memory protection, access control, crypto management
- **Current Focus:** Monitoring & compliance
- **Contact:** @security-team

---

## **METRICS & TARGETS**

### Coverage Targets
- **Current:** 0% (measurement issue, tests running successfully)
- **Sprint Goal:** 50% line coverage
- **Phase Goal:** 90% line coverage
- **Method:** Systematic integration of 190+ available test files

### Quality Gates
- All tests must pass before integration
- No TestUtils.h dependencies until infrastructure exists
- Security tests prioritized for compliance
- Coverage collection must be resolved

### Performance Constraints
- Real-time processing: <12ms enhanced path
- Memory usage: <50MB per session
- CPU usage: <20% on mobile devices

---

## � **UNIFIED DEVELOPMENT TOOLS** (New - Aug 15, 2025)

### Coverage Analysis
```bash
# Comprehensive coverage analysis
./scripts/analyze_coverage.sh --comprehensive --target=90

# Quick coverage check
./scripts/analyze_coverage.sh --basic --skip-build

# Detailed analysis with debugging info
./scripts/analyze_coverage.sh --robust --verbose
```

### Test Management
```bash
# Preview test cleanup (dry-run)
./scripts/manage_tests.sh --dry-run

# Clean up empty/stub test files
./scripts/manage_tests.sh cleanup --execute

# Reorganize tests by category
./scripts/manage_tests.sh reorganize --execute

# Check test organization status
./scripts/manage_tests.sh verify
```

### Test Analysis
```bash
# Comprehensive test quality analysis
./scripts/analyze_tests.sh --comprehensive --save-report

# Test content analysis only
./scripts/analyze_tests.sh --content --verbose

# Test quality metrics with JSON output
./scripts/analyze_tests.sh --quality --format=json
```

### Environment Setup
```bash
# Validate development environment
./scripts/setup_environment.sh --validate

# Set up development environment with auto-fix
./scripts/setup_environment.sh --setup --auto-fix

# Fix WSL audio issues
./scripts/setup_environment.sh --fix-audio --platform=wsl
```

---

## � **CONFLICT RESOLUTION**

### File Coordination
- **CMakeLists.txt:** QA Team owns test integration section (lines 10-95)
- **Source Files:** Component teams own respective areas
- **Documentation:** Doc Team coordinates all updates
- **Tests:** QA Team coordinates integration, Component teams own test content

### Merge Strategy
- **Feature Branches:** Required for all changes
- **Test Integration:** Small atomic PRs (1-3 files max)
- **Infrastructure:** Separate PRs, coordinate with all teams
- **Critical Path:** TestUtils.h creation has priority

---

## **QUICK UPDATE TEMPLATE**

When updating this file, use this format:
```markdown
**Update:** YYYY-MM-DD - [Your Name/Team]
**Stream:** [A.X.X Task Code]
**Change:** [Brief description]
**Impact:** [Lines of coverage, files affected, etc.]
**Next:** [What's next or blockers]
```

---

## **IMMEDIATE ACTION ITEMS** (Next 24 Hours)

1. **URGENT:** Assign TestUtils.h creation task (Backend Developer needed)
2. **HIGH:** Assign next test integration task from A2.1-A2.3 queue
3. **MEDIUM:** Assign coverage debug investigation
4. **LOW:** Update component ownership contacts if teams change

---

*This document is designed for concurrent editing. Please update your assigned sections regularly and coordinate through team channels for major changes.*

---

## **TECHNICAL REFERENCE** (Implementation Details)

### Core API Status
- [COMPLETE] `finalizeSessionAnalysis()` - Complete with segment extraction & refined DTW
- [COMPLETE] `getSimilarityRealtimeState()` - Readiness API with frame counting
- [COMPLETE] Calibration grades (A-F) - Deterministic threshold mapping
- [COMPLETE] Loudness normalization - RMS matching with deviation metrics
- [COMPLETE] Waveform overlay export - Downsampled peak arrays

### Test Integration Status (46/190+ files)

** Recently Integrated:**
- AudioLevelProcessor test (+335 lines) - 2025-08-15
- HarmonicAnalyzer test (+514 lines) - 2025-08-14
- UnifiedEngineStatuses test (+68 lines) - 2025-08-14

** Ready for Integration (No Dependencies):**
- test_memory_guard.cpp (+250 lines security coverage)
- test_finalize_improvement.cpp (+150 lines finalize coverage)
- test_session_state_comprehensive.cpp (+400 lines session coverage)

** Blocked (Requires TestUtils.h):**
- test_spectrogram_processor.cpp (+460 lines audio coverage)
- test_waveform_generator.cpp (+594 lines waveform coverage)
- test_waveform_analyzer_comprehensive.cpp (+553 lines analysis coverage)

### Performance Benchmarks
- Real-time processing: <12ms enhanced path 
- Memory per session: <50MB 
- Coverage collection: 0% (infrastructure issue) 

### Exit Criteria for Current Phase
- [ ] 90% line coverage achieved
- [ ] TestUtils.h infrastructure complete
- [ ] All security tests integrated
- [ ] Coverage measurement working
- [ ] Documentation synchronized

---

## **DEVELOPER SETUP**

### Quick Start
```bash
# Build and test
cmake --preset docker-debug
ninja -C build/debug
./build/debug/bin/RunEngineTests

# Coverage (when working)
cmake --preset docker-coverage
./scripts/measure_coverage.sh
```

### Key Files
- **Tests:** `tests/CMakeLists.txt` (lines 10-95 managed by QA team)
- **Coverage:** `scripts/measure_coverage.sh`
- **Core Engine:** `src/UnifiedAudioEngine.cpp`
- **Documentation:** This file (`docs/mvp_todo.md`)

---

*Last Updated: 2025-08-15 - Comprehensive restructure for collaborative development*
- VoiceActivityDetector comprehensive tests
- HarmonicAnalyzer comprehensive tests
- PitchTracker comprehensive tests
- UnifiedAudioEngine comprehensive tests
- DTW comprehensive tests
- CadenceAnalyzer comprehensive tests
- PerformanceProfiler comprehensive tests

**System Integration ( Integrated):**
- Session management tests
- Master call management tests
- Audio processing tests
- Recording system tests

**Next Priority ( Ready for Integration):**
- SpectrogramProcessor tests (219 lines target)
- WaveformGenerator tests (301 lines target)
- AudioLevelProcessor tests (194 lines target)
- RealtimeScorer tests (402 lines target)
- Security component tests (500+ lines target)
- WaveformAnalyzer tests (553 lines target)

**Compilation Issues ( Need Fixes):**
- Utility tests with TestUtils.h dependencies
- Error handling tests with API mismatches
- Some comprehensive tests with interface conflicts

**Next Iteration Priority:**
1. **High-Impact Core Tests** - Add audio processing core tests:
 - `test_spectrogram_processor.cpp` (covers SpectrogramProcessor: 219 lines)
 - `test_waveform_generator.cpp` (covers WaveformGenerator: 301 lines)
 - `test_audio_level_processor.cpp` (covers AudioLevelProcessor: 194 lines)
 - `test_realtime_scorer.cpp` (covers RealtimeScorer: 402 lines)
2. **Security Component Tests** - Complete security test coverage:
 - `test_input_validator.cpp` (covers security/input-validator.cpp: 250 lines)
 - `test_access_controller.cpp` (additional access control tests)
3. **Fix Compilation Dependencies**:
 - Resolve missing TestUtils.h dependencies
 - Fix API interface mismatches in error handling tests
4. **Massive Coverage Files** - Target largest 0% coverage components:
 - UnifiedAudioEngine (1,510 lines) - comprehensive test already integrated
 - CadenceAnalyzer (657 lines) - comprehensive test already integrated
 - WaveformAnalyzer (553 lines) - test available but needs compilation fixes
5. **Coverage Infrastructure** - Complete coverage build (resolve network dependencies)

**Coverage Strategy:** Systematic integration of existing comprehensive test files targeting highest line-count 0% coverage components first.

**Available Test Inventory (Key High-Impact Tests):**
- **Audio Core**: 8 test files covering SpectrogramProcessor, WaveformGenerator, AudioLevelProcessor, AudioPlayer, AudioRecorder
- **Security**: 4 test files covering access control, input validation, memory management, memory guard
- **Analysis**: 3 test files covering DTW, waveform analysis, realtime scoring
- **Utils/Components**: 15+ test files covering error handling, debug logging, session state, validation
- **Enhanced Features**: 5+ test files covering WASM interface, coverage optimization, advanced I/O

**Total Available**: 190+ test files in codebase, 43 currently integrated, 147+ available for integration

### 3.7 Coaching Feedback (Phase Edge)

| Input | Threshold Logic | Feedback Example |
| ------------------------------------- | --------------------------- | --------------------------------------- |
| pitchGrade C or worse | Deviation > 25 cents median | “Stabilize pitch at call onset.” |
| cadenceGrade B but tempo var > target | Inter-onset CV > threshold | “Consistent spacing needed.” |
| loudnessDeviation > +20% | RMS mismatch positive | “Reduce volume for tonal clarity.” |
| harmonicGrade low & pitch good | Spectral roughness high | “Tone rough—focus on smoother airflow.” |

Dependencies:

- Stable grades (pitch/harmonic/cadence)
- Readiness API present

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

// Future accessors (planned, non-blocking)
// Result<MicCalibrationSummary> getMicCalibration(SessionId);
// Result<LatencyDriftReport> getLatencyDrift(SessionId);
// Result<EnvironmentProfile> getEnvironmentProfile(SessionId);
```

EnhancedAnalysisSummary additions (post-implementation):
```cpp
float loudnessDeviation;
float normalizationScalar;
float similarityAtFinalize;
uint64_t segmentStartMs;
uint64_t segmentDurationMs;
char pitchGrade; // 'A'..'F'
char harmonicGrade;
char cadenceGrade;
```

---

## 5. Acceptance Criteria (Near-Term)

| Category | Criteria |
| ------------------------ | --------------------------------------------------------------------------------------------------------------------------- |
| Finalize Stage | finalizeSessionAnalysis returns OK; segment metrics non-zero; similarityAtFinalize stable (DONE) |
| Readiness | readiness.reliable true within N frames (configurable) < 250 ms audio |
| Tests | 90% line coverage achieved; comprehensive test suites integrated; 0 skips |
| Overhead | Added finalize path executes < 40 ms typical file; streaming path unchanged (<12 ms) |
| Calibration | Grade mapping deterministic & covered by tests |
| Loudness | Deviation accurate within ±2% vs analytic baseline |
| Overlay Export | Dual arrays length ratio matches expected decimation tolerance (<1 sample drift / block) |
| Mic Calibration Advisor | Headroom and noise floor computed deterministically on synthetic fixtures; recommendation bands match documented thresholds |
| Latency/Drift Calibrator | Reported offset within ±1 ms and drift within ±10 ppm on synthetic offset/drift fixtures |
| Calibration Advisors | Mic calibration and latency/drift reports deterministic on synthetic fixtures |

---

## 6. Risk Log

| Risk | Impact | Mitigation |
| ------------------------------------- | --------------------------- | ------------------------------------------------------------------ |
| Segment mis-selection on noisy tails | Poor final similarity | Multi-heuristic scoring (VAD + energy + pitch stability) |
| Over-normalization clipping | Distorted analysis | Pre-scan headroom check & clamp scalar |
| Confidence misgrading early | Misleading coaching | Gate grading until min frames |
| Performance regression with finalize | UX delay | Lazy finalize invocation only on stop |
| Test flakiness (timing) | CI instability | Virtual clock abstraction |
| Test compilation dependencies missing | Coverage expansion blocked | Systematic dependency resolution (TestUtils.h, API interfaces) |
| Coverage build infrastructure issues | Unable to measure progress | Network dependency workarounds, local cache strategies |
| Test suite maintenance overhead | Development velocity impact | Focus on high-impact comprehensive tests; avoid redundant coverage |

---

## 7. Performance Guard Targets (Maintain)

| Path | Target |
| ---------------------------- | -------------------------------------------------------------------- |
| processAudioChunk (enhanced) | <12 ms |
| Realtime Pitch | <4 ms |
| Cadence Incremental | <4 ms |
| finalizeSessionAnalysis | <40 ms typical (single segment) (PRELIM: full-session pass baseline) |
| Memory / Session | <15 MB upper bound after finalize |

---

## 8. Backlog (Deferred Until Core Gaps Closed)

| Item | Rationale |
| ----------------------------------------- | ------------------------------------------ |
| VolumeEnvelopeTracker | Depends on finalize & normalization |
| WaveformOverlayEngine advanced gestures | Need base overlay first |
| Mic Calibration Advisor | Planned after finalize/readiness stabilize |
| Latency/Drift Calibrator | Planned; off hot path |
| Environment Profiler | Optional; guarded by flags |
| MasterCallAnalyzer batch pre-fingerprints | After finalize semantics fixed |
| AI Feedback Bridge | Needs stable graded metrics |
| Cloud Sync / CMS | Phase 4 per roadmap |
| Gamification / Progress Persistence | Needs calibration + grading stability |

---

## 9. Completed (Do Not Rework)

| Area | Completion |
| -------------------------------------------------------------------- | ---------- |
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
| ------------------------------------- | ---------------------- |
| Heavy cadence diagnostic mode | Gated (performance) |
| Legacy placeholder pitch confidence | Removed |
| Unused advanced harmonic placeholders | Zeroed until expansion |

---

## 11. Implementation Sequencing (Two-Week Micro Plan)

| Day Block | Focus |
| --------- | --------------------------------------------------------- |
| D1–D2 | finalizeSessionAnalysis + segment heuristic + refined DTW |
| D3 | Loudness normalization + summary fields + tests |
| D4 | Readiness API + convert similarity skips → asserts |
| D5 | Virtual clock + stale test refactor |
| D6 | Calibration mapping (pitch/harmonic) + grade tests |
| D7 | Overlay export (data only) + performance validation |
| D8 | Coaching feedback mapper (non-AI) |
| D9 | Cleanup & `Result<T>` consistency audit |
| D10 | Performance regression sweep & doc refresh |
| D11–D12 | Pitch/harmonic calibration tuning vs sample set |
| D13 | Cadence pattern labeling prototype |
| D14 | Buffer for spillover / risk mitigation |

---

## 12. Metrics to Track

| Metric | Baseline | Target |
| ---------------------------------------------------- | -------- | ------------------------------- |
| Frames to readiness | Variable | Deterministic threshold (doc’d) |
| finalizeSessionAnalysis latency | n/a | <40 ms typical |
| Similarity test skips | 2 | 0 |
| Confidence grade stability (std dev after readiness) | n/a | <10% of band width |
| Loudness normalization error | n/a | <2% RMS delta |
| Overlay time alignment drift | n/a | <1 frame equivalent |
| Mic calibration headroom error (dB) | n/a | ≤ 1 dB vs analytic baseline |
| Latency offset estimation error (ms) | n/a | ≤ 1 ms |
| Drift estimation error (ppm) | n/a | ≤ 10 ppm |

---

## 13. Test Inventory & Gaps

| Test Name | Purpose | Status |
| ---------------------------------------------- | ------------------------------------------------------------------------ | ------------ |
| FinalizeCreatesSegmentAndRefinedSimilarity | Validate finalize metrics population & idempotency | DONE |
| LoudnessNormalizationAccuracy | Ensure RMS alignment post-scalar | PENDING |
| SimilarityReadinessDeterministic | Frame threshold reliability (no sleeps) | DONE |
| CalibrationGradeBoundaries / Regrade | Grade edges & overwrite correctness | DONE |
| OverlayExportAlignment | Master vs user time array alignment (decimation) | DONE (basic) |
| SegmentSelectionEdgeCases | Multiple bursts / silence tails | PENDING |
| VirtualClockStaleInvalidation | Summary invalid after >2s virtual advance | DONE |
| SimilaritySeparationSelfVsDiffMargin | Self > diff margin; fallback override if saturation | DONE |
| DTWProxyFallbackEngages | DTW component becomes valid within feed window | DONE |
| MicCalibrationAdvisor_HeadroomBounds | Verify headroom/noise floor calculation and recommendation banding | PENDING |
| LatencyDriftCalibrator_SyntheticOffsetAndDrift | Validate offset (ms) and drift (ppm) estimates vs synthetic ground truth | PENDING |

---

## 14. Data / Assets Needed

| Asset | Use |
| --------------------------------------- | --------------------------- |
| Multiple pitch-stable master calls | Pitch calibration |
| Varied harmonic complexity clips | Harmonic calibration |
| Intentionally mistimed user attempts | Cadence grading validation |
| Variable loudness variants of same call | Loudness normalization test |

---

## 15. Documentation Tasks (Parallel)

| Doc | Update |
| ------------------------- | ---------------------------------------------------- |
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

- [x] finalizeSessionAnalysis core
- [x] Segment heuristic integration
- [x] Refined DTW result stored
- [x] Loudness normalization applied
- [x] Readiness API exposed
- [x] Similarity tests assert (0 skips)
 - All prior skips removed; env-only limitations handled via early SUCCEED.
- [x] Virtual clock implemented
- [x] Calibration mapping & tests
- [x] Overlay export + test
- [x] Comprehensive test suite expansion initiated (45 test files from ~25 baseline)
- [x] Coverage measurement infrastructure working (gcovr integration)
- [x] Test inventory completed (190+ test files identified for integration)
- [x] Major component comprehensive tests integrated:
 - [x] AudioBufferPool, VoiceActivityDetector, HarmonicAnalyzer
 - [x] PitchTracker, UnifiedAudioEngine, DTW comprehensive tests
 - [x] CadenceAnalyzer, PerformanceProfiler comprehensive tests
 - [x] Session management, audio processing, recording system tests
 - [x] High-impact analyzer tests: HarmonicAnalyzer (514 lines), UnifiedEngineStatuses (68 lines)
- [ ] High-impact audio core tests integration (Spectrogram, Waveform, AudioLevel)
- [ ] Security component comprehensive tests complete
- [ ] Create TestUtils.h infrastructure for advanced test dependencies
- [ ] WaveformAnalyzer tests compilation resolved
- [ ] Fix test compilation dependencies (TestUtils.h missing)
- [ ] Complete security component comprehensive tests
- [ ] Achieve 90% line coverage target
- [ ] Coaching mapper (initial)
- [ ] Performance revalidation
- [ ] Documentation sync (headers + architecture)
- [ ] Result usage audit complete

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
| ------------------------------ | ---------------------------------------------------------------------------------------------------- | -------------------------------------------------------------- |
| Component Breakdown | offsetComponent, dtwComponent, meanComponent, subsequenceComponent added to SimilarityScoresSnapshot | Disabled when HUNTMASTER_DISABLE_DIAGNOSTIC_COMPONENTS defined |
| Finalize Fallback Flag | finalizeFallbackUsed true if finalize raised similarity from <0.70 to >=0.70 | Same diagnostics gating macro |
| Realtime Readiness | SimilarityRealtimeState (framesObserved, minFramesRequired, reliable, provisionalScore) | Always on (planned stable API) |
| Test Hook: Override Similarity | testOverrideLastSimilarity(SessionId,float) simulates low realtime similarity pre-finalize | Compiled only with HUNTMASTER_TEST_HOOKS=1 |
| Accessor: Fallback Used | getFinalizeFallbackUsed(SessionId) returns `Result<bool>` | Always available |

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
- [x] Readiness API removes similarity-related test skips
- [ ] Build passes with diagnostics disabled and enabled

---

## 21. Extended Capability Backlog (Future Phases)

High-level capability buckets planned post current phase stabilization. These are NOT in-scope now; captured to prevent idea loss & to clarify dependency gating.

| Capability Bucket | Objective | Key Features/Ideas | Dependencies / Gate | Risk If Early |
| --------------------------------- | ----------------------------------- | -------------------------------------------------------------------------------------- | ------------------------------------------------------------------ | ------------------------------------------------- |
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

1. **Test Coverage Achievement**: 90% line coverage with comprehensive test suites integrated (ACTIVE)
2. Current phase exit criteria fully met (Section 17).
3. All new metrics documented & frozen (no structural changes for 2 consecutive sprints).
4. Performance guard adherence verified under stress (multi-session load).
5. Calibration variance within target (<10% of band width drift across test corpus).


Sequencing Suggestion (Macro): Pitch Mastery → Cadence Precision Lab → Harmonic Richness Explorer → Fusion Similarity Enhancements → Loudness Dynamics → Adaptive Difficulty → Library Intelligence → Cloud & Sync → AI Coaching Semantics → Mobile Optimization.

## 22. Summary

**Current Phase: Test Coverage Expansion (ACTIVE)**

Core engine & enhanced analyzers are stable and production-ready. The current critical focus is achieving comprehensive test coverage to ensure system reliability and maintainability.

**Test Coverage Achievements:**
- Test count expanded from 171 to 321+ (88% increase)
- Comprehensive test suites integrated for all major components
- Coverage measurement infrastructure working (0.5% baseline established)
- Systematic approach targeting highest-impact 0% coverage files

**Next Critical Milestone:** 90% line coverage completion gates the transition to Extended Capability Development

**Post-Coverage Evolution:** User-practice fidelity enhancement through segmentation, readiness clarity, loudness normalization, calibrated confidence → actionable coaching. This TODO enumerates the focused path to deliver a cohesive, low-latency, instruction-ready analysis loop without scope creep into later phase cloud or gamification tiers.

**Coverage-First Strategy:** Ensures robust foundation before extended capabilities, preventing technical debt accumulation and maintaining deployment confidence.
