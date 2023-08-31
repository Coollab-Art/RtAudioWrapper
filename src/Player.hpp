#pragma once

#include <rtaudio/RtAudio.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace RtAudioW {

auto audio_through(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData) -> int;

class Player {
private:
    RtAudio                   _audio; // Owns the stream, we need one RtAudio per Player.
    RtAudio::StreamParameters _parameters;
    unsigned int              _sample_rate;
    unsigned int              _buffer_frames{256}; // 256 sample frames
    size_t                    _cursor{0};
    double                    _duration{0};
    std::vector<float>        _data{};
    unsigned int              _output_channels_number;
    unsigned int              _data_channels_number{2};

public:
    explicit Player(unsigned int output_channels = 2, unsigned int sample_rate = 44100);

    auto open(std::vector<float> data, unsigned int sample_rate = 44100, unsigned int data_channels = 2, RtAudioCallback callback = &audio_through) -> RtAudioErrorType;
    auto is_open() const -> bool;
    auto play() -> RtAudioErrorType;
    auto pause() -> RtAudioErrorType;
    void close();
    void seek(float time_in_seconds);

    /// Returns the raw audio data at the given index in the buffer,
    /// or 0 if the index is invalid.
    auto get_data_at(size_t index) const;
    auto get_data_length() const -> size_t;
    auto get_cursor() const -> size_t;
    void set_cursor(size_t position);
    auto get_data_channels() const -> unsigned int;
    auto get_output_channels() const -> unsigned int;

    auto is_API_available() const -> bool;
    auto is_device_available() -> bool;
};

} // namespace RtAudioW