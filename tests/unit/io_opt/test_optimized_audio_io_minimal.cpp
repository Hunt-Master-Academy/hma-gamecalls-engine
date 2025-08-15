// Minimal OptimizedAudioIO tests (engine-free, deterministic)
#include <gtest/gtest.h>

#include "huntmaster/core/OptimizedAudioIO.h"

using namespace huntmaster;

TEST(OptimizedAudioIO_Min, MemoryMappedDefaultsAndOpenFail) {
    MemoryMappedAudioFile::Config cfg;  // defaults
    MemoryMappedAudioFile mm(cfg);

    // Before open, should not be open
    EXPECT_FALSE(mm.isOpen());
    EXPECT_EQ(mm.getSampleCount(), 0u);

    // Open a clearly non-existent file should fail and leave closed state
    EXPECT_FALSE(mm.open("/this/path/does/not/exist/never_ever.wav"));
    EXPECT_FALSE(mm.isOpen());

    // Metrics should be well-defined (zeros)
    auto m = mm.getMetrics();
    EXPECT_EQ(m.bytesRead, 0u);
    EXPECT_EQ(m.readOperations, 0u);
    EXPECT_DOUBLE_EQ(m.getReadThroughputMBps(), 0.0);
}

TEST(OptimizedAudioIO_Min, AsyncWriterConstructedQueueEmpty) {
    AsyncAudioWriter::Config wcfg;  // defaults
    AsyncAudioWriter writer(wcfg);
    // Without start(), queue depth should be 0 and no crash
    EXPECT_EQ(writer.getQueueDepth(), 0u);
}
