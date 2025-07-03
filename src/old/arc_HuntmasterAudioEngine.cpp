#include "../include/huntmaster_engine/HuntmasterAudioEngine.h"
#include "../include/huntmaster_engine/MFCCProcessor.h"
#include "../include/huntmaster_engine/DTWProcessor.h"
#include "../include/huntmaster_engine/AudioRecorder.h"
#include "../libs/dr_wav.h"
#include "../include/huntmaster_engine/AudioPlayer.h"

#include <mutex>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <unordered_map>
#include <deque>

HuntmasterAudioEngine::HuntmasterAudioEngine() = default;
HuntmasterAudioEngine::~HuntmasterAudioEngine() = default;

HuntmasterAudioEngine &HuntmasterAudioEngine::getInstance()
{
    static HuntmasterAudioEngine instance;
    return instance;
}

void HuntmasterAudioEngine::initialize()
{
    // Initialize the audio player instance.
    audioPlayer = std::make_unique<huntmaster::AudioPlayer>();

    // Centralize path management
    masterCallsPath_ = "../data/master_calls/";
    featuresPath_ = "../data/features/";
    recordingsPath_ = "../data/recordings/";

    std::cout << "HuntmasterAudioEngine initialized successfully." << std::endl;
}

void HuntmasterAudioEngine::shutdown()
{
    std::lock_guard<std::mutex> lock(recordingsMutex_);
    // Use a temporary copy of keys to avoid iterator invalidation issues while stopping.
    std::vector<int> recordingIds;
    for (const auto &pair : activeRecordings)
    {
        recordingIds.push_back(pair.first);
    }
    for (int id : recordingIds)
    {
        stopRecording(id);
    }

    // Stop any playback.
    if (audioPlayer)
    {
        audioPlayer->stop();
    }
    std::cout << "Shutting down HuntmasterAudioEngine" << std::endl;
}

// --- Analysis and Scoring ---

void HuntmasterAudioEngine::loadMasterCall(const std::string &masterCallId)
{
    // RAII wrapper for drwav data to ensure it's always freed.
    struct DrWavDeleter
    {
        void operator()(float *p) const
        {
            if (p)
                drwav_free(p, nullptr);
        }
    };

    std::cout << "Loading master call with ID: " << masterCallId << std::endl;
    if (loadFeaturesFromFile(masterCallId))
    {
        std::cout << "  - Successfully loaded pre-computed features from file." << std::endl;
        return;
    }

    std::cout << "  - No pre-computed features found. Processing from source audio..." << std::endl;
    std::string audioFilePath = masterCallsPath_ + masterCallId + ".wav";
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;
    float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(audioFilePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (pSampleData == nullptr)
    {
        std::cerr << "ERROR: Could not load WAV file: " << audioFilePath << std::endl;
        return;
    }

    std::cout << "  - File loaded: " << audioFilePath << std::endl;
    std::cout << "  - Channels: " << channels << ", Sample Rate: " << sampleRate << std::endl;

    if (!mfccProcessor || mfccProcessor->getConfig().sampleRate != sampleRate)
    {
        std::cout << "  - Configuring MFCC processor for " << sampleRate << " Hz." << std::endl;
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
    std::string featureFilePath = featuresPath_ + masterCallId + ".mfc";
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
    std::string featureFilePath = featuresPath_ + masterCallId + ".mfc";
    std::ifstream inFile(featureFilePath, std::ios::binary);
    if (!inFile.is_open())
        return false;
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
    std::lock_guard<std::mutex> lock(realtimeSessionsMutex_);
    int sessionId = nextRealtimeSessionId_++;

    std::cout << "Starting realtime session - Sample Rate: " << sampleRate << ", Buffer Size: " << bufferSize << std::endl;

    // Create and initialize a new session state
    activeRealtimeSessions_[sessionId] = RealtimeSessionState();
    // Set VAD config based on sample rate
    vad_config.minSoundDurationMs = 100.0f;  // Example: 100ms
    vad_config.minSilenceDurationMs = 50.0f; // Example: 50ms
    vad_config.hangoverDurationMs = 100.0f;  // Example: 100ms
    // ... other VAD config adjustments based on sampleRate if needed ...

    if (!mfccProcessor || mfccProcessor->getConfig().sampleRate != sampleRate)
    {
        std::cout << "  - Configuring MFCC processor for live session sample rate: " << sampleRate << " Hz." << std::endl;
        huntmaster::MFCCProcessor::Config newConfig;
        newConfig.sampleRate = sampleRate;
        mfccProcessor = std::make_unique<huntmaster::MFCCProcessor>(newConfig);
    }
    return sessionId;
}

void HuntmasterAudioEngine::processAudioChunk(int sessionId, const float *audioBuffer, int bufferSize)
{
    std::lock_guard<std::mutex> lock(realtimeSessionsMutex_);
    auto it = activeRealtimeSessions_.find(sessionId);
    if (it == activeRealtimeSessions_.end())
    {
        std::cerr << "Error: Invalid session ID in processAudioChunk: " << sessionId << std::endl;
        return;
    }
    RealtimeSessionState &session = it->second;

    if (!mfccProcessor)
    {
        std::cerr << "Error: Cannot process audio chunk. MFCC Processor not initialized." << std::endl;
        return;
    }

    // Append the new audio chunk to the session's audio buffer (now a deque)
    session.audioBuffer.insert(session.audioBuffer.end(), audioBuffer, audioBuffer + bufferSize);
    const auto &mfcc_config = mfccProcessor->getConfig();
    int window_size_frames = static_cast<int>(vad_config.windowDurationMs * mfcc_config.sampleRate / 1000.0f);
    if (window_size_frames == 0)
        window_size_frames = 1; // Ensure window is at least 1 frame

    int min_sound_frames_vad = static_cast<int>(vad_config.minSoundDurationMs * mfcc_config.sampleRate / 1000.0f);
    int min_silence_frames_vad = static_cast<int>(vad_config.minSilenceDurationMs * mfcc_config.sampleRate / 1000.0f);
    int hangover_samples = static_cast<int>(vad_config.hangoverDurationMs * mfcc_config.sampleRate / 1000.0f);

    // Process the incoming audioBuffer in VAD windows
    size_t processed_samples_in_session_buffer = 0;
    while (processed_samples_in_session_buffer + window_size_frames <= session.audioBuffer.size())
    {
        // Calculate energy and peak for the current window
        float energy = calculateEnergy(session.audioBuffer, processed_samples_in_session_buffer, window_size_frames);

        // Calculate peak in current window
        float peak_in_window = 0.0f;
        size_t window_start = processed_samples_in_session_buffer;
        size_t window_end = std::min(window_start + window_size_frames, session.audioBuffer.size());
        for (size_t j = window_start; j < window_end; ++j)
        {
            peak_in_window = std::max(peak_in_window, std::abs(session.audioBuffer[j]));
        }

        bool is_active_audio = (energy > vad_config.energyThreshold || peak_in_window > vad_config.silenceThreshold);

        if (is_active_audio)
        {
            session.consecutive_sound_frames += window_size_frames;
            session.consecutive_silence_frames = 0;

            if (!session.is_in_sound_segment && session.consecutive_sound_frames >= min_sound_frames_vad)
            {
                // Sound segment just started
                std::cout << " (Sound detected) ";
                session.is_in_sound_segment = true;
            }
        }
        else // Silence detected
        {
            session.consecutive_silence_frames += window_size_frames;
            session.consecutive_sound_frames = 0;

            if (session.is_in_sound_segment && session.consecutive_silence_frames >= min_silence_frames_vad)
            {
                // Sound segment just ended, apply hangover
                std::cout << " (Silence detected, processing segment) ";
                session.is_in_sound_segment = false;
                // Mark the end of the current sound segment, including hangover
                size_t segment_end_index = processed_samples_in_session_buffer + window_size_frames;
                size_t actual_segment_end = std::min(segment_end_index + static_cast<size_t>(hangover_samples), session.audioBuffer.size());

                // Extract the segment to process
                // The issue is that std::deque does not have a .data() method for contiguous memory access.
                // We need to copy the deque content to a std::vector to pass it to processBuffer.
                std::vector<float> segment_to_process(session.current_vad_segment_buffer.begin(), session.current_vad_segment_buffer.end());

                segment_to_process.insert(segment_to_process.end(),
                                          session.audioBuffer.begin() + processed_samples_in_session_buffer - (session.mfcc_processed_frames_in_current_vad_segment % mfcc_config.hopSize), // Adjust for partial hop
                                          session.audioBuffer.begin() + actual_segment_end);

                // Process the accumulated sound segment for MFCCs
                auto mfccFrames = mfccProcessor->processBuffer(segment_to_process.data(), segment_to_process.size());
                for (const auto &frame : mfccFrames)
                {
                    session.features.push_back(frame.coefficients);
                }
                session.current_vad_segment_buffer.clear(); // Clear the buffer after processing
                session.mfcc_processed_frames_in_current_vad_segment = 0;
                session.consecutive_silence_frames = 0; // Reset silence counter after processing a segment
            }
        }

        // Append audio to the current VAD segment buffer
        if (session.is_in_sound_segment || (session.consecutive_silence_frames > 0 && session.consecutive_silence_frames <= hangover_samples))
        {
            session.current_vad_segment_buffer.insert(session.current_vad_segment_buffer.end(),
                                                      session.audioBuffer.begin() + processed_samples_in_session_buffer,
                                                      session.audioBuffer.begin() + processed_samples_in_session_buffer + window_size_frames);
        }

        processed_samples_in_session_buffer += window_size_frames;
    }

    // Remove processed samples from session.audioBuffer
    if (processed_samples_in_session_buffer > 0)
    {
        session.audioBuffer.erase(session.audioBuffer.begin(), session.audioBuffer.begin() + processed_samples_in_session_buffer);
    }

    // Process MFCCs from the session.current_vad_segment_buffer if a sound segment is active
    const auto &mfcc_processor_config = mfccProcessor->getConfig();
    while (session.current_vad_segment_buffer.size() >= session.mfcc_processed_frames_in_current_vad_segment + mfcc_processor_config.frameSize)
    {
        // Copy the relevant part of the deque to a temporary vector for processing
        std::vector<float> frame_data(session.current_vad_segment_buffer.begin() + session.mfcc_processed_frames_in_current_vad_segment,
                                      session.current_vad_segment_buffer.begin() + session.mfcc_processed_frames_in_current_vad_segment + mfcc_processor_config.frameSize);
        auto frame = mfccProcessor->processFrame(frame_data.data());
        session.features.push_back(frame.coefficients);
        session.mfcc_processed_frames_in_current_vad_segment += mfcc_processor_config.hopSize;
    }
}

float HuntmasterAudioEngine::getSimilarityScore(int sessionId)
{
    std::lock_guard<std::mutex> lock(realtimeSessionsMutex_);
    auto it = activeRealtimeSessions_.find(sessionId);
    if (it == activeRealtimeSessions_.end())
    {
        std::cerr << "Error: Invalid session ID in getSimilarityScore: " << sessionId << std::endl;
        return 0.0f;
    }
    const RealtimeSessionState &session = it->second;

    if (masterCallFeatures.empty() || session.features.empty())
    {
        std::cout << "Not enough data to calculate score yet." << std::endl;
        return 0.0f;
    }
    float distance = huntmaster::DTWProcessor::calculateDistance(masterCallFeatures, session.features);
    std::cout << "  - Raw DTW Distance: " << distance << std::endl;
    float score = 1.0f / (1.0f + distance);
    return score;
}

void HuntmasterAudioEngine::endRealtimeSession(int sessionId)
{
    std::lock_guard<std::mutex> lock(realtimeSessionsMutex_);
    std::cout << "Ending realtime session " << sessionId << std::endl;
    activeRealtimeSessions_.erase(sessionId);
}

// --- Recording ---

int HuntmasterAudioEngine::startRecording(double sampleRate)
{
    std::lock_guard<std::mutex> lock(recordingsMutex_);
    int recordingId = nextRecordingId++;
    auto session = std::make_unique<RecordingSession>();
    session->recorder = std::make_unique<huntmaster::AudioRecorder>();
    session->id = recordingId;
    session->sampleRate = sampleRate;
    huntmaster::AudioRecorder::Config config;
    config.sampleRate = static_cast<int>(sampleRate);
    config.channels = 1;
    if (session->recorder->startRecording(config))
    {
        activeRecordings.emplace(recordingId, std::move(*session));
        std::cout << "Started recording session " << recordingId << std::endl;
        return recordingId;
    }
    return -1;
}

void HuntmasterAudioEngine::stopRecording(int recordingId)
{
    std::lock_guard<std::mutex> lock(recordingsMutex_);
    auto it = activeRecordings.find(recordingId);
    if (it != activeRecordings.end())
    {
        it->second.recorder->stopRecording();
        std::cout << "Stopped recording session " << recordingId << std::endl;
    }
}

float HuntmasterAudioEngine::calculateEnergy(const std::deque<float> &samples, size_t start, size_t windowSize)
{
    float sum = 0.0f;
    size_t end = std::min(start + windowSize, samples.size());

    for (size_t i = start; i < end; ++i)
    {
        sum += samples[i] * samples[i];
    }

    return (end > start) ? sum / (end - start) : 0.0f;
}

std::string HuntmasterAudioEngine::saveRecording(int recordingId, const std::string &filename)
{
    std::lock_guard<std::mutex> lock(recordingsMutex_);
    auto it = activeRecordings.find(recordingId);
    if (it != activeRecordings.end())
    {
        std::string fullPath = recordingsPath_ + filename + ".wav";
        if (it->second.recorder->saveToWav(fullPath))
        {
            activeRecordings.erase(it);
            return fullPath;
        }
    }
    return "";
}

bool HuntmasterAudioEngine::isRecording() const
{
    std::lock_guard<std::mutex> lock(recordingsMutex_);
    if (activeRecordings.empty())
        return false;
    for (const auto &pair : activeRecordings)
    {
        if (pair.second.recorder->isRecording())
        {
            return true;
        }
    }
    return false;
}

float HuntmasterAudioEngine::getRecordingLevel() const
{
    std::lock_guard<std::mutex> lock(recordingsMutex_);
    float maxLevel = 0.0f;
    for (const auto &pair : activeRecordings)
    {
        float level = pair.second.recorder->getCurrentLevel();
        if (level > maxLevel)
            maxLevel = level;
    }
    return maxLevel;
}

double HuntmasterAudioEngine::getRecordingDuration(int recordingId) const
{
    std::lock_guard<std::mutex> lock(recordingsMutex_);
    auto it = activeRecordings.find(recordingId);
    if (it != activeRecordings.end())
    {
        return it->second.recorder->getDuration();
    }
    return 0.0;
}

// --- Playback ---

void HuntmasterAudioEngine::playMasterCall(const std::string &callId)
{
    std::cout << "Request to play master call: " << callId << std::endl;
    std::string filePath = masterCallsPath_ + callId + ".wav";
    if (audioPlayer->loadFile(filePath))
    {
        audioPlayer->play();
    }
    else
    {
        std::cerr << "Failed to load master call: " << filePath << std::endl;
    }
}

void HuntmasterAudioEngine::playRecording(const std::string &filename)
{
    std::cout << "Request to play recording: " << filename << std::endl;
    std::string filePath = recordingsPath_ + filename; // Assumes full filename like "my_rec.wav"
    if (audioPlayer->loadFile(filePath))
    {
        audioPlayer->play();
    }
}

void HuntmasterAudioEngine::stopPlayback()
{
    std::cout << "Request to stop playback" << std::endl;
    if (audioPlayer)
    {
        audioPlayer->stop();
    }
}
