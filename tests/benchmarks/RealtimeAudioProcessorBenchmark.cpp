#include <benchmark/benchmark.h>
#include "huntmaster/core/RealtimeAudioProcessor.h"

using namespace huntmaster;

static void BM_EnqueueDequeue(benchmark::State& state) {
    RealtimeAudioProcessor::Config cfg;
    cfg.ring_buffer_size = state.range(0);
    cfg.chunk_size = 512;
    cfg.enable_metrics = false;
    
    RealtimeAudioProcessor proc(cfg);
    std::vector<float> data(512, 1.0f);
    
    for (auto _ : state) {
        proc.tryEnqueueAudio(data);
        auto chunk = proc.tryDequeueChunk();
        benchmark::DoNotOptimize(chunk);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_EnqueueDequeue)->RangeMultiplier(2)->Range(8, 256);