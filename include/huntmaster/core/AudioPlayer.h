/**
 * @file AudioPlayer.h
 * @brief Audio playback interface for wildlife call reproduction
 * 
 * This file provides the AudioPlayer class which handles audio file loading
 * and playback for reference calls, master calls, and user recordings. The
 * player supports common audio formats and provides precise control over
 * playback timing and volume for analysis and comparison purposes.
 * 
 * @author Huntmaster Development Team
 * @date 2024
 * @copyright All Rights Reserved
 * @version 4.1
 */

#pragma once
#include <memory>
#include <string>

namespace huntmaster {

/**
 * @class AudioPlayer
 * @brief Audio playback interface optimized for wildlife call reproduction
 * 
 * The AudioPlayer class provides a simple yet powerful interface for playing
 * back audio files, particularly focused on wildlife call analysis scenarios.
 * It supports loading various audio formats and provides precise playback
 * control for comparison and analysis purposes.
 * 
 * Key features:
 * - Support for common audio formats (WAV, MP3, etc.)
 * - Precise playback position control
 * - Volume control for comparative analysis
 * - Non-blocking playback operations
 * - Thread-safe playback state management
 * 
 * @example
 * @code
 * huntmaster::AudioPlayer player;
 * 
 * // Load and play a master call
 * if (player.loadFile("master_calls/deer_grunt.wav")) {
 *     player.setVolume(0.8f);
 *     player.play();
 *     
 *     // Monitor playback
 *     while (player.isPlaying()) {
 *         double position = player.getCurrentPosition();
 *         double duration = player.getDuration();
 *         std::cout << "Playing: " << position << "/" << duration << "s" << std::endl;
 *         std::this_thread::sleep_for(std::chrono::milliseconds(100));
 *     }
 * }
 * @endcode
 */
class AudioPlayer {
  public:
    /**
     * @brief Construct a new AudioPlayer object
     * 
     * Creates an AudioPlayer instance ready for loading and playing audio files.
     * The player is initially in a stopped state with default volume settings.
     */
    AudioPlayer();
    
    /**
     * @brief Destroy the AudioPlayer object
     * 
     * Automatically stops any active playback and releases all audio resources.
     */
    ~AudioPlayer();

    /**
     * @brief Load an audio file for playback
     * 
     * Loads the specified audio file into memory and prepares it for playback.
     * Supports common audio formats including WAV, MP3, and other formats
     * supported by the underlying audio system.
     * 
     * @param filename Path to the audio file to load
     * @return true if the file was loaded successfully, false otherwise
     * 
     * @note Loading a new file will stop any current playback
     * @note The file is validated for format compatibility and integrity
     * 
     * @example
     * @code
     * AudioPlayer player;
     * if (player.loadFile("wildlife_calls/turkey_gobble.wav")) {
     *     std::cout << "File loaded, duration: " << player.getDuration() << "s" << std::endl;
     * } else {
     *     std::cerr << "Failed to load audio file" << std::endl;
     * }
     * @endcode
     */
    bool loadFile(const std::string& filename);
    
    /**
     * @brief Start playback of the loaded audio file
     * 
     * Begins playback of the currently loaded audio file from the beginning
     * or from the current position. This method is non-blocking and returns
     * immediately after starting playback.
     * 
     * @return true if playback started successfully, false otherwise
     * 
     * @note Requires a valid audio file to be loaded first
     * @note If already playing, this method has no effect
     * 
     * @see stop() to halt playback
     * @see isPlaying() to check playback status
     */
    bool play();
    
    /**
     * @brief Stop audio playback
     * 
     * Immediately stops any active playback and resets the playback position
     * to the beginning of the file. This method blocks until playback has
     * been completely stopped.
     * 
     * @note Safe to call even if no playback is active
     * @note Playback position is reset to the beginning
     */
    void stop();
    
    /**
     * @brief Check if audio is currently playing
     * 
     * @return true if audio is currently playing, false otherwise
     * 
     * @note This method provides real-time playback status
     */
    bool isPlaying() const;

    /**
     * @brief Get the total duration of the loaded audio file
     * 
     * Returns the total duration of the currently loaded audio file in seconds.
     * This information is useful for progress indicators and timing analysis.
     * 
     * @return Total audio duration in seconds, or 0.0 if no file is loaded
     * 
     * @note Duration is available immediately after successful file loading
     */
    double getDuration() const;
    
    /**
     * @brief Get the current playback position
     * 
     * Returns the current position within the audio file in seconds from
     * the beginning. This can be used for creating progress indicators
     * and synchronizing with other operations.
     * 
     * @return Current playback position in seconds
     * 
     * @note Returns 0.0 when playback is stopped or no file is loaded
     * @note Position is updated in real-time during playback
     */
    double getCurrentPosition() const;
    
    /**
     * @brief Set the playback volume level
     * 
     * Adjusts the playback volume to the specified level. This is useful
     * for normalizing audio levels when comparing different recordings
     * or accommodating different listening environments.
     * 
     * @param volume Volume level in the range [0.0, 1.0]
     *               - 0.0 = muted
     *               - 1.0 = maximum volume
     * 
     * @note Volume changes take effect immediately during playback
     * @note Values outside the valid range are clamped to [0.0, 1.0]
     * 
     * @example
     * @code
     * player.setVolume(0.5f);  // Set to 50% volume
     * player.setVolume(0.0f);  // Mute
     * player.setVolume(1.0f);  // Maximum volume
     * @endcode
     */
    void setVolume(float volume);  // 0.0 to 1.0

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}  // namespace huntmaster
