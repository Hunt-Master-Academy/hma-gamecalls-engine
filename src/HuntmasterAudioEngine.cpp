#include "../include/huntmaster_engine/HuntmasterAudioEngine.h"
#include "../include/huntmaster_engine/MFCCProcessor.h"
#include "../include/huntmaster_engine/DTWProcessor.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>

#define DR_WAV_IMPLEMENTATION
#include "../libs/dr_wav.h"
#define DR_MP3_IMPLEMENTATION
#include "../libs/dr_mp3.h"

HuntmasterAudioEngine::HuntmasterAudioEngine() = default;
HuntmasterAudioEngine::~HuntmasterAudioEngine() = default;

HuntmasterAudioEngine &HuntmasterAudioEngine::getInstance()
{
    static HuntmasterAudioEngine instance;
    return instance;
}

void HuntmasterAudioEngine::initialize()
{
    std::cout << "HuntmasterAudioEngine initialized successfully." << std::endl;
}

void HuntmasterAudioEngine::loadMasterCall(const std::string &masterCallId)
{
    std::cout << "Loading master call with ID: " << masterCallId << std::endl;

    if (loadFeaturesFromFile(masterCallId))
    {
        std::cout << "  - Successfully loaded pre-computed features from file." << std::endl;
        return;
    }

    std::cout << "  - No pre-computed features found. Processing from source audio..." << std::endl;
    std::string audioFilePath = "../data/master_calls/" + masterCallId + ".wav";

    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;
    float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(audioFilePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (pSampleData == nullptr)
    {
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

    if (!mfccProcessor || mfccProcessor->getConfig().sampleRate != sampleRate)
    {
        std::cout << "  - Configuring MFCC processor for " << sampleRate << " Hz sample rate." << std::endl;
        huntmaster::MFCCProcessor::Config newConfig;
        newConfig.sampleRate = static_cast<float>(sampleRate);
        mfccProcessor = std::make_unique<huntmaster::MFCCProcessor>(newConfig);
    }

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
    drwav_free(pSampleData, nullptr);

    auto mfccFrames = mfccProcessor->processBuffer(monoSamples.data(), monoSamples.size());

    masterCallFeatures.clear();
    for (const auto &frame : mfccFrames)
    {
        masterCallFeatures.push_back(frame.coefficients);
    }

    std::cout << "  - Successfully processed " << masterCallFeatures.size() << " MFCC frames." << std::endl;
    saveFeaturesToFile(masterCallId);
}

void HuntmasterAudioEngine::saveFeaturesToFile(const std::string &masterCallId)
{
    std::string featureFilePath = "../data/features/" + masterCallId + ".mfc";
    std::ofstream outFile(featureFilePath, std::ios::binary);

    if (!outFile.is_open())
    {
        std::cerr << "Error: Could not open file for writing features: " << featureFilePath << std::endl;
        return;
    }

    uint32_t numFrames = static_cast<uint32_t>(masterCallFeatures.size());
    uint32_t numCoeffs = (numFrames > 0) ? static_cast<uint32_t>(masterCallFeatures[0].size()) : 0;

    outFile.write(reinterpret_cast<const char *>(&numFrames), sizeof(numFrames));
    outFile.write(reinterpret_cast<const char *>(&numCoeffs), sizeof(numCoeffs));

    for (const auto &frame : masterCallFeatures)
    {
        outFile.write(reinterpret_cast<const char *>(frame.data()), numCoeffs * sizeof(float));
    }

    outFile.close();
    std::cout << "  - MFCC features saved to " << featureFilePath << std::endl;
}

bool HuntmasterAudioEngine::loadFeaturesFromFile(const std::string &masterCallId)
{
    std::string featureFilePath = "../data/features/" + masterCallId + ".mfc";
    std::ifstream inFile(featureFilePath, std::ios::binary);

    if (!inFile.is_open())
    {
        return false;
    }

    uint32_t numFrames = 0;
    uint32_t numCoeffs = 0;
    inFile.read(reinterpret_cast<char *>(&numFrames), sizeof(numFrames));
    inFile.read(reinterpret_cast<char *>(&numCoeffs), sizeof(numCoeffs));

    if (numFrames == 0 || numCoeffs == 0)
    {
        std::cerr << "Error: Invalid feature file header in " << featureFilePath << std::endl;
        return false;
    }

    masterCallFeatures.assign(numFrames, std::vector<float>(numCoeffs));
    for (uint32_t i = 0; i < numFrames; ++i)
    {
        inFile.read(reinterpret_cast<char *>(masterCallFeatures[i].data()), numCoeffs * sizeof(float));
    }

    inFile.close();
    return true;
}

int HuntmasterAudioEngine::startRealtimeSession(float sampleRate, int bufferSize)
{
    std::cout << "Starting realtime session - Sample Rate: " << sampleRate
              << ", Buffer Size: " << bufferSize << std::endl;

    sessionAudioBuffer.clear();
    sessionFeatures.clear();

    if (!mfccProcessor || mfccProcessor->getConfig().sampleRate != sampleRate)
    {
        std::cout << "  - Configuring MFCC processor for live session sample rate: " << sampleRate << " Hz." << std::endl;
        huntmaster::MFCCProcessor::Config newConfig;
        newConfig.sampleRate = sampleRate;
        mfccProcessor = std::make_unique<huntmaster::MFCCProcessor>(newConfig);
    }

    static int sessionCounter = 0;
    return ++sessionCounter;
}

void HuntmasterAudioEngine::processAudioChunk(int sessionId, const float *audioBuffer, int bufferSize)
{
    if (!mfccProcessor)
    {
        std::cerr << "Error: Cannot process audio chunk. MFCC Processor not initialized." << std::endl;
        return;
    }

    sessionAudioBuffer.insert(sessionAudioBuffer.end(), audioBuffer, audioBuffer + bufferSize);

    const auto &config = mfccProcessor->getConfig();

    while (sessionAudioBuffer.size() >= config.frameSize)
    {
        auto frame = mfccProcessor->processFrame(sessionAudioBuffer.data());
        sessionFeatures.push_back(frame.coefficients);
        sessionAudioBuffer.erase(sessionAudioBuffer.begin(), sessionAudioBuffer.begin() + config.hopSize);
    }
}

float HuntmasterAudioEngine::getSimilarityScore(int sessionId)
{
    if (masterCallFeatures.empty() || sessionFeatures.empty())
    {
        std::cout << "Not enough data to calculate score yet." << std::endl;
        return 0.0f;
    }

    float distance = huntmaster::DTWProcessor::calculateDistance(masterCallFeatures, sessionFeatures);
    std::cout << "  - Raw DTW Distance: " << distance << std::endl;
    float score = 1.0f / (1.0f + distance);
    return score;
}

void HuntmasterAudioEngine::endRealtimeSession(int sessionId)
{
    std::cout << "Ending realtime session " << sessionId << std::endl;
    sessionAudioBuffer.clear();
    sessionFeatures.clear();
}

void HuntmasterAudioEngine::shutdown()
{
    std::cout << "Shutting down HuntmasterAudioEngine" << std::endl;
}