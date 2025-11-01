# C++ Engine Segfault Investigation

## Problem
Node-API bindings segfault on module load before any JavaScript code executes.

## Root Cause
Static initialization issue in C++ engine, likely in `DebugLogger::getInstance()` singleton.

## Evidence
1. Standalone C++ tools work perfectly (generate_features, etc.)
2. Segfault happens during `require()` before any addon functions are called
3. All library dependencies are satisfied (ldd shows no missing libs)
4. DebugLogger uses static singleton pattern that may not be Node-API safe

## Solution Options
1. **Fix static initialization** - Refactor DebugLogger to lazy-initialize or use Node-API lifecycle
2. **Disable logging in bindings** - Add compile flag to skip LOG_* macros
3. **Use shared library** - Build engine as .so instead of static .a to isolate initialization

## Workaround (Current)
Use mock engine for V1.0 MVP, fix real engine for V1.1.

## Files Involved
- `/bindings/node-api/src/*.cc` - Node-API bindings
- `/src/core/UnifiedAudioEngine.cpp` - Engine using LOG_* macros
- `/include/huntmaster/core/DebugLogger.h` - Singleton logger

## Next Steps
1. Add NAPI_SAFE_LOGGING compile flag
2. Wrap LOG_* macros in #ifdef
3. Test with logging disabled
4. If that works, make logging optional via CMake option
