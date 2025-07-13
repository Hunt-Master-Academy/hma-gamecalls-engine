#pragma once
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <memory>

namespace huntmaster
{

    class AudioRecorder
    {
    public:
        struct Config
        {
            int sampleRate = 44100;
            int channels = 1; // Mono for voice
            int bufferSize = 512;
        };

        bool saveToWavTrimmed(const std::string &filename) const;

        AudioRecorder();
        ~AudioRecorder();

        bool startRecording(const Config &config);
        void stopRecording();
        bool isRecording() const;

        std::vector<float> getRecordedData() const;
        bool saveToWav(const std::string &filename) const;
        float getCurrentLevel() const;
        double getDuration() const;

    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };

} // namespace huntmaster
