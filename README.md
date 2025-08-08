# Huntmaster Audio Engine

A real-time audio processing engine for wildlife call analysis and recognition.

## Project Structure

```
huntmaster-engine/
├── src/                    # Source code
├── include/               # Header files
├── tests/                 # Organized test suite
│   ├── unit/             # Unit tests by category
│   │   ├── core/         # Core engine tests
│   │   ├── audio/        # Audio processing tests
│   │   ├── analysis/     # Analysis algorithm tests
│   │   ├── vad/          # Voice Activity Detection tests
│   │   ├── security/     # Security tests
│   │   └── utils/        # Utility tests
│   ├── integration/      # Integration tests
│   ├── performance/      # Performance benchmarks
│   ├── tools/           # Test tools and utilities
│   └── lib/             # Test libraries
├── docs/                 # Documentation
│   ├── alpha_testing/    # Alpha testing documentation
│   ├── deployment/       # Deployment guides
│   ├── development/      # Development documentation
│   └── status_reports/   # Project status reports
├── scripts/              # Build and utility scripts
│   ├── build/           # Build scripts
│   ├── testing/         # Testing scripts
│   └── deployment/      # Deployment scripts
├── data/                # Test data and audio files
├── web/                 # Web interface
├── bindings/            # Language bindings
└── tools/               # Development tools
```

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Testing

```bash
# Core functionality tests (must pass)
make test_core

# Quick test suite
make test_quick

# All unit tests
make test_unit

# Complete test suite
make test_all
```

## Core Tests Status

The core functionality tests ensure the engine's basic operations are working:
- Audio Processing: Feature extraction and similarity scoring
- Session Management: Session lifecycle and isolation
- Master Call Management: Loading and processing reference calls
- Recording System: Audio recording and playback

## License

See LICENSE file for details.
