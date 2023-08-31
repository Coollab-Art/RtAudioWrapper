#include "Player.hpp"
#include <rtaudio/RtAudio.h>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

namespace RtAudioW {

Player::Player(unsigned int output_channels, unsigned int sample_rate)
    : _sample_rate{sample_rate}
    , _output_channels_number{output_channels}
{
    assert(is_API_available());
    assert(is_device_available());

    _parameters.deviceId     = _audio.getDefaultOutputDevice();
    _parameters.firstChannel = 0;
    _parameters.nChannels    = output_channels;
}

auto Player::is_API_available() const -> bool
{
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    return apis[0] != RtAudio::Api::RTAUDIO_DUMMY;
}

auto Player::is_device_available() -> bool
{
    return !_audio.getDeviceIds().empty();
}

auto Player::open(std::vector<float> data, int sample_rate, int data_channels, RtAudioCallback callback) -> RtAudioErrorType
{
    if (is_open()) // TODO tester que ça marche quand on ouvre un deuxième stream audio
        close();
    RtAudio::StreamParameters* out;
    RtAudio::StreamParameters* in;

    _data     = std::move(data);
    _duration = (float)_data.size() / data_channels / sample_rate;

    out                   = &_parameters;
    in                    = nullptr;
    _data_channels_number = data_channels;
    _sample_rate          = sample_rate;

    return _audio.openStream(out, in, RTAUDIO_FLOAT32, _sample_rate, &_buffer_frames, callback, this);
}

auto Player::is_open() const -> bool
{
    return _audio.isStreamOpen();
}

auto Player::play() -> RtAudioErrorType
{
    return _audio.startStream();
}

auto Player::pause() -> RtAudioErrorType
{
    return _audio.stopStream();
}

void Player::close()
{
    return _audio.closeStream();
}

void Player::seek(float time_in_seconds)
{
    _cursor = static_cast<size_t>(_sample_rate * _data_channels_number * time_in_seconds);
}

auto Player::get_data_at(size_t index) const
{
    if (index >= _data.size())
        return 0.f;
    return _data[index];
}

auto Player::get_data_length() const -> size_t
{
    return _data.size();
}

auto Player::get_cursor() const -> size_t
{
    return _cursor;
}

void Player::set_cursor(size_t position)
{
    _cursor = position;
}

auto Player::get_data_channels() const -> unsigned int
{
    return _data_channels_number;
}
auto Player::get_output_channels() const -> unsigned int
{
    return _output_channels_number;
}

auto audio_through(void* output_buffer, void* /* input_buffer */, unsigned int nBufferFrames, double /* stream_time */, RtAudioStreamStatus /* status */, void* user_data) -> int
{
    auto* buffer = static_cast<float*>(output_buffer);
    auto& player = *static_cast<Player*>(user_data);

    auto const output_channels = player.get_output_channels();
    auto const data_channels   = player.get_data_channels(); // TODO regarder et utiliser les bonnes valeurs au bon endroit
    for (size_t i = 0; i < nBufferFrames; i++)
    {
        for (size_t channel = 0; channel < output_channels; ++channel)
        {
            auto const index_in_buffer = i * output_channels + channel;
            buffer[index_in_buffer]    = player.get_data_at(player.get_cursor() + index_in_buffer);
        }
    }

    player.set_cursor(player.get_cursor() + nBufferFrames * output_channels);
    if (player.get_cursor() > player.get_data_length())
        player.set_cursor(0); // Loop from 0

    return 0;
}

} // namespace RtAudioW
