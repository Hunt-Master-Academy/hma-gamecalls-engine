# Hunt Master Academy Game Calls Engine Deployment Guide
Last Updated: August 9, 2025
Status Context: Core Engine Production | Enhanced Analyzers Phase 1 Integrated | Entering UX Alignment & Finalization

---

## 1. Scope
Operational deployment, packaging, performance posture, and upcoming integration considerations (finalize session stage, similarity readiness, loudness normalization, overlay export). Always reconcile with `docs/mvp_todo.md` before altering deployment assumptions.

---

## 2. Build Matrix

| Variant | Purpose | Flags |
|---------|---------|-------|
| Debug | Development & instrumentation | `-DCMAKE_BUILD_TYPE=Debug` |
| Release | Production optimized | `-DCMAKE_BUILD_TYPE=Release` |
| Minimal Core | Embed without tests/tools | `-DHUNTMASTER_BUILD_TESTS=OFF -DHUNTMASTER_BUILD_TOOLS=OFF` |
| WASM | Browser / Web runtime | `emcmake cmake` (Emscripten) |

Baseline commands:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```
WASM:
```bash
emcmake cmake -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm -j
```

---

## 3. CMake Options
| Option | Default | Description |
|--------|---------|-------------|
| HUNTMASTER_BUILD_TESTS | ON | Unit & integration tests |
| HUNTMASTER_BUILD_TOOLS | ON | CLI tools (auto-discovered) |
| HUNTMASTER_ENABLE_BENCHMARKS | OFF (if present) | Performance benchmarking |
| (Planned) HUNTMASTER_ENABLE_HEAVY_CADENCE | OFF | Gated heavy diagnostic path |

| (Planned) HUNTMASTER_ENABLE_NR | OFF | Optional RNNoise noise suppression (engine-flagged) |
| (Planned) HUNTMASTER_ENABLE_CALIBRATION | ON | Enables calibration summaries (lightweight) |

Do not enable heavy diagnostics in production.

---


## 4. Artifacts

| Artifact | Type | Notes |
|----------|------|------|
| libHuntmasterEngine.a / .so / .wasm | Core library | Deterministic ABI within phase |
| Tools executables | Diagnostics | Deploy selectively |
| Web bundle (wasm + JS glue) | Frontend integration | Serve with correct MIME types |

---


## 5. Deployment Profiles

### 5.1 Production (Library Mode)

- Release build
- Logging: Global ERROR (component overrides only during incidents)
- Disable tests/tools in shipped artifact for consumer SDK distribution

### 5.2 Staging (Feature Validation)

- Release build + INFO for UNIFIED_ENGINE / PERFORMANCE DEBUG
- Include overlay / finalize experimental flags once implemented

### 5.3 Developer Sandbox

- Debug build
- Full tests + tools
- Optional TRACE for a single component

---

## 6. Runtime Configuration (Env / Flags)

| Variable | Effect | Default |
|----------|--------|---------|
| HUNTMASTER_LOG_LEVEL | Overrides global log level | ERROR |
| HUNTMASTER_ENABLE_HEAVY_CADENCE | Enables heavy cadence diagnostics | (unset/false) |
| HUNTMASTER_PROFILING | Enables AutoProfiler sections | (unset) |
| HUNTMASTER_OVERLAY_DECIMATION | Override overlay export step size | 512 |
| HUNTMASTER_ENABLE_NR (planned) | Toggle optional noise suppression path | 0/false |
| HUNTMASTER_ENABLE_CALIBRATION (planned) | Enable calibration summaries APIs | 1/true |

Example bootstrap:

```cpp
if (const char* lvl = std::getenv("HUNTMASTER_LOG_LEVEL")) {
    logger.setGlobalLogLevel(parseLogLevel(lvl));
}
```

---


## 7. Logging & Observability

- Keep file logging disabled by default; enable on incident only.
- Planned finalize session logging should emit a single summary line (segment boundaries, similarityAtFinalize).
- Avoid per-frame logs in production; rely on performance counters or aggregated reports.


## 8. Performance Targets (Current)

| Path | Target |
|------|--------|
| processAudioChunk (enhanced) | <12 ms typical |
| Pitch / Harmonic / Cadence each | <4 ms |
| (Planned) finalizeSessionAnalysis | <40 ms (one-shot) |
| Memory / session | <15 MB post-finalize |

Regression detection: performance guard tests (in CI) must remain green; failures block deployment.

---


## 9. Security Posture

Implemented:

- Strict session isolation
- Bounds checking & memory guards
- Minimal dependency surface
Planned:
- Master call integrity hashing (phase aligned with cloud sync)
Deployment note: Avoid shipping debug symbols externally unless needed for crash forensics.

---

## 10. Test Strategy Before Deploy

| Stage | Action |
|-------|--------|
| Unit/Integration | `timeout 60 ./build/bin/RunEngineTests` |
| Real Audio Integration | `--gtest_filter="*MasterCallsComparison*"` |
| Performance Sweep | (Optional) benchmarks / profiling tool |
| Skipped Cases | Ensure no remaining skips once readiness API implemented |


---

## 11. WASM Deployment

Checklist:

- Serve `.wasm` with `application/wasm`
- Preload master calls (range requests optional)
- Confirm AudioWorklet or ScriptProcessor fallback
- Validate microphone permission prompts
- Keep logging minimal (console spam = perf hit)

Simplified local test:

```bash
python3 -m http.server 8080
$BROWSER http://localhost:8080/web/alpha_test_refactored.html
```

---

## 12. Planned Deployment Additions

| Feature | Deployment Consideration |
|---------|--------------------------|
| finalizeSessionAnalysis | Trigger only on user stop; asynchronous UI spinner |
| Readiness API | UI gating (enable similarity meter once reliable) |
| Loudness normalization | Non-destructive; analysis-only metadata |
| Overlay export | Serve downsample arrays; avoid raw waveform spam |
| Calibration grades | Embed static tables; avoid runtime remote fetch |
| Mic calibration & gain advisor | Expose summary JSON; no device I/O required by engine |
| Latency/drift calibration | One-shot, off hot path; store alignment/drift in session config |
| Environment profiler | Disabled by default; NR path behind build+runtime flags |
| Contributions/why breakdown | JSON export only; no UI coupling in engine |

---

## 13. Incident Response

| Issue | Action |
|-------|--------|
| Latency spike | Enable PERFORMANCE DEBUG for a short window |
| Incorrect similarity | Re-run finalize & capture diagnostic snapshot |
| Pitch instability | Enable PITCH_TRACKER TRACE (1–2 chunks) |
| Memory growth | Verify sessions destroyed; run with sanitizers in staging |

Snapshot strategy: add lightweight “dump summary” tool invocation that prints key metrics for a given session.

---

## 14. Rollback Strategy

1. Maintain previous stable artifact (N-1) in release store.
2. Rollback triggers:
   - Performance guard fail in production
   - Similarity correctness drift reported
3. Post-rollback:
   - Enable targeted logging
   - Reproduce issue in staging using captured audio

---

## 15. Integration Guidelines (Consumer App)

Code pattern (current API):
```cpp
auto engineR = UnifiedAudioEngine::create();
if (!engineR.isOk()) return;
auto engine = std::move(engineR.value);

auto sessionR = engine->createSession(44100.f);
if (!sessionR.isOk()) return;
SessionId s = sessionR.value;

engine->loadMasterCall(s, "buck_grunt");
engine->enableEnhancedAnalyzers(s);

engine->processAudioChunk(s, span);
// (Planned) if user stops:
engine->finalizeSessionAnalysis(s);

auto sim = engine->getSimilarityScore(s);
auto summary = engine->getEnhancedSummary(s);
```
Future additive functions must remain backward-compatible (no breaking signature changes).

---

## 16. Deployment Checklist (Condensed)

| Step | Required |
|------|----------|
| Release build succeeds | ✔ |
| Tests (0 failures; post-milestone 0 skips) | ✔ |
| Performance guard pass | ✔ |
| Heavy cadence diagnostics disabled | ✔ |
| Logging level = ERROR or WARN | ✔ |
| README / mvp_todo synced | ✔ |
| finalize + readiness APIs | Documented & stable | Implemented |
| WASM bundle loads & initializes | ✔ (if web channel shipped) |

---

## 17. Observability Roadmap

| Phase | Observability Additions |
|-------|-------------------------|
| Current | Performance guard + basic logs |
| Next | Readiness metrics & finalize summary log |
| Later | Calibration grade distribution counters |
| Cloud Phase | Aggregated similarity trend metrics |

---

## 18. Maintenance
- Rotate debug logs with date-based filenames if enabled: `huntmaster_YYYYMMDD.log`.
- Audit environment variable usage quarterly; remove stale flags once defaults stabilise.
- Re-run performance baselines after major analyzer additions.

---

## 19. Risk Matrix
| Risk | Impact | Mitigation |
|------|--------|-----------|
| Diagnostic flag accidentally enabled | Latency regressions | CI asserts flag off in release |
| Skipped similarity tests leak to prod | Undetected readiness bug | Pre-deploy script checks for “SKIP” substrings |
| Over-normalization (future) clipping | Distorted analysis metrics | Bound scalar + headroom check |
| Overlay export oversize payload | UI perf impact | Fixed decimation + size cap |

---

## 20. Summary
Deployment focus: maintain low-latency streaming, prevent diagnostic drift into production, prepare for finalize & readiness upgrades without destabilizing API consumers. All changes flow through the authoritative roadmap (`docs/mvp_todo.md`); this guide reflects deployment implications only. Keep artifacts lean, logs minimal, and performance guardrails
