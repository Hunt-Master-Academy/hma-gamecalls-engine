// File: RealtimeAudioProcessor.h
#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <optional>
#include <queue>
#include <span>
#include <vector>

#include "Expected.h"

namespace huntmaster {

enum class ProcessorError {
    BUFFER_FULL,
    BUFFER_EMPTY,
    INVALID_SIZE,
    NOT_INITIALIZED,
    PROCESSING_FAILED
};

struct AudioChunk {
    static constexpr size_t MAX_CHUNK_SIZE = 2048;

    std::array<float, MAX_CHUNK_SIZE> data{};
    size_t valid_samples{0};
    std::chrono::steady_clock::time_point timestamp;
    size_t frame_index{0};
    float energy_level{0.0f};
    bool contains_voice{false};
};

struct ProcessorStats {
    size_t total_chunks_processed{0};
    size_t chunks_dropped{0};
    size_t buffer_overruns{0};
    size_t buffer_underruns{0};
    std::chrono::nanoseconds total_processing_time{0};
    std::chrono::nanoseconds max_processing_time{0};
    float average_latency_ms{0.0f};
    size_t current_buffer_usage{0};
};

class RealtimeAudioProcessor {
  public:
    struct Config {
        size_t ring_buffer_size{1024};  // Must be power of 2
        size_t chunk_size{512};
        bool enable_backpressure{true};
        std::chrono::milliseconds backpressure_timeout{10};
        bool enable_metrics{true};
        size_t high_water_mark{768};  // 75% full
        size_t low_water_mark{256};   // 25% full
    };

    explicit RealtimeAudioProcessor(const Config& config);
    ~RealtimeAudioProcessor();

    RealtimeAudioProcessor(RealtimeAudioProcessor&&) noexcept;
    RealtimeAudioProcessor& operator=(RealtimeAudioProcessor&&) noexcept;

    // Producer interface (audio input thread)
    [[nodiscard]] huntmaster::expected<void, ProcessorError>
    enqueueAudio(std::span<const float> audio_data);

    [[nodiscard]] bool tryEnqueueAudio(std::span<const float> audio_data);

    // Consumer interface (processing thread)
    [[nodiscard]] huntmaster::expected<AudioChunk, ProcessorError> dequeueChunk();

    [[nodiscard]] std::optional<AudioChunk> tryDequeueChunk();

    // Batch operations
    [[nodiscard]] size_t enqueueBatch(std::span<const std::span<const float>> audio_batches);

    [[nodiscard]] std::vector<AudioChunk> dequeueBatch(size_t max_chunks);

    // Status and control
    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] bool isFull() const noexcept;
    [[nodiscard]] size_t available() const noexcept;
    [[nodiscard]] size_t capacity() const noexcept;

    [[nodiscard]] ProcessorStats getStats() const noexcept;
    void resetStats() noexcept;

    // Backpressure support
    void waitForSpace(std::chrono::milliseconds timeout);
    void waitForData(std::chrono::milliseconds timeout);

  private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

}  // namespace huntmaster