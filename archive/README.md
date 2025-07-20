# Legacy Code Archive

This directory contains archived legacy code that has been replaced by the modern UnifiedAudioEngine API.

## Directory Structure

```
archive/
├── legacy/
│   └── core/
│       ├── HuntmasterEngine.h           # Original legacy engine header
│       ├── HuntmasterEngine.cpp         # Original legacy engine implementation
│       ├── HuntmasterAudioEngine.h      # Deprecated singleton-based engine header
│       ├── HuntmasterAudioEngine.cpp    # Deprecated singleton-based engine implementation
│       ├── IHuntmasterEngine.h          # Interface for original engine
│       └── Platform.h                   # Platform definitions for original engine
└── README.md                            # This file
```

## Archived Components

### HuntmasterEngine (v3.x) - Original Legacy

- **Archived on**: July 20, 2025
- **Reason**: Replaced by UnifiedAudioEngine for better architecture
- **Migration Status**: Complete - all functionality ported to UnifiedAudioEngine
- **Dependencies**:
  - Originally used by WASM interface (migrated)
  - No active test dependencies found
- **Original Location**:
  - `include/huntmaster/core/HuntmasterEngine.h`
  - `src/core/HuntmasterEngine.cpp`

### HuntmasterAudioEngine (v4.0) - Deprecated Singleton

- **Archived on**: July 20, 2025
- **Reason**: Deprecated singleton pattern replaced by modern session-based UnifiedAudioEngine
- **Migration Status**: Complete - all tests migrated to UnifiedAudioEngine
- **Dependencies**:
  - Used by 7 test files (all migrated)
  - Build system updated to use UnifiedAudioEngine
- **Original Location**:
  - `include/huntmaster/core/HuntmasterAudioEngine.h`
  - `src/core/HuntmasterAudioEngine.cpp`

### IHuntmasterEngine & Platform.h - Original Interfaces

- **Archived on**: July 20, 2025
- **Reason**: Interfaces for original engine no longer needed
- **Migration Status**: Complete - replaced by modern UnifiedAudioEngine interfaces
- **Original Location**:
  - `include/huntmaster/core/IHuntmasterEngine.h`
  - `include/huntmaster/core/Platform.h`

## Migration Summary

The legacy HuntmasterEngine has been fully replaced by the modern UnifiedAudioEngine API:

### Key Improvements in UnifiedAudioEngine:

1. **Session-based Architecture**: Better resource management with explicit session lifecycle
2. **Thread Safety**: Improved concurrency handling and atomic operations
3. **Error Handling**: Consistent Status enum across all operations
4. **Factory Pattern**: Clean instantiation through UnifiedAudioEngineFactory
5. **RAII Compliance**: Proper resource cleanup and exception safety

### API Migration:

- `HuntmasterEngine::loadAudio()` → `UnifiedAudioEngine::loadMasterCall()`
- `HuntmasterEngine::processAudio()` → `UnifiedAudioEngine::processRealtimeAudio()` (session-based)
- Direct instantiation → Factory pattern with `UnifiedAudioEngineFactory::createEngine()`
- Manual lifecycle → Session management with `startRealtimeSession()`/`endRealtimeSession()`

### Components Successfully Migrated:

- ✅ WASM Interface (WASMInterface.h/cpp)
- ✅ All test suites (121 tests passing)
- ✅ Build system (CMakeLists.txt)

## Usage Notes

**⚠️ These archived files should NOT be used in new development.**

For reference purposes only. All new development should use the UnifiedAudioEngine API.

## Restoration Process

If restoration is ever needed (not recommended):

1. Copy files back to their original locations
2. Update CMakeLists.txt to include HuntmasterEngine target
3. Revert WASM interface to use legacy API
4. Update any dependencies that may have changed

## Contact

For questions about the migration or archived code, refer to:

- Migration documentation in `/docs/`
- UnifiedAudioEngine API documentation
- Git history for detailed change tracking
