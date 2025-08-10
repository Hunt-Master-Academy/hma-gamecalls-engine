#include <cmath>
#include <memory>
#include <vector>

#include <benchmark/benchmark.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

static void BM_CreateEngine(benchmark::State& state) {
    for (auto _ : state) {
        auto engineResult = UnifiedAudioEngine::create();
        benchmark::DoNotOptimize(engineResult);
        if (!engineResult.isOk())
            state.SkipWithError("Engine creation failed");
    }
}
BENCHMARK(BM_CreateEngine);

static void BM_ProcessSilentChunk(benchmark::State& state) {
    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        state.SkipWithError("Engine create fail");
        return;
    }
    auto engine = std::move(engineResult.value);
    auto sessionResult = engine->createSession(44100.0f);
    if (!sessionResult.isOk()) {
        state.SkipWithError("Session create fail");
        return;
    }
    SessionId session = sessionResult.value;

    std::vector<float> buffer(state.range(0), 0.0f);
    std::span<const float> spanBuf(buffer.data(), buffer.size());

    for (auto _ : state) {
        auto status = engine->processAudioChunk(session, spanBuf);
        if (status != UnifiedAudioEngine::Status::OK) {
            state.SkipWithError("processAudioChunk failed");
            break;
        }
    }
    auto destroyStatus = engine->destroySession(session);
    (void)destroyStatus;  // Intentional ignore; benchmark teardown
}
BENCHMARK(BM_ProcessSilentChunk)->Arg(256)->Arg(512)->Arg(1024);

BENCHMARK_MAIN();
