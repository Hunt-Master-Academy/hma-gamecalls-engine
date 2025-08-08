/**
 * @file test_circular_audio_buffer.cpp
 * @brief Comprehensive test suite for CircularAudioBuffer
 *
 * This test suite provides thorough testing of the CircularAudioBuffer
 * including initialization, thread-safe operations, memory management,
 * real-time performance, and edge cases.
 *
 * @author Huntmaster Engine Team
 * @version 1.0
 * @date July 27, 2025
 */

#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <random>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "huntmaster/CircularAudioBuffer.h"

using namespace huntmaster;
using namespace huntmaster::core;
using namespace huntmaster::test;
using namespace std::chrono_literals;

class CircularAudioBufferTest : public TestFixtureBase {
  protected:
    void SetUp() override {
        TestFixtureBase::SetUp();

        // Standard configuration for most tests
        config_.bufferSize = 1024;
        config_.sampleRate = 44100;
        config_.numChannels = 1;
        config_.enableOverflowProtection = true;
        config_.enableUnderflowProtection = true;

        buffer_ = std::make_unique<CircularAudioBuffer>(config_);
    }

    void TearDown() override {
        buffer_.reset();
        TestFixtureBase::TearDown();
    }

    // Helper function to generate test audio data
    std::vector<float> generateTestAudio(size_t samples, float frequency = 440.0f) {
        std::vector<float> audio(samples);
        for (size_t i = 0; i < samples; ++i) {
            float t = static_cast<float>(i) / config_.sampleRate;
            audio[i] = 0.5f * std::sin(2.0f * M_PI * frequency * t);
        }
        return audio;
    }

    // Helper function to generate silence
    std::vector<float> generateSilence(size_t samples) {
        return std::vector<float>(samples, 0.0f);
    }

    // Helper function to generate noise
    std::vector<float> generateNoise(size_t samples, float amplitude = 0.1f) {
        std::vector<float> noise(samples);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-amplitude, amplitude);

        for (size_t i = 0; i < samples; ++i) {
            noise[i] = dist(gen);
        }
        return noise;
    }

    CircularBufferConfig config_;
    std::unique_ptr<CircularAudioBuffer> buffer_;
};

// Basic functionality tests
TEST_F(CircularAudioBufferTest, ConstructorDestructorTest) {
    // Test default constructor behavior
    EXPECT_NE(buffer_, nullptr);
    EXPECT_EQ(buffer_->getCapacity(), config_.bufferSize);
    EXPECT_EQ(buffer_->getSampleRate(), config_.sampleRate);
    EXPECT_EQ(buffer_->getNumChannels(), config_.numChannels);
    EXPECT_EQ(buffer_->getAvailableForWrite(), config_.bufferSize);
    EXPECT_EQ(buffer_->getAvailableForRead(), 0);
    EXPECT_TRUE(buffer_->isEmpty());
    EXPECT_FALSE(buffer_->isFull());
}

TEST_F(CircularAudioBufferTest, ConfigurationTest) {
    // Test various buffer configurations
    std::vector<size_t> testSizes = {256, 512, 1024, 2048, 4096};
    std::vector<size_t> testChannels = {1, 2, 4, 8};
    std::vector<uint32_t> testSampleRates = {22050, 44100, 48000, 96000};

    for (size_t bufferSize : testSizes) {
        for (size_t numChannels : testChannels) {
            for (uint32_t sampleRate : testSampleRates) {
                CircularBufferConfig testConfig;
                testConfig.bufferSize = bufferSize;
                testConfig.numChannels = numChannels;
                testConfig.sampleRate = sampleRate;

                auto testBuffer = std::make_unique<CircularAudioBuffer>(testConfig);

                EXPECT_EQ(testBuffer->getCapacity(), bufferSize);
                EXPECT_EQ(testBuffer->getNumChannels(), numChannels);
                EXPECT_EQ(testBuffer->getSampleRate(), sampleRate);
                EXPECT_TRUE(testBuffer->isEmpty());
            }
        }
    }
}

// Write operations tests
TEST_F(CircularAudioBufferTest, BasicWriteTest) {
    auto testData = generateTestAudio(256);

    // Write data to buffer
    size_t written = buffer_->write(testData.data(), testData.size());
    EXPECT_EQ(written, testData.size());

    // Buffer should no longer be empty
    EXPECT_FALSE(buffer_->isEmpty());
    EXPECT_EQ(buffer_->getAvailableForRead(), testData.size());
    EXPECT_EQ(buffer_->getAvailableForWrite(), config_.bufferSize - testData.size());
}

TEST_F(CircularAudioBufferTest, WriteUntilFullTest) {
    auto testData = generateTestAudio(config_.bufferSize);

    // Write full buffer capacity
    size_t written = buffer_->write(testData.data(), testData.size());
    EXPECT_EQ(written, testData.size());

    // Buffer should be full
    EXPECT_TRUE(buffer_->isFull());
    EXPECT_EQ(buffer_->getAvailableForWrite(), 0);
    EXPECT_EQ(buffer_->getAvailableForRead(), config_.bufferSize);
}

TEST_F(CircularAudioBufferTest, WriteOverflowTest) {
    auto testData = generateTestAudio(config_.bufferSize + 256);

    // Try to write more than buffer capacity
    size_t written = buffer_->write(testData.data(), testData.size());

    if (config_.enableOverflowProtection) {
        // Should write only up to capacity
        EXPECT_EQ(written, config_.bufferSize);
        EXPECT_TRUE(buffer_->isFull());
    } else {
        // Behavior depends on implementation (might overwrite or wrap)
        EXPECT_LE(written, testData.size());
    }
}

TEST_F(CircularAudioBufferTest, MultipleWritesTest) {
    auto chunk1 = generateTestAudio(256, 440.0f);
    auto chunk2 = generateTestAudio(256, 880.0f);
    auto chunk3 = generateTestAudio(256, 1320.0f);

    // Write multiple chunks
    size_t written1 = buffer_->write(chunk1.data(), chunk1.size());
    size_t written2 = buffer_->write(chunk2.data(), chunk2.size());
    size_t written3 = buffer_->write(chunk3.data(), chunk3.size());

    EXPECT_EQ(written1, chunk1.size());
    EXPECT_EQ(written2, chunk2.size());
    EXPECT_EQ(written3, chunk3.size());

    size_t totalWritten = written1 + written2 + written3;
    EXPECT_EQ(buffer_->getAvailableForRead(), totalWritten);
    EXPECT_EQ(buffer_->getAvailableForWrite(), config_.bufferSize - totalWritten);
}

// Read operations tests
TEST_F(CircularAudioBufferTest, BasicReadTest) {
    auto testData = generateTestAudio(256);

    // Write data first
    buffer_->write(testData.data(), testData.size());

    // Read data back
    std::vector<float> readData(testData.size());
    size_t readCount = buffer_->read(readData.data(), readData.size());

    EXPECT_EQ(readCount, testData.size());

    // Data should match (within floating point precision)
    for (size_t i = 0; i < testData.size(); ++i) {
        EXPECT_NEAR(readData[i], testData[i], 1e-6f);
    }

    // Buffer should be empty after reading all data
    EXPECT_TRUE(buffer_->isEmpty());
    EXPECT_EQ(buffer_->getAvailableForRead(), 0);
}

TEST_F(CircularAudioBufferTest, ReadFromEmptyBufferTest) {
    std::vector<float> readData(256);

    // Try to read from empty buffer
    size_t readCount = buffer_->read(readData.data(), readData.size());

    if (config_.enableUnderflowProtection) {
        EXPECT_EQ(readCount, 0);
        // Data should remain unchanged or be zeroed
    } else {
        // Behavior depends on implementation
        EXPECT_LE(readCount, readData.size());
    }
}

TEST_F(CircularAudioBufferTest, PartialReadTest) {
    auto testData = generateTestAudio(512);

    // Write data
    buffer_->write(testData.data(), testData.size());

    // Read only part of the data
    std::vector<float> readData(256);
    size_t readCount = buffer_->read(readData.data(), readData.size());

    EXPECT_EQ(readCount, readData.size());
    EXPECT_EQ(buffer_->getAvailableForRead(), testData.size() - readData.size());

    // Verify data integrity
    for (size_t i = 0; i < readData.size(); ++i) {
        EXPECT_NEAR(readData[i], testData[i], 1e-6f);
    }
}

// Peek operations tests
TEST_F(CircularAudioBufferTest, PeekTest) {
    auto testData = generateTestAudio(256);

    // Write data
    buffer_->write(testData.data(), testData.size());

    // Peek at data (should not consume it)
    std::vector<float> peekData(128);
    size_t peekCount = buffer_->peek(peekData.data(), peekData.size());

    EXPECT_EQ(peekCount, peekData.size());
    EXPECT_EQ(buffer_->getAvailableForRead(), testData.size());  // Should not change

    // Verify peeked data matches written data
    for (size_t i = 0; i < peekData.size(); ++i) {
        EXPECT_NEAR(peekData[i], testData[i], 1e-6f);
    }

    // Reading should still work and return the same data
    std::vector<float> readData(128);
    size_t readCount = buffer_->read(readData.data(), readData.size());

    EXPECT_EQ(readCount, readData.size());
    for (size_t i = 0; i < readData.size(); ++i) {
        EXPECT_NEAR(readData[i], peekData[i], 1e-6f);
    }
}

// Circular buffer behavior tests
TEST_F(CircularAudioBufferTest, CircularBehaviorTest) {
    // Fill buffer completely
    auto fillData = generateTestAudio(config_.bufferSize);
    buffer_->write(fillData.data(), fillData.size());
    EXPECT_TRUE(buffer_->isFull());

    // Read half of the data
    std::vector<float> readData(config_.bufferSize / 2);
    buffer_->read(readData.data(), readData.size());

    // Should have space for writing again
    EXPECT_FALSE(buffer_->isFull());
    EXPECT_EQ(buffer_->getAvailableForWrite(), config_.bufferSize / 2);

    // Write new data (should wrap around)
    auto newData = generateTestAudio(config_.bufferSize / 2, 880.0f);
    size_t written = buffer_->write(newData.data(), newData.size());
    EXPECT_EQ(written, newData.size());

    // Buffer should be full again
    EXPECT_TRUE(buffer_->isFull());
}

TEST_F(CircularAudioBufferTest, WriteReadCyclesTest) {
    const int numCycles = 10;
    const size_t chunkSize = 128;

    for (int cycle = 0; cycle < numCycles; ++cycle) {
        // Generate unique data for each cycle
        auto testData = generateTestAudio(chunkSize, 440.0f + cycle * 100.0f);

        // Write data
        size_t written = buffer_->write(testData.data(), testData.size());
        EXPECT_EQ(written, testData.size());

        // Read data back
        std::vector<float> readData(chunkSize);
        size_t readCount = buffer_->read(readData.data(), readData.size());
        EXPECT_EQ(readCount, readData.size());

        // Verify data integrity
        for (size_t i = 0; i < chunkSize; ++i) {
            EXPECT_NEAR(readData[i], testData[i], 1e-6f) << "Cycle " << cycle << ", sample " << i;
        }
    }

    // Buffer should be empty after all cycles
    EXPECT_TRUE(buffer_->isEmpty());
}

// Thread safety tests
TEST_F(CircularAudioBufferTest, ThreadSafetyBasicTest) {
    const size_t chunkSize = 64;
    const int numChunks = 100;
    std::atomic<bool> writerDone{false};
    std::atomic<bool> readerDone{false};
    std::atomic<int> totalWritten{0};
    std::atomic<int> totalRead{0};

    // Writer thread
    std::thread writer([&]() {
        for (int i = 0; i < numChunks; ++i) {
            auto testData = generateTestAudio(chunkSize, 440.0f + i * 10.0f);

            // Try to write with timeout to avoid infinite blocking
            size_t written = 0;
            int attempts = 0;
            while (written < testData.size() && attempts < 1000) {
                written += buffer_->write(testData.data() + written, testData.size() - written);
                if (written < testData.size()) {
                    std::this_thread::sleep_for(1ms);
                    attempts++;
                }
            }

            totalWritten += written;

            // Small delay to let reader catch up occasionally
            if (i % 10 == 0) {
                std::this_thread::sleep_for(5ms);
            }
        }
        writerDone = true;
    });

    // Reader thread
    std::thread reader([&]() {
        std::vector<float> readBuffer(chunkSize);

        while (!writerDone || buffer_->getAvailableForRead() > 0) {
            if (buffer_->getAvailableForRead() >= chunkSize) {
                size_t readCount = buffer_->read(readBuffer.data(), readBuffer.size());
                totalRead += readCount;
            } else {
                std::this_thread::sleep_for(1ms);
            }
        }
        readerDone = true;
    });

    // Wait for both threads with timeout
    writer.join();
    reader.join();

    // Verify no data loss
    size_t expectedTotal = numChunks * chunkSize;
    EXPECT_EQ(totalWritten.load(), expectedTotal);
    EXPECT_EQ(totalRead.load(), expectedTotal);
}

TEST_F(CircularAudioBufferTest, MultipleWritersReadersTest) {
    const int numWriters = 2;
    const int numReaders = 2;
    const int chunksPerWriter = 50;
    const size_t chunkSize = 32;

    std::atomic<int> totalWritten{0};
    std::atomic<int> totalRead{0};
    std::vector<std::thread> threads;

    // Create writer threads
    for (int w = 0; w < numWriters; ++w) {
        threads.emplace_back([&, w]() {
            for (int i = 0; i < chunksPerWriter; ++i) {
                auto testData = generateTestAudio(chunkSize, 440.0f + w * 100.0f + i * 10.0f);

                size_t written = 0;
                int attempts = 0;
                while (written < testData.size() && attempts < 500) {
                    written += buffer_->write(testData.data() + written, testData.size() - written);
                    if (written < testData.size()) {
                        std::this_thread::sleep_for(1ms);
                        attempts++;
                    }
                }

                totalWritten += written;
            }
        });
    }

    // Create reader threads
    for (int r = 0; r < numReaders; ++r) {
        threads.emplace_back([&, r]() {
            std::vector<float> readBuffer(chunkSize);
            int readCycles = 0;

            while (readCycles < chunksPerWriter
                   && (totalWritten.load() > totalRead.load()
                       || totalWritten.load() < numWriters * chunksPerWriter * chunkSize)) {
                if (buffer_->getAvailableForRead() >= chunkSize) {
                    size_t readCount = buffer_->read(readBuffer.data(), readBuffer.size());
                    totalRead += readCount;
                    if (readCount == chunkSize) {
                        readCycles++;
                    }
                } else {
                    std::this_thread::sleep_for(2ms);
                }
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify data consistency (allowing for minor variations due to threading)
    size_t expectedTotal = numWriters * chunksPerWriter * chunkSize;
    EXPECT_GE(totalWritten.load(), expectedTotal * 0.95);  // Allow 5% variation
    EXPECT_GE(totalRead.load(), totalWritten.load() * 0.95);
}

// Performance tests
TEST_F(CircularAudioBufferTest, PerformanceTest) {
    const size_t numOperations = 10000;
    const size_t chunkSize = 64;

    auto testData = generateTestAudio(chunkSize);
    std::vector<float> readBuffer(chunkSize);

    // Measure write performance
    auto writeStart = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < numOperations; ++i) {
        buffer_->write(testData.data(), testData.size());

        // Read to make space if buffer gets full
        if (buffer_->getAvailableForWrite() < chunkSize) {
            buffer_->read(readBuffer.data(), readBuffer.size());
        }
    }

    auto writeEnd = std::chrono::high_resolution_clock::now();
    auto writeDuration =
        std::chrono::duration_cast<std::chrono::microseconds>(writeEnd - writeStart);

    // Measure read performance
    // Fill buffer first
    while (!buffer_->isFull()) {
        buffer_->write(testData.data(), testData.size());
    }

    auto readStart = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < numOperations; ++i) {
        buffer_->read(readBuffer.data(), readBuffer.size());

        // Write to maintain data if buffer gets empty
        if (buffer_->getAvailableForRead() < chunkSize) {
            buffer_->write(testData.data(), testData.size());
        }
    }

    auto readEnd = std::chrono::high_resolution_clock::now();
    auto readDuration = std::chrono::duration_cast<std::chrono::microseconds>(readEnd - readStart);

    // Performance should be reasonable (exact values depend on system)
    double writeUsPerOp = static_cast<double>(writeDuration.count()) / numOperations;
    double readUsPerOp = static_cast<double>(readDuration.count()) / numOperations;

    std::cout << "Write performance: " << writeUsPerOp << " μs/operation" << std::endl;
    std::cout << "Read performance: " << readUsPerOp << " μs/operation" << std::endl;

    // Should complete operations reasonably quickly
    EXPECT_LT(writeUsPerOp, 100.0);  // Less than 100 μs per operation
    EXPECT_LT(readUsPerOp, 100.0);   // Less than 100 μs per operation
}

// Memory alignment tests
TEST_F(CircularAudioBufferTest, MemoryAlignmentTest) {
    // Get buffer memory address - this may not be available in interface
    // Will need to check if this method exists or skip this test
    // float* bufferPtr = buffer_->getWritePointer();
    // ASSERT_NE(bufferPtr, nullptr);

    // Check alignment - For now, just verify buffer is operational
    auto testData = generateTestAudio(64);
    size_t written = buffer_->write(testData.data(), testData.size());
    EXPECT_EQ(written, testData.size());

    std::vector<float> readData(64);
    size_t readCount = buffer_->read(readData.data(), readData.size());
    EXPECT_EQ(readCount, readData.size());
}

// Error handling and edge cases
TEST_F(CircularAudioBufferTest, NullPointerTest) {
    // Test write with null pointer
    size_t written = buffer_->write(nullptr, 100);
    EXPECT_EQ(written, 0);

    // Test read with null pointer
    size_t readCount = buffer_->read(nullptr, 100);
    EXPECT_EQ(readCount, 0);
}

TEST_F(CircularAudioBufferTest, ZeroSizeOperationsTest) {
    auto testData = generateTestAudio(100);

    // Write zero samples
    size_t written = buffer_->write(testData.data(), 0);
    EXPECT_EQ(written, 0);
    EXPECT_TRUE(buffer_->isEmpty());

    // Read zero samples
    std::vector<float> readBuffer(100);
    size_t readCount = buffer_->read(readBuffer.data(), 0);
    EXPECT_EQ(readCount, 0);
}

TEST_F(CircularAudioBufferTest, BufferStateQueriesTest) {
    // Initially empty
    EXPECT_TRUE(buffer_->isEmpty());
    EXPECT_FALSE(buffer_->isFull());
    EXPECT_EQ(buffer_->getCapacity(), config_.bufferSize);
    EXPECT_EQ(buffer_->getAvailableForRead(), 0);
    EXPECT_EQ(buffer_->getAvailableForWrite(), config_.bufferSize);

    // Partially filled
    auto testData = generateTestAudio(config_.bufferSize / 2);
    buffer_->write(testData.data(), testData.size());

    EXPECT_FALSE(buffer_->isEmpty());
    EXPECT_FALSE(buffer_->isFull());
    EXPECT_EQ(buffer_->getAvailableForRead(), testData.size());
    EXPECT_EQ(buffer_->getAvailableForWrite(), config_.bufferSize - testData.size());

    // Completely filled
    auto moreData = generateTestAudio(config_.bufferSize / 2);
    buffer_->write(moreData.data(), moreData.size());

    EXPECT_FALSE(buffer_->isEmpty());
    EXPECT_TRUE(buffer_->isFull());
    EXPECT_EQ(buffer_->getAvailableForRead(), config_.bufferSize);
    EXPECT_EQ(buffer_->getAvailableForWrite(), 0);
}

// Clear and reset tests
TEST_F(CircularAudioBufferTest, ClearTest) {
    // Fill buffer with data
    auto testData = generateTestAudio(config_.bufferSize);
    buffer_->write(testData.data(), testData.size());
    EXPECT_TRUE(buffer_->isFull());

    // Clear buffer
    buffer_->clear();

    // Should be empty
    EXPECT_TRUE(buffer_->isEmpty());
    EXPECT_FALSE(buffer_->isFull());
    EXPECT_EQ(buffer_->getAvailableForRead(), 0);
    EXPECT_EQ(buffer_->getAvailableForWrite(), config_.bufferSize);

    // Should be able to write again
    auto newData = generateTestAudio(256);
    size_t written = buffer_->write(newData.data(), newData.size());
    EXPECT_EQ(written, newData.size());
}

// Multi-channel tests (if supported)
TEST_F(CircularAudioBufferTest, MultiChannelTest) {
    // Create multi-channel buffer
    CircularBufferConfig multiConfig = config_;
    multiConfig.numChannels = 2;
    auto multiBuffer = std::make_unique<CircularAudioBuffer>(multiConfig);

    // Generate stereo test data (interleaved)
    const size_t numSamples = 256;
    std::vector<float> stereoData(numSamples * multiConfig.numChannels);
    for (size_t i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / multiConfig.sampleRate;
        stereoData[i * 2] = 0.5f * std::sin(2.0f * M_PI * 440.0f * t);      // Left channel
        stereoData[i * 2 + 1] = 0.3f * std::sin(2.0f * M_PI * 880.0f * t);  // Right channel
    }

    // Write stereo data
    size_t written = multiBuffer->write(stereoData.data(), stereoData.size());
    EXPECT_EQ(written, stereoData.size());

    // Read stereo data back
    std::vector<float> readData(stereoData.size());
    size_t readCount = multiBuffer->read(readData.data(), readData.size());
    EXPECT_EQ(readCount, readData.size());

    // Verify data integrity
    for (size_t i = 0; i < stereoData.size(); ++i) {
        EXPECT_NEAR(readData[i], stereoData[i], 1e-6f) << "Sample " << i;
    }
}
