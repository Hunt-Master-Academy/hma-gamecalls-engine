// File: AudioBufferPool.h
#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <memory_resource>
#include <optional>
#include <semaphore>
#include <span>
#include <vector>

#include "Expected.h"

namespace huntmaster {

/**
 * @enum BufferPoolError
 * @brief Error codes specific to buffer pool operations
 */
enum class BufferPoolError {
    POOL_EXHAUSTED,
    ALLOCATION_FAILED,
    INVALID_CONFIGURATION,
    INVALID_ALIGNMENT,
    OUT_OF_MEMORY
};

/**
 * @struct BufferPoolStats
 * @brief Runtime statistics for buffer pool monitoring
 */
struct BufferPoolStats {
    size_t total_buffers;
    size_t available_buffers;
    size_t peak_usage;
    size_t total_allocations;
    size_t failed_allocations;
    size_t current_memory_usage;
};

/**
 * @class AudioBufferPool
 * @brief Lock-free buffer pool for efficient audio buffer management
 *
 * This class provides a thread-safe, lock-free buffer pool optimized for
 * real-time audio processing. It uses C++20 counting semaphores for
 * availability tracking and custom memory resources for allocation.
 */
class AudioBufferPool {
  public:
    /**
     * @brief Configuration for buffer pool initialization
     */
    struct Config {
        size_t pool_size{32};      // Number of buffers in pool (must be > 0; invalid if 0)
        size_t buffer_size{4096};  // Size of each buffer in bytes (must be > 0; invalid if 0)
        size_t alignment{64};      // Memory alignment (cache line; must be power of two and >=
                                   // sizeof(float); invalid otherwise)
        std::pmr::memory_resource* memory_resource{
            nullptr};  // Custom allocator (nullptr for default)
        std::chrono::milliseconds acquire_timeout{
            100};  // Acquisition timeout (must be non-negative)
    };

    /**
     * @brief Constructs a buffer pool with the specified configuration
     * @param config Pool configuration parameters
     */
    explicit AudioBufferPool(const Config& config);

    /**
     * @brief Alternative constructor for backward compatibility
     * @param pool_size Number of buffers in the pool
     * @param buffer_size Size of each buffer in bytes
     *
     * Uses default alignment (64 bytes) and the default memory resource.
     */
    AudioBufferPool(size_t pool_size, size_t buffer_size);

    /**
     * @brief Destructor - ensures all buffers are properly deallocated
     */
    ~AudioBufferPool();

    // Delete copy operations, allow move
    AudioBufferPool(const AudioBufferPool&) = delete;
    AudioBufferPool& operator=(const AudioBufferPool&) = delete;
    AudioBufferPool(AudioBufferPool&&) noexcept;
    AudioBufferPool& operator=(AudioBufferPool&&) noexcept;

    /**
     * @brief Buffer handle for RAII-style buffer management
     */
    class BufferHandle {
      public:
        BufferHandle() = default;                    // Creates an empty (invalid) handle
        ~BufferHandle();                             // Releases buffer back to pool if still owned
        BufferHandle(const BufferHandle&) = delete;  // Non-copyable
        BufferHandle& operator=(const BufferHandle&) = delete;
        BufferHandle(BufferHandle&&) noexcept;  // Movable
        BufferHandle& operator=(BufferHandle&&) noexcept;

        [[nodiscard]] std::span<float> data() noexcept;
        [[nodiscard]] std::span<const float> data() const noexcept;
        [[nodiscard]] std::span<std::byte> bytes() noexcept;
        [[nodiscard]] std::span<const std::byte> bytes() const noexcept;
        [[nodiscard]] size_t size() const noexcept;  // size in float samples
        [[nodiscard]] bool valid() const noexcept {
            return buffer_ != nullptr;
        }
        [[nodiscard]] explicit operator bool() const noexcept {
            return valid();
        }
        [[nodiscard]] float* begin() noexcept;  // iteration support
        [[nodiscard]] float* end() noexcept;
        [[nodiscard]] const float* begin() const noexcept;
        [[nodiscard]] const float* end() const noexcept;

      private:
        friend class AudioBufferPool;  // Allow pool access to internals
        BufferHandle(AudioBufferPool* pool, void* buffer, size_t index);
        AudioBufferPool* pool_{nullptr};
        void* buffer_{nullptr};
        size_t index_{0};
    };

    /**
     * @brief Acquire a buffer from the pool
     * @return Buffer handle or error
     */
    [[nodiscard]] huntmaster::expected<BufferHandle, BufferPoolError> acquire();

    /**
     * @brief Attempt to acquire within a user-specified timeout.
     * @param timeout maximum wait duration
     * @return handle on success or POOL_EXHAUSTED / ALLOCATION_FAILED
     */
    [[nodiscard]] huntmaster::expected<BufferHandle, BufferPoolError>
    tryAcquireFor(std::chrono::milliseconds timeout);

    /**
     * @brief Explicitly release a buffer early (optional). Safe to call on moved / empty handles.
     */
    void release(BufferHandle&& handle);

    /**
     * @brief Factory method for creating an AudioBufferPool.
     * This method handles platform-specific initialization and error reporting.
     * @param config Pool configuration parameters
     * @return An expected containing a unique_ptr to the created pool or a BufferPoolError.
     */
    [[nodiscard]] static huntmaster::expected<std::unique_ptr<AudioBufferPool>, BufferPoolError>
    create(const Config& config);

    /**
     * @brief Get current pool statistics
     * @return Pool statistics snapshot
     */
    [[nodiscard]] BufferPoolStats getStats() const noexcept;

    /**
     * @brief Get the number of available buffers
     * @return Available buffer count
     */
    [[nodiscard]] size_t available() const noexcept;

    /**
     * @brief Reset pool statistics
     */
    void resetStats() noexcept;

  private:
    /**
     * @class Impl
     * @brief Internal implementation class for AudioBufferPool
     *
     * Encapsulates platform-specific details, buffer management logic, and
     * synchronization mechanisms. Used to hide implementation details from
     * the public interface (PIMPL idiom).
     */
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

}  // namespace huntmaster
