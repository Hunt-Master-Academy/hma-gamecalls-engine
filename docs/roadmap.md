# Gamecalls Engine – Long-Term Roadmap (High-Level Vision)

> **Governance Note:**
> This file contains no live status or progress metrics. For authoritative scope, phases, acceptance criteria, risks, telemetry, and action items see: [`docs/mvp_todo.md`](docs/mvp_todo.md).

---

## Vision

The Gamecalls Engine provides fast, trustworthy, and explainable acoustic analysis for wildlife calling:
- How closely a user’s call matches an expert master
- Which acoustic dimensions need improvement (pitch, harmonic richness, cadence, loudness consistency)
- How proficiency evolves over time

It deliberately matures from deterministic, hand‑tuned signal processing toward adaptive, data‑informed refinement—without sacrificing transparency, user privacy, or deterministic fallback behavior.

The engine becomes the acoustic backbone for training, coaching, gamification, and integrated multi‑pillar experiences (strategy, education, field utility), exposing stable interfaces that tolerate internal algorithm swaps (e.g., upgraded pitch tracker, ML similarity model) without breaking clients.

---

## Strategic Themes

- **Deterministic Core → Adaptive Enhancements**
 Progressive layering of ML / statistical refinement

- **Multi-Dimensional Explainability**
 Score + per-dimension deltas + actionable suggestions

- **Real-Time Feedback Integrity**
 Low latency, graceful degradation under load

- **Privacy & Local-First Audio Handling**
 No raw audio leaving device by default

- **Interface Stability**
 Semantic contracts for similarity, finalize metrics, coaching insights

- **Performance Discipline**
 ## Gamecalls Engine – Long-Term Roadmap (High-Level Vision)

 **Governance Note:** This file contains no live status or progress metrics. For authoritative scope, phases, acceptance criteria, risks, telemetry, and action items see: [`docs/mvp_todo.md`](docs/mvp_todo.md).

 ---

 ## Vision

 The Gamecalls Engine provides fast, trustworthy, and explainable acoustic analysis for wildlife calling:
 - How closely a user’s call matches an expert master
 - Which acoustic dimensions need improvement (pitch, harmonic richness, cadence, loudness consistency)
 - How proficiency evolves over time

 It deliberately matures from deterministic, hand‑tuned signal processing toward adaptive, data‑informed refinement—without sacrificing transparency, user privacy, or deterministic fallback behavior.

 The engine becomes the acoustic backbone for training, coaching, gamification, and integrated multi‑pillar experiences (strategy, education, field utility), exposing stable interfaces that tolerate internal algorithm swaps (e.g., upgraded pitch tracker, ML similarity model) without breaking clients.

 ---

 ## Strategic Themes

 - **Deterministic Core → Adaptive Enhancements** – Progressive layering of ML / statistical refinement
 - **Multi-Dimensional Explainability** – Score + per-dimension deltas + actionable suggestions
 - **Real-Time Feedback Integrity** – Low latency, graceful degradation under load
 - **Privacy & Local-First Audio Handling** – No raw audio leaving device by default
 - **Interface Stability** – Semantic contracts for similarity, finalize metrics, coaching insights
 - **Performance Discipline** – Sub‑10ms per chunk targets; bounded memory per session
 - **Progressive Gamification** – Skill arcs, streaks, mastery tiers—never replacing fundamentals
 - **Calibration & Personalization** – Device / mic / user baseline normalization
 - **Swap-Friendly Analysis Pipeline** – Modular feature extractors behind capability flags
 - **Observability Without Intrusion** – Metrics + telemetry events—no acoustic content leakage

 ---

 ## Evolution Ladder

 1. **MVP** – Real-time similarity (MFCC + blended offset/DTW/subsequence), finalize refinement, loudness normalization scaffold, per-session diagnostics, basic coaching messages.
 2. **Beta Hardening** – Deterministic thresholds stabilized, segment selection accuracy, latency budget validation, negative/noise path tests, boundary condition suite, profiler baselines.
 3. **Advanced Dimension Layer** – PitchTracker (YIN), HarmonicAnalyzer (spectral envelope / inharmonicity), CadenceAnalyzer (syllable rate consistency), integrated multi-dimensional similarity vector.
 4. **Coaching Intelligence** – Rule-based feedback library → adaptive weighting, progress trend snapshots, overlay synthesis for “ideal vs actual” alignment preview.
 5. **Personalization & Calibration** – Device / mic EQ hints, user baseline pitch range adaptation, loudness deviation tolerance modeling, cross‑session retention of refined baselines.
 6. **ML Augmentation** – Optional embedded/lightweight learned similarity or quality model (edge-inference), confidence calibration, drift monitoring, deterministic fallback retained.
 7. **Ecosystem Integration** – Cross-pillar achievements, strategy engine synergy (acoustic readiness events), educational module auto-curation, scenario-driven challenge loops.
 8. **Continuous Optimization** – Adaptive parameter tuning, noise robustness improvements, domain expansion (species variants), multilingual / phonetic expansions for instructional narration.

 ---

 ## Future Capability Buckets

 - **Pitch Dimension Mastery:** Stable vs unstable contour classification, micro-variation scoring
 - **Harmonic Quality:** Spectral tilt, formant energy distribution, harmonicity / roughness indices
 - **Cadence Precision:** Inter-onset interval variance, syllable envelope shape consistency
 - **Loudness Dynamics:** Attack/release consistency, RMS drift, normalizing scalar accuracy tracking
 - **Overlay Synthesis:** Time-aligned composite of master vs user for UI visualization
 - **Feature Fusion Similarity:** Weighted multi-vector similarity (pitch, timbre, rhythm, dynamics)
 - **Coaching Feedback Graph:** Structured taxonomy mapping deficit → remediation tip chain
 - **Progression & Mastery Metrics:** Per-dimension skill slopes, stability thresholds for rank promotion
 - **Device / Environment Calibration:** Ambient noise floor detection, optimal gain guidance
 - **Noise Robustness Layer:** Adaptive denoising or gating without masking authentic call traits
 - **ML Similarity Module (Optional):** Embedding-based comparison (on-device), fairness & drift checks
 - **Confidence & Uncertainty:** Reliability estimates per dimension (coverage windows, variance)
 - **Offline Mode Enhancements:** Fully local analytics with deferred badge / telemetry sync
 - **Extended Species Library:** Modular species parameter packs (frequency, cadence templates)
 - **Scenario Engine Hooks:** Challenge definitions referencing acoustic criteria (timed drills, precision goals)
 - **Secure Sharing (Opt-In):** Derived metrics sharing without raw waveforms (privacy preserving)
 - **Adaptive Thresholds:** Personalized acceptance windows based on historical variance
 - **Latency Optimization:** SIMD / GPU (optional) MFCC + pitch computations, frame scheduling refinements
 - **Batch Retro-Analysis:** Reprocess stored feature sets with upgraded algorithms (migration path)

 ---

 ## Links Into Authoritative MVP Document

 | Topic | Anchor (`mvp_todo.md`) |
 |-------|------------------------|
 | MVP Definition | #1-mvp-definition-locked-scope--gamecalls-engine |
 | Phase Progress | #2-phase-progress-summary |
 | Immediate Objectives | #3-immediate-phase-objectives-gamecalls |
 | Core Feature Taxonomy | #4-core-feature-taxonomy-gamecalls |
 | Similarity & Finalize Metrics | #5-similarity--finalize-metrics |
 | Coaching Feedback System | #6-coaching-feedback-roadmap |
 | Advanced Analysis Components | #7-advanced-analysis-pitch-harmonic-cadence |
 | Performance Targets | #8-performance-budgets-audio |
 | Telemetry Events | #10-telemetry-implemented |
 | Calibration & Normalization | #11-calibration-normalization-plan |
 | Risk Register | #13-risk-register-active |
 | Security & Privacy Guardrails | #14-privacy-guardrails |
 | Action Items | #17-action-items-active--gamecalls |
 | Phase Exit Criteria | #18-phase-exit-criteria-gamecalls |
 | Post-MVP Backlog | #19-post-mvp-backlog-deferred |
 | Governance Rules | #20-governance--change-control |

 ---

 ## Governance

 - No status percentages or timing details live here.
 - Implementation order, acceptance criteria, and risk dispositions reside exclusively in `mvp_todo.md`.
 - Any capability listed here must appear in `mvp_todo.md` before development.
 - No duplication of metrics, telemetry schemas, or risk details—single source remains `mvp_todo.md`.
 - Modifications to this roadmap require corresponding governance acknowledgment in `mvp_todo.md` (change control).

 ---

_End of high-level roadmap. Refer to `mvp_todo.md` for all operational details._

---

## Analysis & Calibration (Planned)

Purpose
- Strengthen deterministic analysis with device/environment-aware calibration and transparent “why” factors. Engine-only scope; orchestration handles security/UX/CI.

Scope (Engine Additive, Non-Breaking)
- Microphone Calibration & Gain Advisor
 - Detect clipping/too‑quiet, estimate noise floor, compute headroom, suggest input gain presets.
 - Summary (planned): peakLevelDbFS, rmsDbFS, noiseFloorDbFS, headroomDb, recommendation {increase|decrease|ok}.
- Latency & Clock‑Drift Calibration
 - One‑time loopback/clap test to estimate I/O latency and drift (ppm); compensate in DTW alignment window.
 - Session fields (planned): alignmentOffsetMs, driftPpm applied to DTWComparator banding.
- Environment Profiler (optional NR)
 - Ambient profile capture + VAD‑aware normalization hooks. Optional RNNoise behind build/runtime flags; disabled by default.
 - Summary (planned): noiseFloorDbFS, spectralTilt, bandEnergy{low,mid,high}.
- Multi‑Dimensional Confidence and “Why” Breakdown
 - Expose per‑metric contributions (mfcc, pitch, harmonic, cadence, loudness) and short factor strings that explain score composition; sums within tolerance to total.

Acceptance Signals
- Deterministic unit tests cover: zero/quiet/loud bounds; synthetic offset/drift compensation; ambient profile improving VAD false‑positive rate on quiet scenes; contributions sum within epsilon.
- JSON exports include new fields; backward compatible with existing consumers.
- Performance unchanged in streaming hot path; calibration tasks run off critical path or with negligible overhead.

Dependencies & Risks
- Optional NR depends on an external library; keep hard‑off by default with deterministic fallback.
- DTW band adjustments must preserve existing correctness tests.

Governance
- Status/acceptance tracked in `docs/mvp_todo.md`. This section is vision/roadmap only.
