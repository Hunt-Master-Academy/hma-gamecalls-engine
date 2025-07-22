#pragma once
#include <memory>
#include <string>

namespace huntmaster {

class AudioPlayer {
  public:
    AudioPlayer();
    ~AudioPlayer();

    bool loadFile(const std::string& filename);
    bool play();
    void stop();
    bool isPlaying() const;

    double getDuration() const;
    double getCurrentPosition() const;
    void setVolume(float volume);  // 0.0 to 1.0

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}  // namespace huntmaster
