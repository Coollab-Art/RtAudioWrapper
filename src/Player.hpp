#pragma once

#include <rtaudio/RtAudio.h>
#include <cstdint>
#include <vector>

namespace RtAudioW {

struct AudioData {
    /// All the samples. If `channels_count` is > 1, the data MUST be in interleaved format:
    /// [Frame 0 | Channel 0]
    /// [Frame 0 | Channel 1]
    /// [Frame 1 | Channel 0]
    /// [Frame 1 | Channel 1]
    /// For a definition of Frame, see https://youtu.be/jNSiZqSQis4?t=937
    std::vector<float> samples{};

    /// The number of frames per second.
    unsigned int sample_rate{};

    /// The number of channels (usually 1 or 2, mono or stereo).
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
    ~Player()                                    = default;
    Player(Player const&)                        = delete; // Can't copy nor move
    auto operator=(Player const&) -> Player&     = delete; // because we pass the address of this object to the audio callback.
    Player(Player&&) noexcept                    = delete; // And you should be using the global instance returned by
    auto operator=(Player&&) noexcept -> Player& = delete; // RtAudioW::player() anyways.

    /// Receives some data (e.g. a song coming from an mp3 file) and stores it.
    /// After that, the player is ready to play it whenever play() will be called (or starts playing immediately if play() has already been called).
    void set_audio_data(AudioData);
    /// Deletes the data that was set with set_audio_data().
    void reset_audio_data();
    /// Getter for the audio data.
    auto audio_data() const -> AudioData const& { return _data; }
    /// True iff some data has been set with set_audio_data() and not reset with reset_audio_data().
    auto has_audio_data() const -> bool;

    /// Returns the value of the audio data at the given position in time, while taking all the player properties into account.
    auto sample(int64_t frame_index, int64_t channel_index) const -> float;
    /// Returns the value of the audio data at the given position in time, while ignoring the `volume` and `is_muted` properties of the player. It still takes `does_loop` into account.
    auto sample_unaltered_volume(int64_t frame_index, int64_t channel_index) const -> float;
    auto current_frame_index() const -> int64_t { return _next_frame_to_play; }

    /// Used to get and set the properties.
    auto properties() -> PlayerProperties& { return _properties; }
    auto properties() const -> PlayerProperties const& { return _properties; }

    /// Starts or resumes the playing, or does nothing if it was already playing.
    /// If no audio data has been set with set_audio_data(), the actual playing will not start until set_audio_data() is called.
    void play();
    /// Pauses the playing, or does nothing if it was already paused.
    void pause();
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
    int64_t _next_frame_to_play{0}; // Next frame of the `_data.samples` buffer that the player needs to play.
    bool    _is_playing{false};

    // Output device
    unsigned int _current_output_device_id{0}; // 0 is an invalid ID.
};

/// Must be called before any call to player() if you want to be sure to catch all errors.
void set_error_callback(RtAudioErrorCallback);
/// Global instance that you need to use. Having two Players at once doesn't work anyways (because of the way rtaudio handles it I think).
auto player() -> Player&;
/// Call this before your application exits.
void shut_down();

} // namespace RtAudioW