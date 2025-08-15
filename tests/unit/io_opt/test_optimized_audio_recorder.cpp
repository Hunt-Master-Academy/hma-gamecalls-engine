// Deterministic tests for OptimizedAudioRecorder (memory mode) without device I/O
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <vector>

#include <gtest/gtest.h>

#include "dr_wav.h"
#include "huntmaster/core/OptimizedAudioRecorder.h"

namespace {
constexpr double kPI = 3.14159265358979323846264338327950288;

static std::vector<float> makeSine(size_t frames, float amplitude = 0.5f) {
    std::vector<float> v(frames);
    for (size_t i = 0; i < frames; ++i) {
        double s = std::sin(2.0 * kPI * (static_cast<double>(i % 32) / 32.0));
        v[i] = amplitude * static_cast<float>(s);
    }
    return v;
}
}  // namespace

TEST(OptimizedAudioRecorder, MemoryMode_BasicBuffering) {
    huntmaster::OptimizedAudioRecorder::Config cfg{};
    cfg.recordingMode = huntmaster::OptimizedAudioRecorder::RecordingMode::MEMORY_BASED;
    cfg.channels = 1;
    cfg.sampleRate = 16000;
    cfg.memoryBufferMaxFrames = 1024;
    cfg.enableMemoryGrowth = false;  // fixed capacity for this test

    huntmaster::OptimizedAudioRecorder rec{cfg};

    // Without starting the device, feed samples directly
    auto s1 = makeSine(256);
    rec.testFeedMemorySamples(s1.data(), s1.size());
    EXPECT_EQ(rec.getSampleCount(), s1.size());

    // Append more and validate
    auto s2 = makeSine(128);
    rec.testFeedMemorySamples(s2.data(), s2.size());
    EXPECT_EQ(rec.getSampleCount(), s1.size() + s2.size());

    // Access vector copy API
    auto data = rec.getRecordedData();
    ASSERT_EQ(data.size(), s1.size() + s2.size());
    EXPECT_FLOAT_EQ(data[0], s1[0]);
}

TEST(OptimizedAudioRecorder, MemoryMode_CopyAndInfo) {
    huntmaster::OptimizedAudioRecorder::Config cfg{};
    cfg.recordingMode = huntmaster::OptimizedAudioRecorder::RecordingMode::MEMORY_BASED;
    cfg.channels = 1;
    cfg.sampleRate = 8000;
    cfg.memoryBufferMaxFrames = 512;
    cfg.enableMemoryGrowth = true;
    cfg.memoryGrowthIncrement = 256;

    huntmaster::OptimizedAudioRecorder rec{cfg};
    auto s = makeSine(600);
    rec.testFeedMemorySamples(s.data(), s.size());

    size_t sz = 0;
    const float* ptr = rec.getRecordedDataPtr(sz);
    ASSERT_TRUE(ptr != nullptr);
    ASSERT_EQ(sz, s.size());

    std::vector<float> out(600, 0.0f);
    size_t copied = rec.copyRecordedData(out.data(), out.size());
    EXPECT_EQ(copied, s.size());
    EXPECT_FLOAT_EQ(out[10], s[10]);

    auto info = rec.getMemoryBufferInfo();
    EXPECT_GE(info.totalCapacityFrames, s.size());
    EXPECT_EQ(info.usedFrames, s.size());
    EXPECT_FALSE(info.hasOverflowed);

    rec.clearMemoryBuffer();
    EXPECT_EQ(rec.getSampleCount(), 0u);
}

TEST(OptimizedAudioRecorder, FileMode_BufferedWritesAndFlush) {
    huntmaster::OptimizedAudioRecorder::Config cfg{};
    cfg.recordingMode = huntmaster::OptimizedAudioRecorder::RecordingMode::FILE_BASED;
    cfg.channels = 2;
    cfg.sampleRate = 22050;
    cfg.chunkSize = 64;         // small chunk for test
    cfg.bufferDurationMs = 10;  // small internal buffer

    huntmaster::OptimizedAudioRecorder rec{cfg};

    // Prepare 150 frames (will create 2 full chunks (128 frames) + 22 frames leftover)
    std::vector<float> interleaved(150 * cfg.channels, 0.0f);
    for (size_t f = 0; f < 150; ++f) {
        interleaved[2 * f + 0] = static_cast<float>(f) / 150.0f;
        interleaved[2 * f + 1] = 1.0f - static_cast<float>(f) / 150.0f;
    }

    rec.testFeedFileSamples(interleaved.data(), 150);

    // After feeding, two chunks should have been recorded (128 frames * 2ch)
    size_t sz = 0;
    const float* ptr = rec.getRecordedDataPtr(sz);
    ASSERT_TRUE(ptr != nullptr);
    EXPECT_EQ(sz, 128 * cfg.channels);  // only full chunks copied so far

    // Force flush the remainder
    rec.testForceFlushFileBuffer();
    ptr = rec.getRecordedDataPtr(sz);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(sz, 150 * cfg.channels);
    EXPECT_FLOAT_EQ(ptr[0], interleaved[0]);
    EXPECT_FLOAT_EQ(ptr[1], interleaved[1]);
}

TEST(OptimizedAudioRecorder, HybridMode_MemoryAndFilePaths) {
    huntmaster::OptimizedAudioRecorder::Config cfg{};
    cfg.recordingMode = huntmaster::OptimizedAudioRecorder::RecordingMode::HYBRID;
    cfg.channels = 1;
    cfg.sampleRate = 16000;
    cfg.chunkSize = 50;
    cfg.memoryBufferMaxFrames = 64;
    cfg.enableMemoryGrowth = true;

    huntmaster::OptimizedAudioRecorder rec{cfg};

    auto s = makeSine(120);
    // Hybrid: feed both memory and file paths
    rec.testFeedMemorySamples(s.data(), s.size());
    rec.testFeedFileSamples(s.data(), 120);

    size_t sz = 0;
    (void)rec.getRecordedDataPtr(sz);  // file path size
    EXPECT_GE(sz, 100u);               // at least some chunking occurred

    EXPECT_EQ(rec.getSampleCount(), s.size());  // memory side count
}

TEST(OptimizedAudioRecorder, SaveMemoryToFile_WritesValidWav) {
    huntmaster::OptimizedAudioRecorder::Config cfg{};
    cfg.recordingMode = huntmaster::OptimizedAudioRecorder::RecordingMode::MEMORY_BASED;
    cfg.channels = 1;
    cfg.sampleRate = 8000;
    cfg.memoryBufferMaxFrames = 256;

    huntmaster::OptimizedAudioRecorder rec{cfg};
    auto s = makeSine(200, 0.25f);
    rec.testFeedMemorySamples(s.data(), s.size());

    // Write to a temp file inside build tree
    std::string dir = "build/debug/test_artifacts";
    std::filesystem::create_directories(dir);
    std::string out = dir + "/rec_mem.wav";
    // Ensure parent folder exists is handled by environment; attempt save
    ASSERT_TRUE(rec.saveMemoryToFile(out));

    // Open via dr_wav and validate
    drwav wav{};
    ASSERT_TRUE(drwav_init_file(&wav, out.c_str(), nullptr));
    EXPECT_EQ(wav.channels, cfg.channels);
    EXPECT_EQ(wav.sampleRate, cfg.sampleRate);
    EXPECT_EQ(wav.bitsPerSample, 32);
    EXPECT_EQ(wav.totalPCMFrameCount, s.size());

    std::vector<float> readback(s.size());
    drwav_uint64 frames = drwav_read_pcm_frames(&wav, readback.size(), readback.data());
    EXPECT_EQ(frames, s.size());
    // Spot check a couple samples
    EXPECT_NEAR(readback[10], s[10], 1e-6f);
    EXPECT_NEAR(readback[100], s[100], 1e-6f);
    drwav_uninit(&wav);
}
