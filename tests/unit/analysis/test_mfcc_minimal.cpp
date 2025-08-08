#include <gtest/gtest.h>

#include "huntmaster/core/MFCCProcessor.h"

using namespace huntmaster;

class MFCCMinimalTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Standard config for most tests
        standard_config.sample_rate = 44100;
        standard_config.frame_size = 512;
        standard_config.num_coefficients = 13;
        standard_config.num_filters = 26;
        standard_config.low_freq = 0.0f;
        standard_config.high_freq = 0.0f;  // Auto-set to Nyquist
        standard_config.use_energy = true;
        standard_config.apply_lifter = true;
        standard_config.enable_simd = true;
        standard_config.enable_caching = true;
        standard_config.lifter_coeff = 22;
    }

    MFCCProcessor::Config standard_config;
};

// Test 1: Constructor validation paths - targeting constructor coverage
TEST_F(MFCCMinimalTest, BasicConstructor) {
    // Test valid config (should work)
    EXPECT_NO_THROW({ MFCCProcessor processor(standard_config); });
}
