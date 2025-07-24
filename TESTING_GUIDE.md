# Huntmaster Audio Engine - Testing Guide

## Quick Start

This comprehensive testing framework ensures your Huntmaster Audio Engine is ready for WASM deployment.

### 🚀 One-Command Full Test

```bash
./master_test.sh
```

This runs all testing phases in sequence and generates a comprehensive report.

### 📋 Available Test Scripts

| Script                   | Purpose                                              | Usage                                       |
| ------------------------ | ---------------------------------------------------- | ------------------------------------------- |
| `master_test.sh`         | **Main orchestrator** - Runs all test phases         | `./master_test.sh [options]`                |
| `comprehensive_test.sh`  | Core engine testing (unit, integration, diagnostics) | `./comprehensive_test.sh [options]`         |
| `component_test.sh`      | Detailed component-specific testing                  | `./component_test.sh <component> [options]` |
| `pre_wasm_validation.sh` | WASM deployment readiness validation                 | `./pre_wasm_validation.sh [options]`        |

### 🎯 Testing Phases

1. **Build Phase** - Compile and verify project builds correctly
2. **Unit Tests** - Test individual components in isolation
3. **Integration Tests** - Test component interactions
4. **Component Tests** - Deep testing of audio engine, MFCC, DTW, etc.
5. **Validation Phase** - Verify WASM compatibility and readiness
6. **Performance Phase** - Benchmark and stress testing

### 🔧 Common Usage Patterns

#### Run specific test phase:

```bash
./master_test.sh --phase=build         # Just build and compile
./master_test.sh --phase=unit          # Just unit tests
./master_test.sh --phase=validation    # Just WASM validation
```

#### Test specific components:

```bash
./component_test.sh engine             # Test audio engine
./component_test.sh mfcc               # Test MFCC processing
./component_test.sh dtw                # Test DTW comparison
./component_test.sh audio              # Test audio I/O
```

#### Verbose output for debugging:

```bash
./master_test.sh --verbose             # See all output in real-time
./component_test.sh engine --verbose   # Verbose component testing
```

#### Continue on failures (for debugging):

```bash
./master_test.sh --continue-on-fail    # Don't stop on first failure
```

### 📊 Test Results

All test results are saved in the `test_logs/` directory:

- **Summary files**: Quick overview of pass/fail status
- **Individual logs**: Detailed output for each test phase
- **Final report**: Comprehensive markdown report with recommendations

### 🎯 Pre-WASM Validation

Before compiling to WASM, run the validation suite:

```bash
./pre_wasm_validation.sh
```

This checks:

- ✅ WASM compatibility (no unsupported features)
- ✅ Audio dependencies and file formats
- ✅ Memory usage patterns
- ✅ Core functionality validation
- ✅ Performance benchmarks

### 🔍 Troubleshooting

#### Build Issues

```bash
# Clean and rebuild
rm -rf build/
./master_test.sh --phase=build --verbose
```

#### Audio Issues

```bash
# Test audio components specifically
./component_test.sh audio --verbose
```

#### MFCC/DTW Issues

```bash
# Run diagnostic tools
./tools/debug_dtw_similarity
./component_test.sh mfcc --stress-test
```

#### Memory Issues

```bash
# Check for memory leaks
./component_test.sh engine --memory-check
```

### 📈 Performance Testing

```bash
# Run performance benchmarks
./master_test.sh --phase=performance

# Stress test specific components
./component_test.sh engine --stress-test
./component_test.sh realtime --stress-test
```

### 🔄 Continuous Integration

For automated testing in CI/CD:

```bash
# Non-interactive mode with XML output
./master_test.sh --phase=all --continue-on-fail > test_results.log 2>&1
```

### 📝 Custom Testing

You can also run individual test executables:

```bash
# Run specific test executables
./build/tests/RunEngineTests           # Unit tests
./build/debug_dtw_test                 # DTW debugging
./tools/generate_features              # Feature extraction
./tools/analyze_recording              # Audio analysis
```

### 🎉 Ready for WASM?

When all tests pass, you'll see:

```
🎉 All test phases completed successfully! Engine is ready for WASM deployment.
```

Then proceed with:

```bash
./scripts/build_wasm.sh
```

### 📞 Need Help?

1. Check the generated test reports in `test_logs/`
2. Run with `--verbose` for detailed output
3. Look at individual component logs for specific issues
4. Use `--continue-on-fail` to see all issues at once

---

**Happy Testing! 🧪**
