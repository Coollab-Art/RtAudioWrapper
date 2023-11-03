#include "InputStream.hpp"
#include <utility>

namespace RtAudioW {

InputStream::InputStream(RtAudioErrorCallback error_callback)
{
    _backend.setErrorCallback(std::move(error_callback));
    set_device(_backend.getDefaultInputDevice());
}

auto InputStream::device_ids() const -> std::vector<unsigned int>
{
    auto ids = _backend.getDeviceIds();
    // Keep only the input devices
    std::erase_if(ids, [&](unsigned int id) {
        auto const info = _backend.getDeviceInfo(id);
        return info.inputChannels == 0;
    });
    return ids;
}

auto InputStream::device_info(unsigned int device_id) const -> RtAudio::DeviceInfo
{
    return _backend.getDeviceInfo(device_id);
}

void InputStream::shrink_samples_to_fit()
{
    while (_samples.size() > _nb_of_retained_samples && !_samples.empty())
        _samples.pop_front();
}

void InputStream::set_nb_of_retained_samples(size_t samples_count)
{
    _nb_of_retained_samples = samples_count;
    shrink_samples_to_fit();
}

void InputStream::for_each_sample(size_t samples_count, std::function<void(float)> const& callback)
{
    set_nb_of_retained_samples(samples_count); // Now we know exactly how many to store, the next calls to `for_each_sample()` (if they are done with the same `samples_count`) will have the exact data that they want, with no dummy 0s to fill in the missing data.

    auto const samples = _samples; // TODO(Audio) better way of ensuring thread safety
    if (samples_count > samples.size())
        for (size_t i = 0; i < samples_count - samples.size(); ++i)
            callback(0.f); // Fill in the first potentially missing samples with 0s.
    for (float const sample : samples)
        callback(sample);
}

auto audio_input_callback(void* /* output_buffer */, void* input_buffer, unsigned int frames_count, double /* stream_time */, RtAudioStreamStatus /* status */, void* user_data) -> int
{
    auto const input = std::span{static_cast<float*>(input_buffer), frames_count};
    auto&      This  = *static_cast<InputStream*>(user_data);

    for (float const sample : input)
    {
        This._samples.push_back(sample);
        This.shrink_samples_to_fit();
    }
    return 0;
}

void InputStream::set_device(unsigned int device_id)
{
    if (_backend.isStreamOpen())
        _backend.closeStream();

    auto const                info = _backend.getDeviceInfo(device_id);
    RtAudio::StreamParameters params;
    params.deviceId  = device_id;
    params.nChannels = 1;
    unsigned int nb_frames{512};                         // TODO(Audio) Allow users to customize this?
    auto const   sample_rate = info.preferredSampleRate; // TODO(Audio) Should we use preferredSampleRate or currentSampleRate?
    _backend.openStream(nullptr, &params, RTAUDIO_FLOAT32, sample_rate, &nb_frames, &audio_input_callback, this);
    _backend.startStream();
    _current_input_device_name        = info.name;
    _current_input_device_sample_rate = sample_rate;
}

} // namespace RtAudioW