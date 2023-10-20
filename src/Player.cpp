#include "Player.hpp"
#include <cassert>
#include <cstdint>

namespace RtAudioW {

static constexpr int64_t output_channels_count = 2;

static auto backend() -> RtAudio&
{
    static RtAudio instance{};
    return instance;
}

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
    auto const id = backend().getDefaultOutputDevice();
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
                player._is_playing
                    ? player.sample(player._next_frame_to_play, channel_idx)
                    : 0.f;
        }
        if (player._is_playing)
            ++player._next_frame_to_play;
    }

    return 0;
}

void Player::recreate_stream_adapted_to_current_audio_data()
{
    if (backend().isStreamOpen())
        backend().closeStream();

    if (!has_audio_data()
        || !has_device())
        return;

    RtAudio::StreamParameters _parameters;
    _parameters.deviceId     = _current_output_device_id;
    _parameters.firstChannel = 0;
    _parameters.nChannels    = output_channels_count;
    unsigned int nb_frames_per_callback{128};

    backend().openStream(
        &_parameters,
        nullptr, // No input stream needed
        RTAUDIO_FLOAT32,
        _data.sample_rate, // TODO(Audio) TODO(Philippe) Resample the audio data to make it match the preferredSampleRate of the device. The current strategy works, unless the device does not support the sample_rate used by our audio data, in which case the audio will be played too slow or too fast.
        &nb_frames_per_callback,
        &audio_callback,
        this
    );

    backend().startStream();
}

void Player::set_audio_data(AudioData data)
{
    if (backend().isStreamOpen())
        backend().closeStream(); // Otherwise data race with the audio thread that is reading _audio_data. Could cause crashes.

    float const current_time = get_time();

    _data = std::move(data);
    set_time(current_time); // Need to adjust the _next_frame_to_play so that we will be at the same point in time in both audios even if they have different sample rates.

    recreate_stream_adapted_to_current_audio_data();
}

void Player::reset_audio_data()
{
    set_audio_data({});
}

void Player::play()
{
    _is_playing = true;
}

void Player::pause()
{
    _is_playing = false;
}

void Player::set_time(float time_in_seconds)
{
    _next_frame_to_play = static_cast<int64_t>(
        static_cast<float>(_data.sample_rate)
        * time_in_seconds
    );
}

auto Player::get_time() const -> float
{
    if (_data.sample_rate == 0)
        return 0.f;
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

auto Player::sample(int64_t frame_index, int64_t channel_index) const -> float
{
    if (_properties.is_muted)
        return 0.f;
    return _properties.volume * sample_unaltered_volume(frame_index, channel_index);
}

auto Player::sample_unaltered_volume(int64_t frame_index, int64_t channel_index) const -> float
{
    auto const sample_index = frame_index * _data.channels_count
                              + channel_index % _data.channels_count;
    if ((sample_index < 0
         || sample_index >= static_cast<int64_t>(_data.samples.size())
        )
        && !_properties.does_loop)
        return 0.f;

    return _data.samples[static_cast<size_t>(mod(sample_index, static_cast<int64_t>(_data.samples.size())))];
}

void set_error_callback(RtAudioErrorCallback callback)
{
    backend().setErrorCallback(std::move(callback));
}

auto player() -> Player&
{
    static Player instance{};
    return instance;
}

} // namespace RtAudioW
