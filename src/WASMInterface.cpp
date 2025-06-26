#include <emscripten/emscripten.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include "../include/huntmaster_engine/MFCCProcessor.h"
#include "../include/huntmaster_engine/DTWProcessor.h"

// Simplified engine for WASM
class WASMHuntmasterEngine
{
private:
    std::unique_ptr<huntmaster::MFCCProcessor> mfccProcessor;
    std::vector<std::vector<float>> masterFeatures;
    std::vector<std::vector<float>> sessionFeatures;
    std::vector<float> audioBuffer;
    int currentSessionId = 0;

public:
    WASMHuntmasterEngine() = default;
    ~WASMHuntmasterEngine() = default;

    bool loadMasterFeatures(const float *mfccData, int numFrames, int numCoeffs)
    {
        masterFeatures.clear();
        masterFeatures.reserve(numFrames);

        for (int i = 0; i < numFrames; ++i)
        {
            std::vector<float> frame(numCoeffs);
            for (int j = 0; j < numCoeffs; ++j)
            {
                frame[j] = mfccData[i * numCoeffs + j];
            }
            masterFeatures.push_back(frame);
        }

        return true;
    }

    int startSession(float sampleRate, int frameSize, int hopSize)
    {
        sessionFeatures.clear();
        audioBuffer.clear();

        if (!mfccProcessor || mfccProcessor->getConfig().sampleRate != sampleRate)
        {
            huntmaster::MFCCProcessor::Config config;
            config.sampleRate = sampleRate;
            config.frameSize = frameSize;
            config.hopSize = hopSize;
            mfccProcessor = std::make_unique<huntmaster::MFCCProcessor>(config);
        }

        return ++currentSessionId;
    }

    void processAudioChunk(const float *audioData, int numSamples)
    {
        if (!mfccProcessor)
            return;

        // Append to buffer
        audioBuffer.insert(audioBuffer.end(), audioData, audioData + numSamples);

        // Process complete frames
        const auto &config = mfccProcessor->getConfig();
        while (audioBuffer.size() >= config.frameSize)
        {
            auto frame = mfccProcessor->processFrame(audioBuffer.data());
            sessionFeatures.push_back(frame.coefficients);

            // Hop forward
            audioBuffer.erase(audioBuffer.begin(), audioBuffer.begin() + config.hopSize);
        }
    }

    float getSimilarityScore()
    {
        if (masterFeatures.empty() || sessionFeatures.empty())
        {
            return 0.0f;
        }

        float distance = huntmaster::DTWProcessor::calculateDistance(masterFeatures, sessionFeatures);
        return 1.0f / (1.0f + distance);
    }

    void endSession()
    {
        sessionFeatures.clear();
        audioBuffer.clear();
    }

    // Get MFCC features for visualization
    int getSessionFeatureCount() const
    {
        return static_cast<int>(sessionFeatures.size());
    }

    void getSessionFeatures(float *output) const
    {
        int idx = 0;
        for (const auto &frame : sessionFeatures)
        {
            for (float coeff : frame)
            {
                output[idx++] = coeff;
            }
        }
    }
};

// Global instance management
static std::unordered_map<int, std::unique_ptr<WASMHuntmasterEngine>> engines;
static int nextEngineId = 1;

// C-style interface for Emscripten
extern "C"
{

    EMSCRIPTEN_KEEPALIVE
    int createEngine()
    {
        int id = nextEngineId++;
        engines[id] = std::make_unique<WASMHuntmasterEngine>();
        return id;
    }

    EMSCRIPTEN_KEEPALIVE
    void destroyEngine(int engineId)
    {
        engines.erase(engineId);
    }

    EMSCRIPTEN_KEEPALIVE
    int loadMasterCall(int engineId, float *mfccData, int numFrames, int numCoeffs)
    {
        auto it = engines.find(engineId);
        if (it != engines.end())
        {
            return it->second->loadMasterFeatures(mfccData, numFrames, numCoeffs) ? 1 : 0;
        }
        return 0;
    }

    EMSCRIPTEN_KEEPALIVE
    int startSession(int engineId, float sampleRate, int frameSize, int hopSize)
    {
        auto it = engines.find(engineId);
        if (it != engines.end())
        {
            return it->second->startSession(sampleRate, frameSize, hopSize);
        }
        return -1;
    }

    EMSCRIPTEN_KEEPALIVE
    void processAudioChunk(int engineId, float *audioData, int numSamples)
    {
        auto it = engines.find(engineId);
        if (it != engines.end())
        {
            it->second->processAudioChunk(audioData, numSamples);
        }
    }

    EMSCRIPTEN_KEEPALIVE
    float getSimilarityScore(int engineId)
    {
        auto it = engines.find(engineId);
        if (it != engines.end())
        {
            return it->second->getSimilarityScore();
        }
        return 0.0f;
    }

    EMSCRIPTEN_KEEPALIVE
    void endSession(int engineId)
    {
        auto it = engines.find(engineId);
        if (it != engines.end())
        {
            it->second->endSession();
        }
    }

    EMSCRIPTEN_KEEPALIVE
    int getSessionFeatureCount(int engineId)
    {
        auto it = engines.find(engineId);
        if (it != engines.end())
        {
            return it->second->getSessionFeatureCount();
        }
        return 0;
    }

    EMSCRIPTEN_KEEPALIVE
    void getSessionFeatures(int engineId, float *output)
    {
        auto it = engines.find(engineId);
        if (it != engines.end())
        {
            it->second->getSessionFeatures(output);
        }
    }

} // extern "C"