#include "../include/huntmaster_engine/HuntmasterAudioEngine.h"
#include "../include/huntmaster_engine/MFCCProcessor.h"
#include <iostream>
#include <vector>
#include <fstream> // Required for file I/O (ifstream, ofstream)

// Define the implementations for dr_wav and dr_mp3 before including the headers.
// This is how these header-only libraries work.
#define DR_WAV_IMPLEMENTATION
#include "../libs/dr_wav.h"
#define DR_MP3_IMPLEMENTATION
#include "../libs/dr_mp3.h"

// Constructor (private for singleton)
HuntmasterAudioEngine::HuntmasterAudioEngine()
{
    // The std::unique_ptr for mfccProcessor is initialized to nullptr by default.
}

// Destructor
HuntmasterAudioEngine::~HuntmasterAudioEngine()
{
    // The std::unique_ptr will automatically delete the MFCCProcessor instance.
}

// Singleton instance getter
HuntmasterAudioEngine &HuntmasterAudioEngine::getInstance()
{
    static HuntmasterAudioEngine instance;
    return instance;
}

// Initialize the engine
void HuntmasterAudioEngine::initialize()
{
    std::cout << "HuntmasterAudioEngine initialized successfully." << std::endl;
    // MFCC Processor is now created on-demand in loadMasterCall to ensure
    // it has the correct configuration for the audio being processed.
}

// Load master call features, using cache if available.
void HuntmasterAudioEngine::loadMasterCall(const std::string &masterCallId)
{
    std::cout << "Loading master call with ID: " << masterCallId << std::endl;

    // First, try to load from a pre-computed features file.
    if (loadFeaturesFromFile(masterCallId))
    {
        std::cout << "  - Successfully loaded pre-computed features from file." << std::endl;
        return; // Success, no need to process audio.
    }

    // If loading from file fails, proceed to process the source audio file.
    std::cout << "  - No pre-computed features found. Processing from source audio..." << std::endl;
    std::string audioFilePath = "../data/master_calls/" + masterCallId + ".wav";

    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;
    float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(audioFilePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (pSampleData == nullptr)
    {
        // Try with .mp3 as a fallback
        audioFilePath = "../data/master_calls/" + masterCallId + ".mp3";
        drmp3_config mp3Config;
        pSampleData = drmp3_open_file_and_read_pcm_frames_f32(audioFilePath.c_str(), &mp3Config, &totalPCMFrameCount, nullptr);
        if (pSampleData == nullptr)
        {
            std::cerr << "Error: Could not load audio file for master call ID: " << masterCallId << std::endl;
            return;
        }
        channels = mp3Config.channels;
        sampleRate = mp3Config.sampleRate;
    }

    std::cout << "  - File loaded: " << audioFilePath << std::endl;
    std::cout << "  - Channels: " << channels << ", Sample Rate: " << sampleRate << std::endl;

    // --- FIX: Dynamically configure the MFCC processor based on the audio file's sample rate ---
    // If the processor doesn't exist or if the sample rate is different, create a new one.
    if (!mfccProcessor || mfccProcessor->getConfig().sampleRate != sampleRate)
    {
        std::cout << "  - Configuring MFCC processor for " << sampleRate << " Hz sample rate." << std::endl;
        huntmaster::MFCCProcessor::Config newConfig;
        newConfig.sampleRate = static_cast<float>(sampleRate);
        mfccProcessor = std::make_unique<huntmaster::MFCCProcessor>(newConfig);
    }

    // Convert to mono for MFCC processing
    std::vector<float> monoSamples;
    if (channels > 1)
    {
        monoSamples.resize(totalPCMFrameCount);
        for (drwav_uint64 i = 0; i < totalPCMFrameCount; ++i)
        {
            float monoSample = 0.0f;
            for (unsigned int j = 0; j < channels; ++j)
            {
                monoSample += pSampleData[i * channels + j];
            }
            monoSamples[i] = monoSample / channels;
        }
    }
    else
    {
        monoSamples.assign(pSampleData, pSampleData + totalPCMFrameCount);
    }
    drwav_free(pSampleData, nullptr); // Free audio data memory now that we have it in monoSamples

    // Process the entire mono audio buffer to extract MFCC features.
    auto mfccFrames = mfccProcessor->processBuffer(monoSamples.data(), monoSamples.size());

    // Store the computed coefficients.
    masterCallFeatures.clear();
    for (const auto &frame : mfccFrames)
    {
        masterCallFeatures.push_back(frame.coefficients);
    }

    std::cout << "  - Successfully processed " << masterCallFeatures.size() << " MFCC frames." << std::endl;

    // Save the newly computed features to a file for next time.
    saveFeaturesToFile(masterCallId);
}

// Saves computed features to a binary file.
void HuntmasterAudioEngine::saveFeaturesToFile(const std::string &masterCallId)
{
    // Corrected file path to be relative to the build directory
    std::string featureFilePath = "../data/features/" + masterCallId + ".mfc";
    std::ofstream outFile(featureFilePath, std::ios::binary);

    if (!outFile.is_open())
    {
        std::cerr << "Error: Could not open file for writing features: " << featureFilePath << std::endl;
        return;
    }

    // Write a simple header: number of frames and number of coefficients.
    uint32_t numFrames = static_cast<uint32_t>(masterCallFeatures.size());
    uint32_t numCoeffs = (numFrames > 0) ? static_cast<uint32_t>(masterCallFeatures[0].size()) : 0;

    outFile.write(reinterpret_cast<const char *>(&numFrames), sizeof(numFrames));
    outFile.write(reinterpret_cast<const char *>(&numCoeffs), sizeof(numCoeffs));

    // Write the actual feature data.
    for (const auto &frame : masterCallFeatures)
    {
        outFile.write(reinterpret_cast<const char *>(frame.data()), numCoeffs * sizeof(float));
    }

    outFile.close();
    std::cout << "  - MFCC features saved to " << featureFilePath << std::endl;
}

// Loads pre-computed features from a binary file.
bool HuntmasterAudioEngine::loadFeaturesFromFile(const std::string &masterCallId)
{
    // Corrected file path to be relative to the build directory
    std::string featureFilePath = "../data/features/" + masterCallId + ".mfc";
    std::ifstream inFile(featureFilePath, std::ios::binary);

    if (!inFile.is_open())
    {
        return false; // File doesn't exist or cannot be opened.
    }

    // Read the header.
    uint32_t numFrames = 0;
    uint32_t numCoeffs = 0;
    inFile.read(reinterpret_cast<char *>(&numFrames), sizeof(numFrames));
    inFile.read(reinterpret_cast<char *>(&numCoeffs), sizeof(numCoeffs));

    if (numFrames == 0 || numCoeffs == 0)
    {
        std::cerr << "Error: Invalid feature file header in " << featureFilePath << std::endl;
        return false;
    }

    // Resize the features vector and read the data.
    masterCallFeatures.assign(numFrames, std::vector<float>(numCoeffs));
    for (uint32_t i = 0; i < numFrames; ++i)
    {
        inFile.read(reinterpret_cast<char *>(masterCallFeatures[i].data()), numCoeffs * sizeof(float));
    }

    inFile.close();
    return true;
}

// Start a realtime session
int HuntmasterAudioEngine::startRealtimeSession(float sampleRate, int bufferSize)
{
    std::cout << "Starting realtime session - Sample Rate: " << sampleRate
              << ", Buffer Size: " << bufferSize << std::endl;
    // TODO: Implement session management
    static int sessionCounter = 0;
    return ++sessionCounter; // Return a simple session ID
}

// Process audio chunk
void HuntmasterAudioEngine::processAudioChunk(int sessionId, const float *audioBuffer, int bufferSize)
{
    std::cout << "Processing audio chunk for session " << sessionId
              << " with buffer size " << bufferSize << std::endl;
    // TODO: Implement audio processing and MFCC extraction for live data
}

// Get similarity score
float HuntmasterAudioEngine::getSimilarityScore(int sessionId)
{
    std::cout << "Getting similarity score for session " << sessionId << std::endl;
    // TODO: Implement DTW similarity calculation
    return 0.85f; // Return dummy score for now
}

// End realtime session
void HuntmasterAudioEngine::endRealtimeSession(int sessionId)
{
    std::cout << "Ending realtime session " << sessionId << std::endl;
    // TODO: Implement session cleanup
}

// Shutdown the engine
void HuntmasterAudioEngine::shutdown()
{
    std::cout << "Shutting down HuntmasterAudioEngine" << std::endl;
    // mfccProcessor is automatically cleaned up by its unique_ptr.
}
