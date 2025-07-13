# Huntmaster Audio Engine

> ⚠️ **License: All Rights Reserved**
>
> The source code for this project is made available for **portfolio review and evaluation purposes only.** It is not licensed for use of any kind, commercial or non-commercial, without express written permission from the author. For licensing inquiries, please contact time@3dtechsolutions.us.

---

The Huntmaster Audio Engine is a cross-platform C++ audio analysis engine designed to be the core of the Huntmaster platform. Its primary function is to analyze and compare audio recordings of animal calls, such as deer grunts or turkey gobbles, by providing a similarity score. It is built to be a self-contained, high-performance static library that can be easily integrated into various client applications (desktop, mobile, etc.).

## 🚧 Project Status: In Development

This engine is currently under active development for its Minimum Viable Product (MVP). The core architecture is in place, and development is focused on implementing the master call processing pipeline.

### Phase 1: Game Calls MVP

✅ Sprint 1: Environment & Core Dependencies (100%)

🚧 Sprint 2: Master Call Pipeline & Feature Extraction (50%)

⏳ Sprint 3: Real-time Processing & DTW Comparison (0%)

⏳ Sprint 4: API Implementation & Unit Testing (25%)

⏳ Sprint 5: Build System & Integration Prep (25%)

## ✨ Features

MFCC Extraction: Converts raw audio signals into Mel-Frequency Cepstral Coefficients, a standard representation for audio feature analysis.

Offline Processing: Designed to pre-process and store feature data from master call audio files.

Real-time Analysis: (In Progress) Will support processing live audio chunks from a microphone.

Audio Comparison: (In Progress) Will use Dynamic Time Warping (DTW) to compare the features of a user's call against a master call.

Cross-Platform: Built with CMake to support compilation across different operating systems.

Static Library: Designed to be compiled into a .lib or .a file for easy integration with other projects.

## Dependencies

This project uses git submodules for external dependencies:

- **KissFFT**: Fast Fourier Transform library (`libs/kissfft`)
- **GoogleTest**: Testing framework (`tests/lib/googletest`)
- **Emscripten SDK**: WebAssembly compilation (`tools/emsdk`)

### Cloning with Submodules

```bash
git clone --recursive https://github.com/tescolopio/huntmaster-engine.git

# Or if already cloned:
git submodule update --init --recursive
```
