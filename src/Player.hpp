#pragma once

#include <rtaudio/RtAudio.h>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace RtAudioW {

// TODO(Audio) Error handing when the various operations can go wrong
// TODO(Audio) Fix all the messages sent by RtAudio in the console

struct AudioData {
    /// All the samples. If `channels_count` is > 1, the data MUST be in interleaved format:
    /// [Frame 0 | Channel 0]
    /// [Frame 0 | Channel 1]
    /// [Frame 1 | Channel 0]
    /// [Frame 1 | Channel 1]
    /// For a definition of Frame, see https://youtu.be/jNSiZqSQis4?t=937
    std::vector<float> samples{};

    /// The number of samples per second.
    unsigned int sample_rate{};

    /// The number of channels (mono vs stereo).
    unsigned int channels_count{};
};

struct PlayerProperties {
    float volume{1.f};
    bool  is_muted{false};
    bool  does_loop{false};
};

/// A player that will output sound to your default output device.
/// You almost always want to use the default device,
/// as users usually want all the audio to come out of it
/// (e.g. if they plug in headphones, they expect all the audio to come out of there
/// and when they unplug them the audio should go back to the default speakers of their computer).
///
/// /!\ IN ORDER FOR THIS TO WORK, you need to regularly call update_device_if_necessary()
/// so that we can check if it has changed and react accordingly.
class Player {
public:
    Player();

    /// Receives some data (e.g. a song coming from an mp3 file) and stores it.
    /// After that, the player is ready to play it whenever play() will be called (or starts playing immediately if play() has already been called).
    void set_audio_data(AudioData);
    /// Deletes the data that was set with set_audio_data().
    void reset_audio_data();
    /// Getter for the audio data.
    auto audio_data() const -> AudioData const& { return _data; }
    /// True iff some data has been set with set_audio_data() and not reset with reset_audio_data().
    auto has_audio_data() const -> bool;

    /// Returns the value of the audio data at the given position, while taking all the player properties into account.
    auto sample(int64_t frame_index, int64_t channel_index) -> float;

    /// Used to get and set the properties.
    auto properties() -> PlayerProperties& { return _properties; }
    auto properties() const -> PlayerProperties const& { return _properties; }

    /// Starts or resumes the playing, or does nothing if it was already playing.
    /// If no audio data has been set with set_audio_data(), the actual playing will not start until set_audio_data() is called.
    auto play() -> RtAudioErrorType;
    /// Pauses the playing, or does nothing if it was already paused.
    auto pause() -> RtAudioErrorType;
    /// Makes the player jump to a specific moment in time.
    void set_time(float time_in_seconds);
    /// Returns the moment in time the player is currently playing.
    auto get_time() const -> float;

    /// Checks if the default device has changed (e.g. the user has just plugged in some headphones)
    /// and switches device accordingly.
    /// You need to call this regularly (e.g. once per application frame), otherwise your application will not react to audio device changes.
    void update_device_if_necessary();

private:
    /// Destroys the current stream if there is one, and then creates a new one with a sample rate matching our audio data (or does not create anything if we have no audio data).
    void recreate_stream_adapted_to_current_audio_data();

    auto has_device() const -> bool;

private:
    friend auto audio_callback(void* output_buffer, void* input_buffer, unsigned int frames_count, double stream_time, RtAudioStreamStatus status, void* user_data) -> int;

private:
    AudioData        _data{};
    PlayerProperties _properties{};

    // Player state
    int64_t _next_frame_to_play{0};          // Next frame of the `_data.samples` buffer that the player needs to play.
    bool    _play_has_been_requested{false}; // If someone calls play() while we don't have audio data yet, we cannot create the stream yet, but still want to remember we are in the playing state and start playing as soon as we have data.

    // Output device
    RtAudio      _backend;                     // Owns the stream and connection to the hardware device.
    unsigned int _current_output_device_id{0}; // 0 is an invalid ID.
};

auto player() -> Player&;

} // namespace RtAudioW