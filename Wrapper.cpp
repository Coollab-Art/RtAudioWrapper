#include "Wrapper.h"
#include <rtaudio/RtAudio.h>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

namespace RtAudioW {

Player::Player(int channels)
{
    assert(is_API_available());
    assert(is_device_available());

    {
        // TODO use initializer list
        parameters.deviceId     = audio.getDefaultOutputDevice();
        parameters.firstChannel = 0;
        parameters.nChannels    = channels;
        bufferFrames            = 256;
        cursor                  = 0;
        duration                = 0;
    }
}

auto Player::is_API_available() const -> bool
{
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    return apis[0] != RtAudio::Api::RTAUDIO_DUMMY;
}

auto Player::is_device_available() -> bool
{
    return !audio.getDeviceIds().empty();
}

auto Player::open(std::vector<float> data, int sample_rate, int channels, RtAudioCallback callback) -> RtAudioErrorType
{
    if (isOpen()) // TODO tester que ça marche quand on ouvre un deuxième stream audio
        close();
    RtAudio::StreamParameters* out;
    RtAudio::StreamParameters* in;

    _data = std::move(data);

    out                    = &parameters;
    in                     = nullptr;
    channels_count_in_data = channels;
    sampleRate             = sample_rate;

    return audio.openStream(out, in, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, callback, this);
}

auto Player::play() -> RtAudioErrorType
{
    return audio.startStream();
}

auto Player::pause() -> RtAudioErrorType
{
    return audio.stopStream();
}

void Player::seek(float time_in_seconds)
{
    cursor = static_cast<unsigned int>(sampleRate * channels_count_in_data * time_in_seconds);
}

auto Player::isOpen() -> bool
{
    return audio.isStreamOpen();
}

void Player::close()
{
    return audio.closeStream();
}

auto Player::get_data_at(size_t index) const
{
    if (index >= _data.size())
        return 0.f;
    return _data[index];
}

auto audio_through(void* outputBuffer, void* /* inputBuffer */, unsigned int nBufferFrames, double /* streamTime */, RtAudioStreamStatus status, void* userData) -> int
{
    float*  buffer = (float*)outputBuffer;
    Player* rtawp  = (Player*)userData;
    // if (status)
    // std::cout << "Stream underflow detected!" << std::endl;  // TODO store the info, and make a getter for it

    // std::vector<float> test; // TODO rename
    auto const nb_desired_channels = rtawp->parameters.nChannels;
    auto const nb_actual_channels  = rtawp->channels_count_in_data; // TODO regarder et utiliser les bonnes valeurs au bon endroit
    for (size_t i = 0; i < nBufferFrames; i++)
    {
        for (size_t channel = 0; channel < nb_desired_channels; ++channel)
        {
            auto const index_in_buffer = i * nb_desired_channels + channel;
            buffer[index_in_buffer]    = rtawp->get_data_at(rtawp->cursor + index_in_buffer);
        }
    }

    // std::memcpy(buffer, test.data(), nBufferFrames * sizeof(float) * nb_desired_channels);

    rtawp->cursor += nBufferFrames * nb_desired_channels;
    if (rtawp->cursor > rtawp->_data.size())
        rtawp->cursor = 0; // Loop from 0

    return 0;
}

} // namespace RtAudioW
