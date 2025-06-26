// Generate test vectors on one platform
void generateTestVectors()
{
    // Process standard test files
    std::vector<TestCase> cases = {
        {"sine_440hz.wav", "sine_440_mfcc.bin"},
        {"buck_grunt.wav", "buck_grunt_mfcc.bin"},
        {"white_noise.wav", "white_noise_mfcc.bin"}};

    for (auto &test : cases)
    {
        auto mfcc = engine->processFile(test.input);
        saveBinary(test.output, mfcc);
    }
}

// Verify on all platforms
void verifyTestVectors()
{
    // Load and compare
    for (auto &test : cases)
    {
        auto expected = loadBinary(test.output);
        auto actual = engine->processFile(test.input);

        // Should match within floating point tolerance
        EXPECT_NEAR_MATRIX(expected, actual, 1e-5);
    }
}