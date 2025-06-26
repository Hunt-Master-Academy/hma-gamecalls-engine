#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include "huntmaster_engine/HuntmasterAudioEngine.h"
#include "dr_wav.h"

// Generate a test sine wave
std::vector<float> generateSineWave(float frequency, float duration, float sampleRate)
{
    int numSamples = static_cast<int>(duration * sampleRate);
    std::vector<float> samples(numSamples);

    const float twoPi = 2.0f * 3.14159265359f;
    for (int i = 0; i < numSamples; ++i)
    {
        samples[i] = 0.5f * sin(twoPi * frequency * i / sampleRate);
    }

    return samples;
}

// Save audio to WAV file
bool saveTestWav(const std::string &filename, const std::vector<float> &samples, float sampleRate)
{
    drwav wav;
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 1;
    format.sampleRate = static_cast<drwav_uint32>(sampleRate);
    format.bitsPerSample = 32;

    if (!drwav_init_file_write(&wav, filename.c_str(), &format, nullptr))
    {
        std::cerr << "Failed to create file: " << filename << std::endl;
        return false;
    }

    drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, samples.size(), samples.data());
    drwav_uninit(&wav);

    return framesWritten == samples.size();
}

int main()
{
    std::cout << "=== MFCC Consistency Test ===" << std::endl;
    std::cout << "This test verifies that MFCC processing produces identical results" << std::endl;
    std::cout << "when processing the same audio multiple times.\n"
              << std::endl;

    // Initialize engine
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    // Create test directory if it doesn't exist
    system("mkdir ..\\data\\test_audio 2>nul");

    // Test 1: Pure Sine Wave (440 Hz)
    std::cout << "Test 1: Processing 440 Hz sine wave" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    auto sineWave440 = generateSineWave(440.0f, 1.0f, 44100.0f);
    std::string testFile1 = "../data/test_audio/test_sine_440.wav";

    if (!saveTestWav(testFile1, sineWave440, 44100.0f))
    {
        std::cerr << "Failed to create test file!" << std::endl;
        return 1;
    }
    std::cout << "Created test file: " << testFile1 << std::endl;

    // Process the same file 5 times and collect scores
    std::vector<float> scores;
    std::cout << "\nProcessing 5 times:" << std::endl;

    for (int i = 0; i < 5; ++i)
    {
        // Load as master (this triggers MFCC processing)
        engine.loadMasterCall("test_sine_440");

        // Create a session and process the same audio
        int sessionId = engine.startRealtimeSession(44100.0f, 1024);

        // Process in chunks
        const int chunkSize = 1024;
        for (size_t j = 0; j < sineWave440.size(); j += chunkSize)
        {
            size_t remaining = sineWave440.size() - j;
            size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);
            engine.processAudioChunk(sessionId, sineWave440.data() + j, toProcess);
        }

        float score = engine.getSimilarityScore(sessionId);
        scores.push_back(score);
        engine.endRealtimeSession(sessionId);

        std::cout << "  Run " << (i + 1) << ": Score = " << std::fixed
                  << std::setprecision(8) << score << std::endl;
    }

    // Analyze consistency
    float minScore = scores[0];
    float maxScore = scores[0];
    float avgScore = 0.0f;

    for (float score : scores)
    {
        minScore = std::min(minScore, score);
        maxScore = std::max(maxScore, score);
        avgScore += score;
    }
    avgScore /= scores.size();

    float maxDeviation = maxScore - minScore;
    float maxDeviationPercent = (maxDeviation / avgScore) * 100.0f;

    std::cout << "\nResults:" << std::endl;
    std::cout << "  Average score: " << avgScore << std::endl;
    std::cout << "  Min score: " << minScore << std::endl;
    std::cout << "  Max score: " << maxScore << std::endl;
    std::cout << "  Max deviation: " << maxDeviation << " ("
              << maxDeviationPercent << "%)" << std::endl;

    bool test1Pass = maxDeviation < 0.0001f;
    std::cout << "  Status: " << (test1Pass ? "PASS ✓" : "FAIL ✗") << std::endl;

    // Test 2: Complex waveform (multiple frequencies)
    std::cout << "\n\nTest 2: Processing complex waveform" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    // Create a more complex waveform
    std::vector<float> complexWave(44100); // 1 second
    for (int i = 0; i < 44100; ++i)
    {
        float t = i / 44100.0f;
        complexWave[i] = 0.3f * sin(2.0f * 3.14159f * 220.0f * t) + // 220 Hz
                         0.2f * sin(2.0f * 3.14159f * 440.0f * t) + // 440 Hz
                         0.1f * sin(2.0f * 3.14159f * 880.0f * t);  // 880 Hz
    }

    std::string testFile2 = "../data/test_audio/test_complex.wav";
    if (!saveTestWav(testFile2, complexWave, 44100.0f))
    {
        std::cerr << "Failed to create complex test file!" << std::endl;
        return 1;
    }

    // Process multiple times
    std::vector<float> complexScores;
    for (int i = 0; i < 5; ++i)
    {
        engine.loadMasterCall("test_complex");

        int sessionId = engine.startRealtimeSession(44100.0f, 1024);

        const int chunkSize = 1024;
        for (size_t j = 0; j < complexWave.size(); j += chunkSize)
        {
            size_t remaining = complexWave.size() - j;
            size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);
            engine.processAudioChunk(sessionId, complexWave.data() + j, toProcess);
        }

        float score = engine.getSimilarityScore(sessionId);
        complexScores.push_back(score);
        engine.endRealtimeSession(sessionId);

        std::cout << "  Run " << (i + 1) << ": Score = " << std::fixed
                  << std::setprecision(8) << score << std::endl;
    }

    // Analyze complex wave consistency
    float complexMaxDev = 0.0f;
    for (size_t i = 1; i < complexScores.size(); ++i)
    {
        complexMaxDev = std::max(complexMaxDev,
                                 std::abs(complexScores[i] - complexScores[0]));
    }

    bool test2Pass = complexMaxDev < 0.0001f;
    std::cout << "\nResults:" << std::endl;
    std::cout << "  Max deviation: " << complexMaxDev << std::endl;
    std::cout << "  Status: " << (test2Pass ? "PASS ✓" : "FAIL ✗") << std::endl;

    // Test 3: Real audio file (if available)
    std::cout << "\n\nTest 3: Processing real audio file" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    // Try to test with buck_grunt if it exists
    std::vector<float> realScores;
    bool realFileExists = false;

    for (int i = 0; i < 3; ++i)
    {
        try
        {
            engine.loadMasterCall("buck_grunt");
            realFileExists = true;

            // Load the actual audio file
            unsigned int channels, sampleRate;
            drwav_uint64 totalFrames;
            float *audioData = drwav_open_file_and_read_pcm_frames_f32(
                "../data/master_calls/buck_grunt.wav", &channels, &sampleRate, &totalFrames, nullptr);

            if (audioData)
            {
                // Convert to mono if needed
                std::vector<float> monoData(totalFrames);
                if (channels > 1)
                {
                    for (drwav_uint64 j = 0; j < totalFrames; ++j)
                    {
                        float sum = 0.0f;
                        for (unsigned int ch = 0; ch < channels; ++ch)
                        {
                            sum += audioData[j * channels + ch];
                        }
                        monoData[j] = sum / channels;
                    }
                }
                else
                {
                    monoData.assign(audioData, audioData + totalFrames);
                }

                int sessionId = engine.startRealtimeSession(static_cast<float>(sampleRate), 1024);

                const int chunkSize = 1024;
                for (size_t j = 0; j < monoData.size(); j += chunkSize)
                {
                    size_t remaining = monoData.size() - j;
                    size_t toProcess = std::min(static_cast<size_t>(chunkSize), remaining);
                    engine.processAudioChunk(sessionId, monoData.data() + j, toProcess);
                }

                float score = engine.getSimilarityScore(sessionId);
                realScores.push_back(score);
                engine.endRealtimeSession(sessionId);

                std::cout << "  Run " << (i + 1) << ": Score = " << std::fixed
                          << std::setprecision(8) << score << std::endl;

                drwav_free(audioData, nullptr);
            }
        }
        catch (...)
        {
            std::cout << "  Could not load buck_grunt.wav - SKIPPED" << std::endl;
            break;
        }
    }

    if (realFileExists && realScores.size() > 1)
    {
        float realMaxDev = 0.0f;
        for (size_t i = 1; i < realScores.size(); ++i)
        {
            realMaxDev = std::max(realMaxDev,
                                  std::abs(realScores[i] - realScores[0]));
        }

        bool test3Pass = realMaxDev < 0.0001f;
        std::cout << "\nResults:" << std::endl;
        std::cout << "  Max deviation: " << realMaxDev << std::endl;
        std::cout << "  Status: " << (test3Pass ? "PASS ✓" : "FAIL ✗") << std::endl;
    }

    // Overall summary
    std::cout << "\n\n=== OVERALL SUMMARY ===" << std::endl;
    std::cout << "Test 1 (Sine wave): " << (test1Pass ? "PASS ✓" : "FAIL ✗") << std::endl;
    std::cout << "Test 2 (Complex wave): " << (test2Pass ? "PASS ✓" : "FAIL ✗") << std::endl;

    if (test1Pass && test2Pass)
    {
        std::cout << "\n✓ MFCC processing is CONSISTENT!" << std::endl;
        std::cout << "  You can proceed with confidence that the algorithm is deterministic." << std::endl;
    }
    else
    {
        std::cout << "\n✗ MFCC processing is INCONSISTENT!" << std::endl;
        std::cout << "  This needs to be fixed before proceeding." << std::endl;
        std::cout << "\nPossible causes:" << std::endl;
        std::cout << "  - Uninitialized variables in MFCC processing" << std::endl;
        std::cout << "  - Race conditions in multi-threaded code" << std::endl;
        std::cout << "  - Floating point precision issues" << std::endl;
    }

    engine.shutdown();
    return (test1Pass && test2Pass) ? 0 : 1;
}