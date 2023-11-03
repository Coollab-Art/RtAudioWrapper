#include "InputStream.hpp"
#include <mutex>
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
    // shrink_samples_to_fit(); // Don't shrink here, this will be done during `for_each_sample()`. This avoids locking too often.
}

void InputStream::for_each_sample(int64_t samples_count, std::function<void(float)> const& callback)
{
    auto const samples = [&]() {
        std::lock_guard const lock{_samples_mutex}; // Lock while we copy
        shrink_samples_to_fit();                    // Might not be fit, if set_nb_of_retained_samples() has been called.
        return _samples;
    }();
    for ( // Take the `samples_count` last elements of `samples`.
        int64_t i = static_cast<int64_t>(samples.size()) - samples_count;
        i < static_cast<int64_t>(samples.size());
        ++i
    )
    {
        if (i < 0) // If `samples` has less than `samples_count` elements this will happen.
            callback(0.f);
        else
            callback(samples[static_cast<size_t>(i)]);
    }
}

auto audio_input_callback(void* /* output_buffer */, void* input_buffer, unsigned int frames_count, double /* stream_time */, RtAudioStreamStatus /* status */, void* user_data) -> int
{
    auto const input = std::span{static_cast<float*>(input_buffer), frames_count};
    auto&      This  = *static_cast<InputStream*>(user_data);

    {
        std::lock_guard const lock{This._samples_mutex};
        for (float const sample : input)
        {
            This._samples.push_back(sample);
            This.shrink_samples_to_fit();
        }
    }
    return 0;
}

void InputStream::set_device(unsigned int device_id)
{
    if (_backend.isStreamOpen())
        _backend.closeStream();

    { // Clear the samples, they do not correspond to the new device. (Shouldn't really matter, but I guess this is technically more correct)
        std::lock_guard const lock{_samples_mutex};
        _samples.clear();
    }

    auto const                info = _backend.getDeviceInfo(device_id);
    RtAudio::StreamParameters params;
    params.deviceId  = device_id;
    params.nChannels = 1;
    unsigned int nb_frames{512};                         // 512 is a decent value that seems to work well.
    auto const   sample_rate = info.preferredSampleRate; // TODO(Audio-Philippe) Should we use preferredSampleRate or currentSampleRate?
    _backend.openStream(nullptr, &params, RTAUDIO_FLOAT32, sample_rate, &nb_frames, &audio_input_callback, this);
    _backend.startStream();
    _current_input_device_name        = info.name;
    _current_input_device_sample_rate = sample_rate;
}

} // namespace RtAudioW