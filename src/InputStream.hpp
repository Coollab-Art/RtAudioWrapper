#pragma once
#include <span>
#include "rtaudio/RtAudio.h"

namespace RtAudioW {

/// Receives a buffer containing audio samples.
/// There is always 1 channel in the input buffer.
using AudioInputCallback = std::function<void(std::span<float>)>;

class InputStream {
public:
    InputStream(AudioInputCallback, RtAudioErrorCallback);
    ~InputStream()                                         = default;
    InputStream(InputStream const&)                        = delete; //
    auto operator=(InputStream const&) -> InputStream&     = delete; // Can't copy nor move
    InputStream(InputStream&&) noexcept                    = delete; // because we pass the address of this object to the audio callback.
    auto operator=(InputStream&&) noexcept -> InputStream& = delete; //

    /// Returns the list of all the ids of input devices.
    auto device_ids() const -> std::vector<unsigned int>;
    /// Returns all the info about a given device.
    auto device_info(unsigned int device_id) const -> RtAudio::DeviceInfo;
    ///
    auto current_device_name() const -> std::string const& { return _current_input_device_name; }
    /// Sets the device to use.
    /// By default, when an InputStream is created it uses the default input device selected by the OS.
    void set_device(unsigned int device_id);

private:
    friend auto audio_input_callback(void* output_buffer, void* input_buffer, unsigned int frames_count, double stream_time, RtAudioStreamStatus status, void* user_data) -> int;

private:
    mutable RtAudio    _backend{};
    AudioInputCallback _callback{};
    std::string        _current_input_device_name{};
};

} // namespace RtAudioW