// File: RealtimeAudioProcessor.cpp
#include "huntmaster/core/RealtimeAudioProcessor.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <mutex>
#include <numeric>
#include <queue>
#include <thread>

#include "huntmaster/core/DebugLogger.h"

// Enable debug output for RealtimeAudioProcessor
#define DEBUG_REALTIME_PROCESSOR 0

namespace huntmaster {

class RealtimeAudioProcessor::Impl {
   public:
    Config config_;

    // Performance metrics
    alignas(64) std::atomic<size_t> total_chunks_{0};
    alignas(64) std::atomic<size_t> dropped_chunks_{0};
    alignas(64) std::atomic<size_t> overruns_{0};
    alignas(64) std::atomic<size_t> underruns_{0};
    alignas(64) std::atomic<std::chrono::nanoseconds::rep> total_processing_ns_{0};
    alignas(64) std::atomic<std::chrono::nanoseconds::rep> max_processing_ns_{0};

    // Frame counter
    std::atomic<size_t> frame_counter_{0};

#if HUNTMASTER_SINGLE_THREADED
    // ========================================================================
    // --- SINGLE-THREADED IMPLEMENTATION (Simpler, std::queue) ---
    // ========================================================================
    std::queue<AudioChunk> queue_;
    mutable std::mutex mutex_;  // A simple mutex for basic thread safety

    explicit Impl(const Config& config) : config_(config) {}

    [[nodiscard]] huntmaster::expected<void, ProcessorError> enqueue(
        std::span<const float> audio_data) {
        if (audio_data.size() > AudioChunk::MAX_CHUNK_SIZE) {
#if DEBUG_REALTIME_PROCESSOR
            std::cout << "[DEBUG] Single-threaded enqueue: INVALID_SIZE - data size "
                      << audio_data.size() << " > " << AudioChunk::MAX_CHUNK_SIZE << std::endl;
#endif
            return huntmaster::unexpected(ProcessorError::INVALID_SIZE);
        }

        std::unique_lock lock(mutex_);
        if (queue_.size() >= config_.ring_buffer_size) {
            overruns_.fetch_add(1, std::memory_order_relaxed);
#if DEBUG_REALTIME_PROCESSOR
            std::cout << "[DEBUG] Single-threaded enqueue: BUFFER_FULL - queue size "
                      << queue_.size() << " >= " << config_.ring_buffer_size << std::endl;
#endif
            return huntmaster::unexpected(ProcessorError::BUFFER_FULL);
        }

        AudioChunk& chunk = queue_.emplace();  // Create chunk in place
        chunk.valid_samples = audio_data.size();
        chunk.timestamp = std::chrono::steady_clock::now();
        chunk.frame_index = frame_counter_.fetch_add(1, std::memory_order_relaxed);
        std::copy(audio_data.begin(), audio_data.end(), chunk.data.begin());

        // Calculate energy for audio metadata
        chunk.energy_level =
            std::sqrt(std::transform_reduce(audio_data.begin(), audio_data.end(), 0.0f, std::plus{},
                                            [](float x) { return x * x; }) /
                      audio_data.size());

        // Simple voice detection
        chunk.contains_voice = chunk.energy_level > 0.01f;

#if DEBUG_REALTIME_PROCESSOR
        std::cout << "[DEBUG] Single-threaded enqueue: SUCCESS - energy=" << chunk.energy_level
                  << ", voice=" << chunk.contains_voice << ", samples=" << chunk.valid_samples
                  << std::endl;
#endif

        // Update metrics
        if (config_.enable_metrics) {
            auto start_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::high_resolution_clock::now() - start_time;
            auto ns = duration.count();
            total_processing_ns_.fetch_add(ns, std::memory_order_relaxed);
            auto max_ns = max_processing_ns_.load(std::memory_order_relaxed);
            while (ns > max_ns) {
                if (max_processing_ns_.compare_exchange_weak(max_ns, ns, std::memory_order_relaxed,
                                                             std::memory_order_relaxed)) {
                    break;
                }
            }
        }

        total_chunks_.fetch_add(1, std::memory_order_relaxed);
        return {};
    }

    [[nodiscard]] huntmaster::expected<AudioChunk, ProcessorError> dequeue() {
        std::unique_lock lock(mutex_);
        if (queue_.empty()) {
            underruns_.fetch_add(1, std::memory_order_relaxed);
#if DEBUG_REALTIME_PROCESSOR
            std::cout << "[DEBUG] Single-threaded dequeue: BUFFER_EMPTY - queue is empty"
                      << std::endl;
#endif
            return huntmaster::unexpected(ProcessorError::BUFFER_EMPTY);
        }

        AudioChunk chunk = std::move(queue_.front());
        queue_.pop();
#if DEBUG_REALTIME_PROCESSOR
        std::cout << "[DEBUG] Single-threaded dequeue: SUCCESS - energy=" << chunk.energy_level
                  << ", voice=" << chunk.contains_voice << ", samples=" << chunk.valid_samples
                  << std::endl;
#endif
        return chunk;
    }

    [[nodiscard]] bool isEmpty() const noexcept {
        std::unique_lock lock(mutex_);
        return queue_.empty();
    }

    [[nodiscard]] bool isFull() const noexcept {
        std::unique_lock lock(mutex_);
        bool full = queue_.size() >= config_.ring_buffer_size;
#if DEBUG_REALTIME_PROCESSOR
        std::cout << "[DEBUG] Single-threaded isFull: " << full << " (size=" << queue_.size()
                  << ", capacity=" << config_.ring_buffer_size << ")" << std::endl;
#endif
        return full;
    }

    [[nodiscard]] size_t available() const noexcept {
        std::unique_lock lock(mutex_);
        return queue_.size();
    }

#else
    // ========================================================================
    // --- MULTI-THREADED IMPLEMENTATION (Lock-Free Ring Buffer) ---
    // ========================================================================
    // The atomicity is handled by the read/write indices, not by making the
    // large AudioChunk object itself atomic. This is much more efficient.

    std::unique_ptr<AudioChunk[]> ring_buffer_;
    alignas(64) std::atomic<size_t> write_index_{0};
    alignas(64) std::atomic<size_t> read_index_{0};
    alignas(64) mutable std::atomic<size_t> cached_write_index_{0};
    alignas(64) mutable std::atomic<size_t> cached_read_index_{0};
    alignas(64) std::atomic<size_t> item_count_{0};  // Track actual item count

    size_t buffer_mask_;

    // Backpressure support
    std::condition_variable_any cv_space_;
    std::condition_variable_any cv_data_;
    std::mutex cv_mutex_;

    explicit Impl(const Config& config) : config_(config) {
        // Ensure buffer size is power of 2
        if (!std::has_single_bit(config.ring_buffer_size)) {
#if DEBUG_REALTIME_PROCESSOR
            std::cout << "[DEBUG] Multi-threaded constructor: INVALID buffer size "
                      << config.ring_buffer_size << " - must be power of 2" << std::endl;
#endif
            throw std::invalid_argument("Ring buffer size must be power of 2");
        }

        buffer_mask_ = config.ring_buffer_size - 1;
        ring_buffer_ = std::make_unique<AudioChunk[]>(config.ring_buffer_size);
#if DEBUG_REALTIME_PROCESSOR
        std::cout << "[DEBUG] Multi-threaded constructor: SUCCESS - buffer_size="
                  << config.ring_buffer_size << ", mask=" << buffer_mask_ << std::endl;
#endif
    }

    [[nodiscard]] size_t distance(size_t from, size_t to) const noexcept {
        return (to - from) & buffer_mask_;
    }

    [[nodiscard]] bool canWrite() const noexcept {
        auto count = item_count_.load(std::memory_order_relaxed);
        bool can_write = count < config_.ring_buffer_size;

#if DEBUG_REALTIME_PROCESSOR
        std::cout << "[DEBUG] Multi-threaded canWrite: " << can_write << " - item_count=" << count
                  << ", capacity=" << config_.ring_buffer_size << std::endl;
#endif
        return can_write;
    }

    [[nodiscard]] bool canRead() const noexcept {
        auto count = item_count_.load(std::memory_order_relaxed);
        bool can_read = count > 0;

#if DEBUG_REALTIME_PROCESSOR
        std::cout << "[DEBUG] Multi-threaded canRead: " << can_read << " - item_count=" << count
                  << std::endl;
#endif
        return can_read;
    }

    [[nodiscard]] huntmaster::expected<void, ProcessorError> enqueue(
        std::span<const float> audio_data) {
        if (audio_data.size() > AudioChunk::MAX_CHUNK_SIZE) {
#if DEBUG_REALTIME_PROCESSOR
            std::cout << "[DEBUG] Multi-threaded enqueue: INVALID_SIZE - data size "
                      << audio_data.size() << " > " << AudioChunk::MAX_CHUNK_SIZE << std::endl;
#endif
            return huntmaster::unexpected(ProcessorError::INVALID_SIZE);
        }
        if (!canWrite()) {
            overruns_.fetch_add(1, std::memory_order_relaxed);
#if DEBUG_REALTIME_PROCESSOR
            std::cout << "[DEBUG] Multi-threaded enqueue: BUFFER_FULL - cannot write" << std::endl;
#endif
            return huntmaster::unexpected(ProcessorError::BUFFER_FULL);
        }

        auto write_idx = write_index_.load(std::memory_order_relaxed);
        AudioChunk& chunk = ring_buffer_[write_idx & buffer_mask_];

        chunk.valid_samples = audio_data.size();
        chunk.timestamp = std::chrono::steady_clock::now();
        chunk.frame_index = frame_counter_.fetch_add(1, std::memory_order_relaxed);
        std::copy(audio_data.begin(), audio_data.end(), chunk.data.begin());

        // Calculate energy for audio metadata
        chunk.energy_level =
            std::sqrt(std::transform_reduce(audio_data.begin(), audio_data.end(), 0.0f, std::plus{},
                                            [](float x) { return x * x; }) /
                      audio_data.size());

        // Simple voice detection
        chunk.contains_voice = chunk.energy_level > 0.01f;

#if DEBUG_REALTIME_PROCESSOR
        std::cout << "[DEBUG] Multi-threaded enqueue: SUCCESS - write_idx=" << write_idx
                  << ", energy=" << chunk.energy_level << ", voice=" << chunk.contains_voice
                  << ", samples=" << chunk.valid_samples << std::endl;
#endif

        // Update metrics
        if (config_.enable_metrics) {
            auto start_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::high_resolution_clock::now() - start_time;
            auto ns = duration.count();
            total_processing_ns_.fetch_add(ns, std::memory_order_relaxed);
            auto max_ns = max_processing_ns_.load(std::memory_order_relaxed);
            while (ns > max_ns) {
                if (max_processing_ns_.compare_exchange_weak(max_ns, ns, std::memory_order_relaxed,
                                                             std::memory_order_relaxed)) {
                    break;
                }
            }
        }

        // Advance write index
        write_index_.store((write_idx + 1) & buffer_mask_, std::memory_order_release);
        item_count_.fetch_add(1, std::memory_order_relaxed);

        total_chunks_.fetch_add(1, std::memory_order_relaxed);
        if (config_.enable_backpressure) cv_data_.notify_one();
        return {};
    }

    [[nodiscard]] huntmaster::expected<AudioChunk, ProcessorError> dequeue() {
        if (!canRead()) {
            underruns_.fetch_add(1, std::memory_order_relaxed);
#if DEBUG_REALTIME_PROCESSOR
            std::cout << "[DEBUG] Multi-threaded dequeue: BUFFER_EMPTY - cannot read" << std::endl;
#endif
            return huntmaster::unexpected(ProcessorError::BUFFER_EMPTY);
        }

        auto read_idx = read_index_.load(std::memory_order_relaxed);
        AudioChunk chunk = ring_buffer_[read_idx & buffer_mask_];
        read_index_.store((read_idx + 1) & buffer_mask_, std::memory_order_release);
        item_count_.fetch_sub(1, std::memory_order_relaxed);

#if DEBUG_REALTIME_PROCESSOR
        std::cout << "[DEBUG] Multi-threaded dequeue: SUCCESS - read_idx=" << read_idx
                  << ", energy=" << chunk.energy_level << ", voice=" << chunk.contains_voice
                  << ", samples=" << chunk.valid_samples << std::endl;
#endif

        if (config_.enable_backpressure) cv_space_.notify_one();
        return chunk;
    }

    [[nodiscard]] bool isEmpty() const noexcept { return !canRead(); }
    [[nodiscard]] bool isFull() const noexcept {
        bool full = !canWrite();
#if DEBUG_REALTIME_PROCESSOR
        std::cout << "[DEBUG] Multi-threaded isFull: " << full << std::endl;
#endif
        return full;
    }
    [[nodiscard]] size_t available() const noexcept {
        return item_count_.load(std::memory_order_relaxed);
    }
#endif
};

// ============================================================================
// --- PUBLIC INTERFACE IMPLEMENTATION ---
// ============================================================================
// This part remains the same, as it just delegates to the Pimpl.

RealtimeAudioProcessor::RealtimeAudioProcessor(const Config& config)
    : pimpl_(std::make_unique<Impl>(config)) {}
RealtimeAudioProcessor::~RealtimeAudioProcessor() = default;
RealtimeAudioProcessor::RealtimeAudioProcessor(RealtimeAudioProcessor&&) noexcept = default;
RealtimeAudioProcessor& RealtimeAudioProcessor::operator=(RealtimeAudioProcessor&&) noexcept =
    default;

huntmaster::expected<void, ProcessorError> RealtimeAudioProcessor::enqueueAudio(
    std::span<const float> audio_data) {
    return pimpl_->enqueue(audio_data);
}

bool RealtimeAudioProcessor::tryEnqueueAudio(std::span<const float> audio_data) {
    return pimpl_->enqueue(audio_data).has_value();
}

huntmaster::expected<AudioChunk, ProcessorError> RealtimeAudioProcessor::dequeueChunk() {
    return pimpl_->dequeue();
}

std::optional<AudioChunk> RealtimeAudioProcessor::tryDequeueChunk() {
    auto result = pimpl_->dequeue();
    if (result) return std::move(*result);
    return std::nullopt;
}

size_t RealtimeAudioProcessor::enqueueBatch(std::span<const std::span<const float>> audio_batches) {
    size_t enqueued = 0;
    for (const auto& batch : audio_batches) {
        if (tryEnqueueAudio(batch)) {
            ++enqueued;
        } else {
            break;
        }
    }
    return enqueued;
}

std::vector<AudioChunk> RealtimeAudioProcessor::dequeueBatch(size_t max_chunks) {
    std::vector<AudioChunk> chunks;
    chunks.reserve(std::min(max_chunks, available()));
    while (chunks.size() < max_chunks) {
        auto chunk = tryDequeueChunk();
        if (!chunk) break;
        chunks.push_back(std::move(*chunk));
    }
    return chunks;
}

bool RealtimeAudioProcessor::isEmpty() const noexcept { return pimpl_->isEmpty(); }
bool RealtimeAudioProcessor::isFull() const noexcept { return pimpl_->isFull(); }
size_t RealtimeAudioProcessor::available() const noexcept { return pimpl_->available(); }
size_t RealtimeAudioProcessor::capacity() const noexcept {
    return pimpl_->config_.ring_buffer_size;
}

ProcessorStats RealtimeAudioProcessor::getStats() const noexcept {
    ProcessorStats stats;
    stats.total_chunks_processed = pimpl_->total_chunks_.load(std::memory_order_relaxed);
    stats.chunks_dropped = pimpl_->dropped_chunks_.load(std::memory_order_relaxed);
    stats.buffer_overruns = pimpl_->overruns_.load(std::memory_order_relaxed);
    stats.buffer_underruns = pimpl_->underruns_.load(std::memory_order_relaxed);
    stats.total_processing_time =
        std::chrono::nanoseconds(pimpl_->total_processing_ns_.load(std::memory_order_relaxed));
    stats.max_processing_time =
        std::chrono::nanoseconds(pimpl_->max_processing_ns_.load(std::memory_order_relaxed));
    stats.current_buffer_usage = available();
    if (stats.total_chunks_processed > 0) {
        stats.average_latency_ms = static_cast<float>(stats.total_processing_time.count()) /
                                   stats.total_chunks_processed / 1e6f;
    }

#if DEBUG_REALTIME_PROCESSOR
    std::cout << "[DEBUG] getStats: processed=" << stats.total_chunks_processed
              << ", dropped=" << stats.chunks_dropped << ", overruns=" << stats.buffer_overruns
              << ", underruns=" << stats.buffer_underruns
              << ", total_time=" << stats.total_processing_time.count()
              << ", max_time=" << stats.max_processing_time.count() << std::endl;
#endif

    return stats;
}

void RealtimeAudioProcessor::resetStats() noexcept {
    pimpl_->total_chunks_.store(0);
    pimpl_->dropped_chunks_.store(0);
    pimpl_->overruns_.store(0);
    pimpl_->underruns_.store(0);
    pimpl_->total_processing_ns_.store(0);
    pimpl_->max_processing_ns_.store(0);
}

void RealtimeAudioProcessor::waitForSpace([[maybe_unused]] std::chrono::milliseconds timeout) {
#if !HUNTMASTER_SINGLE_THREADED
    std::unique_lock lock(pimpl_->cv_mutex_);
    pimpl_->cv_space_.wait_for(lock, timeout, [this] { return !isFull(); });
#endif
}

void RealtimeAudioProcessor::waitForData([[maybe_unused]] std::chrono::milliseconds timeout) {
#if !HUNTMASTER_SINGLE_THREADED
    std::unique_lock lock(pimpl_->cv_mutex_);
    pimpl_->cv_data_.wait_for(lock, timeout, [this] { return !isEmpty(); });
#endif
}

}  // namespace huntmaster
