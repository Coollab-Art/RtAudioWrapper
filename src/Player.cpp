#include "Player.hpp"
#include <cassert>
#include <cstdint>

namespace RtAudioW {

static constexpr int64_t output_channels_count = 2;

#ifndef NDEBUG // Only used by the assert, so unused in Release, which would cause a warning.
static auto is_API_available() -> bool
{
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    return apis[0] != RtAudio::Api::RTAUDIO_DUMMY;
}
#endif

Player::Player()
{
    assert(is_API_available());
    update_device_if_necessary();
}

void Player::update_device_if_necessary()
{
    auto const id = _backend.getDefaultOutputDevice();
    if (id == _current_output_device_id)
        return;

    _current_output_device_id = id;
    recreate_stream_adapted_to_current_audio_data();
}

auto Player::has_audio_data() const -> bool
{
    return !_data.samples.empty();
}

auto Player::has_device() const -> bool
{
    return _current_output_device_id != 0;
}

auto audio_callback(void* output_buffer, void* /* input_buffer */, unsigned int frames_count, double /* stream_time */, RtAudioStreamStatus /* status */, void* user_data) -> int
{
    auto* out_buffer = static_cast<float*>(output_buffer);
    auto& player     = *static_cast<Player*>(user_data);

    for (int64_t frame_idx = 0; frame_idx < frames_count; frame_idx++)
    {
        for (int64_t channel_idx = 0; channel_idx < output_channels_count; ++channel_idx)
        {
            out_buffer[frame_idx * output_channels_count + channel_idx] = // NOLINT(*pointer-arithmetic)
                player.sample(player._next_frame_to_play, channel_idx);
        }
        player._next_frame_to_play++;
    }

    return 0;
}

void Player::recreate_stream_adapted_to_current_audio_data()
{
    if (_backend.isStreamOpen())
        _backend.closeStream();

    if (!has_audio_data()
        || !has_device())
        return;

    RtAudio::StreamParameters _parameters;
    _parameters.deviceId     = _current_output_device_id;
    _parameters.firstChannel = 0;
    _parameters.nChannels    = output_channels_count;
    unsigned int nb_frames_per_callback{0}; // 0 means we want the smallest number of frames per callback possible.

    _backend.openStream(
        &_parameters,
        nullptr, // No input stream needed
        RTAUDIO_FLOAT32,
        _data.sample_rate, // TODO(Audio) TODO(Philippe) Resample the audio data to make it match the preferredSampleRate of the device. The current strategy works, unless the device does not support the sample_rate used by our audio data, in which case the audio will be played too slow or too fast.
        &nb_frames_per_callback,
        &audio_callback,
        this
    );

    if (_play_has_been_requested)
        _backend.startStream();
}

void Player::set_audio_data(AudioData data)
{
    _data = std::move(data);
    recreate_stream_adapted_to_current_audio_data();
}

void Player::reset_audio_data()
{
    set_audio_data({});
}

auto Player::play() -> RtAudioErrorType
{
    _play_has_been_requested = true;
    if (!_backend.isStreamOpen()      // We will start playing when we open the stream, i.e. when we receive audio data
        || _backend.isStreamRunning() // The stream is already started, no need to do anything
    )
    {
        return RtAudioErrorType::RTAUDIO_NO_ERROR;
    }
    return _backend.startStream();
}

auto Player::pause() -> RtAudioErrorType
{
    _play_has_been_requested = false;
    if (!_backend.isStreamRunning() /* The stream is already inactive, no need to do anything */)
        return RtAudioErrorType::RTAUDIO_NO_ERROR;
    return _backend.stopStream();
}

void Player::set_time(float time_in_seconds)
{
    // TODO(Audio) Store the desired time in seconds too, so that if we switch to an audio data with a different sample rate, we can adjust the _next_frame_to_play to make it match the actual time in seconds.
    _next_frame_to_play = static_cast<int64_t>(
        static_cast<float>(_data.sample_rate)
        * time_in_seconds
    );
}

auto Player::get_time() const -> float
{
    // TODO(Audio) return the stored desired time and handle when no data (sample_rate == 0)
    return static_cast<float>(_next_frame_to_play)
           / static_cast<float>(_data.sample_rate);
}

static auto mod(int64_t a, int64_t b) -> int64_t
{
    auto res = a % b;
    if (res < 0)
        res += b;
    return res;
}

auto Player::sample(int64_t frame_index, int64_t channel_index) -> float
{
    if (_properties.is_muted)
        return 0.f;

    auto const sample_index = frame_index * _data.channels_count
                              + channel_index % _data.channels_count;
    if ((sample_index < 0
         || sample_index >= static_cast<int64_t>(_data.samples.size())
        )
        && !_properties.does_loop)
        return 0.f;

    return _data.samples[static_cast<size_t>(mod(sample_index, static_cast<int64_t>(_data.samples.size())))]
           * _properties.volume;
}

auto player() -> Player&
{
    static Player instance{};
    return instance;
}

} // namespace RtAudioW
