// File: RealtimeAudioProcessor.cpp
#include "RealtimeAudioProcessor.h"
#include <algorithm>
#include <bit>
#include <numeric>
#include <thread>

namespace huntmaster
{

    class RealtimeAudioProcessor::Impl
    {
    public:
        Config config_;

        // Lock-free ring buffer
        alignas(64) std::array<std::atomic<AudioChunk>, 1024> ring_buffer_;
        alignas(64) std::atomic<size_t> write_index_{0};
        alignas(64) std::atomic<size_t> read_index_{0};
        alignas(64) std::atomic<size_t> cached_write_index_{0};
        alignas(64) std::atomic<size_t> cached_read_index_{0};

        size_t buffer_mask_;

        // Backpressure support
        std::condition_variable_any cv_space_;
        std::condition_variable_any cv_data_;
        std::mutex cv_mutex_;

        // Performance metrics
        alignas(64) std::atomic<size_t> total_chunks_{0};
        alignas(64) std::atomic<size_t> dropped_chunks_{0};
        alignas(64) std::atomic<size_t> overruns_{0};
        alignas(64) std::atomic<size_t> underruns_{0};
        alignas(64) std::atomic<std::chrono::nanoseconds::rep> total_processing_ns_{0};
        alignas(64) std::atomic<std::chrono::nanoseconds::rep> max_processing_ns_{0};

        // Frame counter
        std::atomic<size_t> frame_counter_{0};

        explicit Impl(const Config &config) : config_(config)
        {
            // Ensure buffer size is power of 2
            if (!std::has_single_bit(config.ring_buffer_size))
            {
                throw std::invalid_argument("Ring buffer size must be power of 2");
            }

            buffer_mask_ = config.ring_buffer_size - 1;

            // Initialize atomic chunks
            for (auto &chunk : ring_buffer_)
            {
                chunk.store(AudioChunk{}, std::memory_order_relaxed);
            }
        }

        [[nodiscard]] size_t distance(size_t from, size_t to) const noexcept
        {
            return (to - from) & buffer_mask_;
        }

        [[nodiscard]] bool canWrite() const noexcept
        {
            auto write_idx = write_index_.load(std::memory_order_relaxed);
            auto cached_read = cached_read_index_.load(std::memory_order_relaxed);

            if (distance(cached_read, write_idx) < buffer_mask_)
            {
                return true;
            }

            // Update cached read index
            auto actual_read = read_index_.load(std::memory_order_acquire);
            cached_read_index_.store(actual_read, std::memory_order_relaxed);

            return distance(actual_read, write_idx) < buffer_mask_;
        }

        [[nodiscard]] bool canRead() const noexcept
        {
            auto read_idx = read_index_.load(std::memory_order_relaxed);
            auto cached_write = cached_write_index_.load(std::memory_order_relaxed);

            if (read_idx != cached_write)
            {
                return true;
            }

            // Update cached write index
            auto actual_write = write_index_.load(std::memory_order_acquire);
            cached_write_index_.store(actual_write, std::memory_order_relaxed);

            return read_idx != actual_write;
        }

        [[nodiscard]] std::expected<void, ProcessorError>
        enqueue(std::span<const float> audio_data)
        {
            if (audio_data.size() > AudioChunk::MAX_CHUNK_SIZE)
            {
                return std::unexpected(ProcessorError::INVALID_SIZE);
            }

            if (!canWrite())
            {
                overruns_.fetch_add(1, std::memory_order_relaxed);
                return std::unexpected(ProcessorError::BUFFER_FULL);
            }

            auto start_time = config_.enable_metrics ? std::chrono::high_resolution_clock::now() : std::chrono::high_resolution_clock::time_point{};

            // Create chunk
            AudioChunk chunk;
            chunk.valid_samples = audio_data.size();
            chunk.timestamp = std::chrono::steady_clock::now();
            chunk.frame_index = frame_counter_.fetch_add(1, std::memory_order_relaxed);

            // Copy data
            std::copy(audio_data.begin(), audio_data.end(), chunk.data.begin());

            // Calculate energy
            chunk.energy_level = std::sqrt(
                std::transform_reduce(
                    audio_data.begin(), audio_data.end(), 0.0f,
                    std::plus{}, [](float x)
                    { return x * x; }) /
                audio_data.size());

            // Simple voice detection
            chunk.contains_voice = chunk.energy_level > 0.01f;

            // Store in ring buffer
            auto write_idx = write_index_.load(std::memory_order_relaxed);
            ring_buffer_[write_idx & buffer_mask_].store(chunk, std::memory_order_release);

            // Advance write index
            write_index_.store((write_idx + 1) & buffer_mask_, std::memory_order_release);

            // Update metrics
            if (config_.enable_metrics)
            {
                auto duration = std::chrono::high_resolution_clock::now() - start_time;
                auto ns = duration.count();

                total_processing_ns_.fetch_add(ns, std::memory_order_relaxed);

                auto max_ns = max_processing_ns_.load(std::memory_order_relaxed);
                while (ns > max_ns)
                {
                    if (max_processing_ns_.compare_exchange_weak(
                            max_ns, ns,
                            std::memory_order_relaxed,
                            std::memory_order_relaxed))
                    {
                        break;
                    }
                }
            }

            total_chunks_.fetch_add(1, std::memory_order_relaxed);

            // Notify waiting consumers
            if (config_.enable_backpressure)
            {
                cv_data_.notify_one();
            }

            return {};
        }

        [[nodiscard]] std::expected<AudioChunk, ProcessorError>
        dequeue()
        {
            if (!canRead())
            {
                underruns_.fetch_add(1, std::memory_order_relaxed);
                return std::unexpected(ProcessorError::BUFFER_EMPTY);
            }

            auto read_idx = read_index_.load(std::memory_order_relaxed);
            AudioChunk chunk = ring_buffer_[read_idx & buffer_mask_].load(
                std::memory_order_acquire);

            // Advance read index
            read_index_.store((read_idx + 1) & buffer_mask_, std::memory_order_release);

            // Notify waiting producers
            if (config_.enable_backpressure)
            {
                cv_space_.notify_one();
            }

            return chunk;
        }

        [[nodiscard]] size_t availableSpace() const noexcept
        {
            auto write_idx = write_index_.load(std::memory_order_acquire);
            auto read_idx = read_index_.load(std::memory_order_acquire);

            return buffer_mask_ - distance(read_idx, write_idx);
        }

        [[nodiscard]] size_t availableData() const noexcept
        {
            auto write_idx = write_index_.load(std::memory_order_acquire);
            auto read_idx = read_index_.load(std::memory_order_acquire);

            return distance(read_idx, write_idx);
        }
    };

    // Public interface implementation

    RealtimeAudioProcessor::RealtimeAudioProcessor(const Config &config)
        : pimpl_(std::make_unique<Impl>(config))
    {
    }

    RealtimeAudioProcessor::~RealtimeAudioProcessor() = default;

    RealtimeAudioProcessor::RealtimeAudioProcessor(RealtimeAudioProcessor &&) noexcept = default;

    RealtimeAudioProcessor &
    RealtimeAudioProcessor::operator=(RealtimeAudioProcessor &&) noexcept = default;

    std::expected<void, ProcessorError>
    RealtimeAudioProcessor::enqueueAudio(std::span<const float> audio_data)
    {
        return pimpl_->enqueue(audio_data);
    }

    bool RealtimeAudioProcessor::tryEnqueueAudio(std::span<const float> audio_data)
    {
        auto result = enqueueAudio(audio_data);
        return result.has_value();
    }

    std::expected<AudioChunk, ProcessorError>
    RealtimeAudioProcessor::dequeueChunk()
    {
        return pimpl_->dequeue();
    }

    std::optional<AudioChunk>
    RealtimeAudioProcessor::tryDequeueChunk()
    {
        auto result = dequeueChunk();
        if (result)
        {
            return std::move(result.value());
        }
        return std::nullopt;
    }

    size_t RealtimeAudioProcessor::enqueueBatch(
        std::span<const std::span<const float>> audio_batches)
    {
        size_t enqueued = 0;

        for (const auto &batch : audio_batches)
        {
            if (tryEnqueueAudio(batch))
            {
                ++enqueued;
            }
            else
            {
                break;
            }
        }

        return enqueued;
    }

    std::vector<AudioChunk>
    RealtimeAudioProcessor::dequeueBatch(size_t max_chunks)
    {
        std::vector<AudioChunk> chunks;
        chunks.reserve(std::min(max_chunks, available()));

        while (chunks.size() < max_chunks)
        {
            auto chunk = tryDequeueChunk();
            if (!chunk)
                break;
            chunks.push_back(std::move(*chunk));
        }

        return chunks;
    }

    bool RealtimeAudioProcessor::isEmpty() const noexcept
    {
        return pimpl_->availableData() == 0;
    }

    bool RealtimeAudioProcessor::isFull() const noexcept
    {
        return pimpl_->availableSpace() == 0;
    }

    size_t RealtimeAudioProcessor::available() const noexcept
    {
        return pimpl_->availableData();
    }

    size_t RealtimeAudioProcessor::capacity() const noexcept
    {
        return pimpl_->config_.ring_buffer_size;
    }

    ProcessorStats RealtimeAudioProcessor::getStats() const noexcept
    {
        ProcessorStats stats;

        stats.total_chunks_processed = pimpl_->total_chunks_.load(std::memory_order_relaxed);
        stats.chunks_dropped = pimpl_->dropped_chunks_.l