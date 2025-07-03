// File: AudioBufferPool.cpp
#include "AudioBufferPool.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <new>

namespace huntmaster
{

    /**
     * @class AudioBufferPool::Impl
     * @brief Private implementation of the buffer pool
     */
    class AudioBufferPool::Impl
    {
    public:
        Config config_;

        // Memory management
        std::pmr::memory_resource *memory_resource_;
        std::vector<void *> buffers_;
        std::vector<bool> in_use_;

        // Synchronization using C++20 counting semaphore
        std::counting_semaphore<> available_semaphore_;

        // Statistics tracking with atomics
        std::atomic<size_t> total_allocations_{0};
        std::atomic<size_t> failed_allocations_{0};
        std::atomic<size_t> peak_usage_{0};
        std::atomic<size_t> current_usage_{0};

        // Memory tracking
        std::atomic<size_t> total_memory_allocated_{0};

        explicit Impl(const Config &config)
            : config_(config), memory_resource_(config.memory_resource
                                                    ? config.memory_resource
                                                    : std::pmr::get_default_resource()),
              buffers_(config.pool_size, nullptr), in_use_(config.pool_size, false), available_semaphore_(static_cast<std::ptrdiff_t>(config.pool_size))
        {

            // Validate configuration
            if (config.pool_size == 0 || config.buffer_size == 0)
            {
                throw std::invalid_argument("Invalid pool configuration");
            }

            // Ensure alignment is power of 2
            if (!std::has_single_bit(config.alignment))
            {
                throw std::invalid_argument("Alignment must be power of 2");
            }

            // Allocate all buffers upfront
            allocateBuffers();
        }

        ~Impl()
        {
            deallocateBuffers();
        }

        /**
         * @brief Allocate all buffers in the pool
         */
        void allocateBuffers()
        {
            // Calculate aligned buffer size
            const size_t aligned_size = alignUp(config_.buffer_size, config_.alignment);

            for (size_t i = 0; i < config_.pool_size; ++i)
            {
                try
                {
                    // Allocate aligned memory
                    void *buffer = memory_resource_->allocate(
                        aligned_size,
                        config_.alignment);

                    if (!buffer)
                    {
                        throw std::bad_alloc();
                    }

                    // Zero-initialize the buffer
                    std::memset(buffer, 0, aligned_size);

                    buffers_[i] = buffer;
                    total_memory_allocated_.fetch_add(aligned_size, std::memory_order_relaxed);
                }
                catch (...)
                {
                    // Clean up any allocated buffers
                    deallocateBuffers();
                    throw;
                }
            }
        }

        /**
         * @brief Deallocate all buffers in the pool
         */
        void deallocateBuffers()
        {
            const size_t aligned_size = alignUp(config_.buffer_size, config_.alignment);

            for (size_t i = 0; i < buffers_.size(); ++i)
            {
                if (buffers_[i])
                {
                    memory_resource_->deallocate(
                        buffers_[i],
                        aligned_size,
                        config_.alignment);
                    buffers_[i] = nullptr;
                }
            }

            total_memory_allocated_.store(0, std::memory_order_relaxed);
        }

        /**
         * @brief Find the next available buffer
         * @return Buffer index or nullopt if none available
         */
        [[nodiscard]] std::optional<size_t> findAvailableBuffer()
        {
            // Simple linear search - could be optimized with bitset
            for (size_t i = 0; i < in_use_.size(); ++i)
            {
                bool expected = false;
                if (in_use_[i].compare_exchange_strong(expected, true,
                                                       std::memory_order_acquire,
                                                       std::memory_order_relaxed))
                {
                    return i;
                }
            }
            return std::nullopt;
        }

        /**
         * @brief Mark a buffer as available
         * @param index Buffer index to release
         */
        void markAvailable(size_t index)
        {
            if (index < in_use_.size())
            {
                in_use_[index].store(false, std::memory_order_release);
                current_usage_.fetch_sub(1, std::memory_order_relaxed);
                available_semaphore_.release();
            }
        }

        /**
         * @brief Align a size up to the specified alignment
         */
        static constexpr size_t alignUp(size_t size, size_t alignment)
        {
            return (size + alignment - 1) & ~(alignment - 1);
        }
    };

    // BufferHandle implementation

    AudioBufferPool::BufferHandle::BufferHandle(AudioBufferPool *pool, void *buffer, size_t index)
        : pool_(pool), buffer_(buffer), index_(index)
    {
    }

    AudioBufferPool::BufferHandle::~BufferHandle()
    {
        if (pool_ && buffer_)
        {
            pool_->pimpl_->markAvailable(index_);
        }
    }

    AudioBufferPool::BufferHandle::BufferHandle(BufferHandle &&other) noexcept
        : pool_(std::exchange(other.pool_, nullptr)), buffer_(std::exchange(other.buffer_, nullptr)), index_(std::exchange(other.index_, 0))
    {
    }

    AudioBufferPool::BufferHandle &
    AudioBufferPool::BufferHandle::operator=(BufferHandle &&other) noexcept
    {
        if (this != &other)
        {
            // Release current buffer if any
            if (pool_ && buffer_)
            {
                pool_->pimpl_->markAvailable(index_);
            }

            pool_ = std::exchange(other.pool_, nullptr);
            buffer_ = std::exchange(other.buffer_, nullptr);
            index_ = std::exchange(other.index_, 0);
        }
        return *this;
    }

    std::span<float> AudioBufferPool::BufferHandle::data() noexcept
    {
        if (!buffer_)
            return {};
        return std::span<float>(
            static_cast<float *>(buffer_),
            pool_->pimpl_->config_.buffer_size / sizeof(float));
    }

    std::span<const float> AudioBufferPool::BufferHandle::data() const noexcept
    {
        if (!buffer_)
            return {};
        return std::span<const float>(
            static_cast<const float *>(buffer_),
            pool_->pimpl_->config_.buffer_size / sizeof(float));
    }

    std::span<std::byte> AudioBufferPool::BufferHandle::bytes() noexcept
    {
        if (!buffer_)
            return {};
        return std::span<std::byte>(
            static_cast<std::byte *>(buffer_),
            pool_->pimpl_->config_.buffer_size);
    }

    std::span<const std::byte> AudioBufferPool::BufferHandle::bytes() const noexcept
    {
        if (!buffer_)
            return {};
        return std::span<const std::byte>(
            static_cast<const std::byte *>(buffer_),
            pool_->pimpl_->config_.buffer_size);
    }

    size_t AudioBufferPool::BufferHandle::size() const noexcept
    {
        if (!buffer_ || !pool_)
            return 0;
        return pool_->pimpl_->config_.buffer_size / sizeof(float);
    }

    float *AudioBufferPool::BufferHandle::begin() noexcept
    {
        return buffer_ ? static_cast<float *>(buffer_) : nullptr;
    }

    float *AudioBufferPool::BufferHandle::end() noexcept
    {
        return buffer_ ? static_cast<float *>(buffer_) + size() : nullptr;
    }

    const float *AudioBufferPool::BufferHandle::begin() const noexcept
    {
        return buffer_ ? static_cast<const float *>(buffer_) : nullptr;
    }

    const float *AudioBufferPool::BufferHandle::end() const noexcept
    {
        return buffer_ ? static_cast<const float *>(buffer_) + size() : nullptr;
    }

    // AudioBufferPool implementation

    AudioBufferPool::AudioBufferPool(const Config &config)
        : pimpl_(std::make_unique<Impl>(config))
    {
    }

    AudioBufferPool::AudioBufferPool(size_t pool_size, size_t buffer_size)
        : AudioBufferPool(Config{.pool_size = pool_size, .buffer_size = buffer_size})
    {
    }

    AudioBufferPool::~AudioBufferPool() = default;

    AudioBufferPool::AudioBufferPool(AudioBufferPool &&) noexcept = default;

    AudioBufferPool &AudioBufferPool::operator=(AudioBufferPool &&) noexcept = default;

    std::expected<AudioBufferPool::BufferHandle, BufferPoolError>
    AudioBufferPool::acquire()
    {
        return tryAcquireFor(pimpl_->config_.acquire_timeout);
    }

    std::expected<AudioBufferPool::BufferHandle, BufferPoolError>
    AudioBufferPool::tryAcquireFor(std::chrono::milliseconds timeout)
    {
        // Update statistics
        pimpl_->total_allocations_.fetch_add(1, std::memory_order_relaxed);

        // Try to acquire with timeout
        if (!pimpl_->available_semaphore_.try_acquire_for(timeout))
        {
            pimpl_->failed_allocations_.fetch_add(1, std::memory_order_relaxed);
            return std::unexpected(BufferPoolError::POOL_EXHAUSTED);
        }

        // Find an available buffer
        auto index_opt = pimpl_->findAvailableBuffer();
        if (!index_opt)
        {
            // This shouldn't happen if semaphore is working correctly
            pimpl_->available_semaphore_.release();
            pimpl_->failed_allocations_.fetch_add(1, std::memory_order_relaxed);
            return std::unexpected(BufferPoolError::ALLOCATION_FAILED);
        }

        size_t index = *index_opt;
        void *buffer = pimpl_->buffers_[index];

        // Update usage statistics
        size_t current = pimpl_->current_usage_.fetch_add(1, std::memory_order_relaxed) + 1;

        // Update peak usage if necessary
        size_t peak = pimpl_->peak_usage_.load(std::memory_order_relaxed);
        while (current > peak)
        {
            if (pimpl_->peak_usage_.compare_exchange_weak(peak, current,
                                                          std::memory_order_relaxed,
                                                          std::memory_order_relaxed))
            {
                break;
            }
        }

        return BufferHandle(this, buffer, index);
    }

    void AudioBufferPool::release(BufferHandle &&handle)
    {
        // BufferHandle destructor will handle the release
        handle = BufferHandle{};
    }

    BufferPoolStats AudioBufferPool::getStats() const noexcept
    {
        return BufferPoolStats{
            .total_buffers = pimpl_->config_.pool_size,
            .available_buffers = available(),
            .peak_usage = pimpl_->peak_usage_.load(std::memory_order_relaxed),
            .total_allocations = pimpl_->total_allocations_.load(std::memory_order_relaxed),
            .failed_allocations = pimpl_->failed_allocations_.load(std::memory_order_relaxed),
            .current_memory_usage = pimpl_->total_memory_allocated_.load(std::memory_order_relaxed)};
    }

    size_t AudioBufferPool::available() const noexcept
    {
        size_t in_use = pimpl_->current_usage_.load(std::memory_order_relaxed);
        return pimpl_->config_.pool_size - in_use;
    }

    void AudioBufferPool::resetStats() noexcept
    {
        pimpl_->total_allocations_.store(0, std::memory_order_relaxed);
        pimpl_->failed_allocations_.store(0, std::memory_order_relaxed);
        pimpl_->peak_usage_.store(pimpl_->current_usage_.load(std::memory_order_relaxed),
                                  std::memory_order_relaxed);
    }

} // namespace huntmaster