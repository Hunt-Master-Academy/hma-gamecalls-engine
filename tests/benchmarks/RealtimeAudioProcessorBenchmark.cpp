#include <benchmark/benchmark.h>

#include "huntmaster/core/RealtimeAudioProcessor.h"

using namespace huntmaster;

// Benchmark the performance of enqueuing audio data into the ring buffer.
static void BM_Enqueue(benchmark::State& state) {
    RealtimeAudioProcessor::Config cfg;
    cfg.ring_buffer_size = state.range(0);
    cfg.chunk_size = 512;
    cfg.enable_metrics = false;

    RealtimeAudioProcessor proc(cfg);
    std::vector<float> data(512, 1.0f);

    for (auto _ : state) {
        // Reset the processor state for each timing loop to ensure
        // we are measuring raw enqueue speed into an empty buffer.
        state.PauseTiming();
        proc.resetStats();
        state.ResumeTiming();

        while (proc.tryEnqueueAudio(data)) {
        }
    }
}

// Benchmark the performance of dequeuing audio data from the ring buffer.
static void BM_Dequeue(benchmark::State& state) {
    RealtimeAudioProcessor::Config cfg;
    cfg.ring_buffer_size = state.range(0);
    cfg.chunk_size = 512;
    cfg.enable_metrics = false;

    RealtimeAudioProcessor proc(cfg);
    std::vector<float> data(512, 1.0f);

    for (auto _ : state) {
        // Pre-fill the buffer so we always have something to dequeue.
        // This setup work is done with timing paused.
        state.PauseTiming();
        (void)proc.tryEnqueueAudio(data);
        state.ResumeTiming();

        // The operation we are timing.
        auto chunk = proc.tryDequeueChunk();
        benchmark::DoNotOptimize(chunk);
    }

    state.SetItemsProcessed(state.iterations());
}

// Benchmark the combined enqueue/dequeue latency (round trip).
// This is a good measure of the processor's overall throughput.
static void BM_RoundTrip(benchmark::State& state) {
    RealtimeAudioProcessor::Config cfg;
    cfg.ring_buffer_size = state.range(0);
    cfg.chunk_size = 512;
    cfg.enable_metrics = false;

    RealtimeAudioProcessor proc(cfg);
    std::vector<float> data(512, 1.0f);

    for (auto _ : state) {
        (void)proc.tryEnqueueAudio(data);
        auto chunk = proc.tryDequeueChunk();
        benchmark::DoNotOptimize(chunk);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_Enqueue)->RangeMultiplier(2)->Range(8, 256);
BENCHMARK(BM_Dequeue)->RangeMultiplier(2)->Range(8, 256);
BENCHMARK(BM_RoundTrip)->RangeMultiplier(2)->Range(8, 256);