# Huntmaster Audio Engine

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

## 🚀 Getting Started

Follow these instructions to get a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

You will need the following tools installed on your system:

CMake (version 3.15 or higher)

A C++17 compliant compiler:

On Windows: MinGW-w64 (recommended to be installed via Conda for easy environment management).

On macOS/Linux: GCC or Clang.

Conda (or Miniconda): Recommended for managing the C++ toolchain on Windows.

### Dependencies

The project relies on the following third-party libraries:

KissFFT: For Fast Fourier Transform calculations. This library is included in the libs/kissfft directory and is configured by our custom CMake script.

Google Test: For unit testing. This is downloaded and configured automatically by CMake using the FetchContent module.

dr_libs (dr_wav & dr_mp3): Header-only libraries for loading audio files. They are included directly in the libs/ directory.

### Build Instructions

Clone the repository:

git clone [https://github.com/your-username/huntmaster-engine.git](https://github.com/your-username/huntmaster-engine.git)
cd huntmaster-engine

(Windows with Conda) Activate your environment:
Open a terminal and activate the Conda environment that contains your MinGW toolchain.

conda activate huntmaster_engine_env

Create and navigate to the build directory:
It is standard practice to perform an out-of-source build.

mkdir build
cd build

Configure the project with CMake:
From within the build directory, run CMake to generate the build files.

On Windows (using MinGW Makefiles from Conda):

cmake -G "MinGW Makefiles" ..

On macOS or Linux:

cmake ..

Compile the code:
After CMake has successfully configured the project, run the build command.

Using the CMake build command (recommended for all platforms):

cmake --build .

Alternatively, using make directly:

mingw32-make # On Windows with MinGW
make # On macOS/Linux

This will compile the HuntmasterEngine static library, the TestHarness executable, and the RunEngineTests executable.

🧪 Running the Tests
The project includes a suite of unit tests built with Google Test. To run them, first ensure you have built the project, then run ctest from the build directory.

cd build
ctest --verbose

This will discover and run all tests, providing detailed output on their success or failure.

🛠️ Project Structure
huntmaster-engine/
├── .gitignore # Specifies intentionally untracked files to ignore.
├── CMakeLists.txt # The root CMake build script.
├── README.md # This file.
├── include/
│ └── huntmaster_engine/
│ ├── HuntmasterAudioEngine.h # Public API for the main engine.
│ └── MFCCProcessor.h # Public API for the MFCC processor.
├── libs/
│ ├── kissfft/ # KissFFT library source and CMake file.
│ ├── dr_mp3.h # Header-only MP3 loading library.
│ └── dr_wav.h # Header-only WAV loading library.
├── src/
│ ├── HuntmasterAudioEngine.cpp # Implementation of the main engine.
│ └── MFCCProcessor.cpp # Implementation of the MFCC processor.
├── tests/
│ ├── engine_tests.cpp # Unit tests for the main engine.
│ └── mfcc_tests.cpp # Unit tests for the MFCC processor.
└── tools/
└── test_harness.cpp # A simple command-line executable for testing the engine.

🤝 Contributing
Contributions are welcome! Please follow these steps to contribute:

Fork the repository.

Create a new branch for your feature or bug fix (git checkout -b feature/my-new-feature).

Make your changes and commit them (git commit -am 'Add some feature').

Push to the branch (git push origin feature/my-new-feature).

Create a new Pull Request.
