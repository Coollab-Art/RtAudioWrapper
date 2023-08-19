#include <rtaudio/RtAudio.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace RtAudioW {

auto audio_through(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData) -> int;

class Player {
private:
public:
    // TODO tout mettre en privé, et mettre des _ devant les noms (e.g. `_audio`)
    // TODO snake_case
    RtAudio                   audio; // Owns the stream, we need one RtAudio per Player.
    RtAudio::StreamParameters parameters;
    unsigned int              sampleRate;
    unsigned int              bufferFrames; // 256 sample frames
    unsigned int              cursor{0};
    double                    duration;
    std::vector<float>        _data{};
    unsigned int              channels_count_in_data;

    explicit Player(int channels = 2);

    auto open(std::vector<float> data, int sample_rate = 44100, int channels = 2, RtAudioCallback callback = &audio_through) -> RtAudioErrorType;
    auto isOpen() -> bool;
    auto play() -> RtAudioErrorType;
    auto pause() -> RtAudioErrorType;
    void close();
    void seek(float time_in_seconds);

    /// Returns the raw audio data at the given index in the buffer,
    /// or 0 if the index is invalid.
    auto get_data_at(size_t index) const;

    auto is_API_available() const -> bool; // TODO missing const on many methods
    auto is_device_available() -> bool;
};

} // namespace RtAudioW