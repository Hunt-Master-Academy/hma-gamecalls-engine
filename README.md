# Hunt Master Academy – Game Calls Engine (Huntmaster Audio Core)

Last Updated: August 9, 2025
Status: MVP COMPLETE – Enhanced Analyzers Phase 1 Integrated (Pitch, Harmonic, Cadence)
Current Phase: Enhanced Platform Integration & UX Alignment

## Platform Context
Hunt Master Academy is a multi‑pillar training platform (Game Calls, Game Strategy, Stealth & Scouting, Tracking & Recovery, Gear & Marksmanship).
THIS REPOSITORY: Implements only the Game Calls pillar realtime analysis core (“Unified Audio Engine”).
Roadmap authority: docs/mvp_todo.md (always sync changes there first).

## Pillar 1 (Game Calls) Feature Landscape & Status (Roadmap-Aligned)
| Layer | Implemented Now | In Progress (Near-Term) | Planned (Later Phases) |
|-------|-----------------|-------------------------|------------------------|
| Core Audio Pipeline | Session engine, MFCC, realtime + DTW similarity | finalizeSessionAnalysis (segment + refined DTW) | Multi-master comparative sets |
| Enhanced Analyzers | Pitch, Harmonic, Cadence (EMA smoothed) | Confidence calibration (grades) | VolumeEnvelopeTracker, MasterCallAnalyzer |
| Loudness | RMS / Peak measurement | Normalization scalar + deviation metric | Dynamic envelope / attack-decay metrics |
| Segmentation | Streaming VAD gating | Best-call region extraction | Multi-pass smart segmentation |
| Readiness | Implicit (frame accumulation) | Explicit readiness state API | Predictive readiness heuristics |
| Visualization Data | (Single waveform via existing tools) | Overlay export (master + user peaks) | Spectrogram overlays / dialect layers |
| Coaching Metrics | Raw similarity + analyzer values | Metric→tip mapper (non-AI), graded bands | AI narrative feedback / adaptive drills |
| Practice UX | (External UI: load, record, basic metrics) | Countdown integration + overlay alignment | Scenario-driven guided sessions |
| Data Calibration | None | Pitch / harmonic / cadence grade thresholds | Species & call-type specific calibration |
| Testing | Core + analyzer + real audio MFCC distance | Remove similarity skips (readiness loop) | Curriculum progression tests |
| Performance | <12 ms enhanced path | Guard finalize (<40 ms) | SIMD optimization wave 2 |

(For all “In Progress / Planned” see task breakdown in docs/mvp_todo.md.)

## Mapping Original Pillar Breakdown → Current Scope
| Original Pillar 1 Category | Current Engine Coverage | Deferred (Out of Scope Here) |
|----------------------------|-------------------------|-------------------------------|
| Call Library & Education (species, 200+ calls, dialect maps) | Master call load + limited curated assets | Full taxonomy, dialect maps, educational content (handled upstream) |
| AI Bioacoustics Coach (multi-dimensional feedback, progress dashboards) | Core metrics (similarity, pitch, cadence, harmonicity) | Historical progress graphs, rich UI overlays, narrative coaching |
| Structured Learning Modules (Foundation → Mastery hours) | Not implemented (needs graded calibrations) | Curriculum system, progression logic |
| Interactive Practice Tools (scenarios, call-and-response, rhythm trainer) | Cadence metric groundwork | Scenario engine, simulator, device advisor |
| Field Mode (quick reference, situation matcher, favorites) | None (engine-only) | Field UX + library indexing / retrieval |
| Field Recording & Contribution | Basic ability to process loaded audio | Geotag, community submission, moderation |
| Real-time Coaching (confidence meter, context tips) | Raw confidences + smoothing | Contextual suggestion engine |

## Why Focus Narrowly Now
Reliability first: finalizeSessionAnalysis + readiness + calibration must stabilize before curriculum, AI coaching, cloud ingestion, or large library scaling to avoid compounding noisy metrics.

## Cross-Pillar Reference (Informational Only)
Other pillars (Strategy, Stealth, Tracking, Marksmanship) will *consume* some Game Calls outputs (e.g., wind-aware calling heuristics) later. No implementation for those pillars exists in this repo; do not add here until Game Calls engine phase success criteria met (see docs/mvp_todo.md “Exit Criteria”).

## Executive Summary
The Huntmaster Audio Engine (Game Calls pillar core) is a C++20, session‑oriented real‑time wildlife call analysis module providing deterministic MFCC feature extraction, dual (realtime + DTW) similarity scoring, and enhanced analyzers (pitch, harmonicity, cadence) with low latency (<12 ms enhanced path). Next steps: finalize session analysis (segmentation + loudness normalization), similarity readiness API, waveform overlay, calibration & coaching feedback.

## High-Level User Flow (Target)
1. Select master call (waveform visible, playback).
2. (Planned) Countdown → user records attempt.
3. Streaming analysis: VAD → MFCC → realtime similarity → analyzers.
4. (Planned) Overlay + stabilized metrics.
5. Stop → (Planned) finalizeSessionAnalysis (segment, normalization, refined DTW).
6. Feedback (scores + future AI narrative).

## Core Architecture
VAD → Levels → MFCC → RealtimeScorer → (Pitch/Harmonic/Cadence) → Summary (EMA) → Similarity (realtime or DTW fallback) → (Planned finalize refinement).

## Key Current Features
- Master call loading + MFCC cache
- Realtime similarity & DTW fallback
- PitchTracker (YIN) w/ confidence
- HarmonicAnalyzer baseline
- CadenceAnalyzer base rhythm
- EnhancedAnalysisSummary (EMA smoothing)
- Security & performance instrumentation
- Real audio integration tests

## Planned (Active Roadmap)
- finalizeSessionAnalysis()
- Similarity readiness API
- Waveform overlay export
- Confidence calibration → graded feedback
- Loudness deviation metrics
- Coaching feedback mapper
- MasterCallAnalyzer & VolumeEnvelopeTracker (later)
- Cloud sync / gamification (future phases)

## Public API (Current)
```cpp
auto engineR = UnifiedAudioEngine::create();
auto engine  = std::move(engineR.value);
auto sessionR = engine->createSession(44100.f);
SessionId session = sessionR.value;

engine->loadMasterCall(session, "buck_grunt");
engine->setEnhancedAnalyzersEnabled(session, true);

// Stream audio chunks
engine->processAudioChunk(session, std::span<const float>(buffer));

// Realtime similarity (peak-smoothed) single value
float similarity = engine->getSimilarityScore(session).value;

// Current + peak snapshot (UI may want both)
auto scoresSnap = engine->getSimilarityScores(session); // { current, peak }

// Readiness / reliability introspection
auto rtStateR = engine->getRealtimeSimilarityState(session);
if (rtStateR.isOk()) {
	auto st = rtStateR.value; // framesObserved, minFramesRequired, reliable, provisionalScore
}

// Enhanced analysis summary
auto enhanced = engine->getEnhancedAnalysisSummary(session);

engine->destroySession(session);
```
(Coming) finalizeSessionAnalysis(session), overlay export, calibration utilities.

## Performance
Enhanced streaming <12 ms typical; finalize target <40 ms.

## Testing
All core & similarity readiness tests passing (self / asymmetric). Performance guards active (latency thresholds). Roadmap tasks in docs/mvp_todo.md.

## Contributing (Internal)
1. Update docs/mvp_todo.md first.
2. Add tests + performance assertions.
3. Enforce Result<T>/Status checks (never assume .value without isOk()).
4. No new sleeps (virtual clock when added).
5. Keep heavy diagnostics gated (use env HUNTMASTER_VERBOSE=1 for readiness debug logs).

## Quick Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
timeout 60 ./build/bin/RunEngineTests
```

## License
See LICENSE.

## More Documentation
docs/mvp_todo.md • docs/architecture.md • docs/PROJECT_STRUCTURE.md • docs/DEBUGGING.md • docs/DEPLOYMENT.md • docs/TESTING.md

This README is the single entry point; docs/README.md
