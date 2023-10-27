#include "InputStream.hpp"
#include <utility>

namespace RtAudioW {

InputStream::InputStream(AudioInputCallback callback, RtAudioErrorCallback error_callback)
    : _callback{std::move(callback)}
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

auto audio_input_callback(void* /* output_buffer */, void* input_buffer, unsigned int frames_count, double /* stream_time */, RtAudioStreamStatus /* status */, void* user_data) -> int
{
    static_cast<InputStream*>(user_data)->_callback(std::span{static_cast<float*>(input_buffer), frames_count});
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
    unsigned int nb_frames{512}; // TODO(Audio) Allow users to customize this?
    _backend.openStream(nullptr, &params, RTAUDIO_FLOAT32, info.preferredSampleRate, &nb_frames, &audio_input_callback, this);
    _backend.startStream();
    _current_input_device_name = info.name;
}

} // namespace RtAudioW