# CONTRIBUTING.md

## Overview
This repository contains C++ game engine components for Hunt Master Academy's Game Calls pillar.

## Development Workflow

### Prerequisites
- CMake 3.20+
- C++17 compatible compiler
- Platform-specific audio/graphics libraries

### Building
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Testing
```bash
ctest --output-on-failure
```

## Code Standards

### Performance
- Profile audio processing paths
- Minimize allocations in real-time code
- Document latency requirements
- Target <50ms p95 for audio analysis

### Memory Safety
- Use RAII patterns
- Validate buffer bounds
- Enable sanitizers in debug builds
- Follow memory protection guidelines in `docs/security/`

### Audio Processing
- Handle sample rate variations
- Implement graceful degradation
- Validate input formats
- Document frequency response characteristics

## PR Guidelines
- Include performance benchmarks for audio code
- Test on target mobile platforms
- Verify memory safety with sanitizers
- Update documentation for API changes

## Architecture
- See `docs/web/` for UI component guides
- Security considerations in `docs/security/`
- Performance targets documented in component READMEs
