#include "Wrapper.h"
#include <rtaudio/RtAudio.h>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

namespace RtAudioW {

Player::Player(int output_channels, int _play_rate) :
	_play_rate(_play_rate),
	_output_channels_number(output_channels)
	{

	assert(is_API_available());
	assert(is_device_available());

	_parameters.deviceId=_audio.getDefaultOutputDevice();
	_parameters.firstChannel=0;
	_parameters.nChannels=output_channels;

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

	_data = std::move(data);
	_duration = (float)_data.size()/data_channels/sample_rate;

	out                    = &_parameters;
	in                     = nullptr;
	_data_channels_number  = data_channels;
	_play_rate             = sample_rate;

	return _audio.openStream(out, in, RTAUDIO_FLOAT32, _play_rate, &_buffer_frames, callback, this);
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

auto Player::seek(float time_in_seconds) -> unsigned int
{
	_cursor = static_cast<unsigned int>(_play_rate * _data_channels_number * time_in_seconds);
	return _cursor;
}

auto Player::get_data_at(size_t index) const
{
	if (index >= _data.size())
		return 0.f;
	return _data[index];
}

auto Player::get_data_length() const -> unsigned int {
	return _data.size();
}

auto Player::get_cursor() const -> unsigned int {
	return _cursor;
}

auto Player::set_cursor(unsigned int position) -> unsigned int {
	_cursor = position;
	return _cursor;
}

auto Player::get_data_channels() const -> unsigned int {
	return _data_channels_number;
}
auto Player::get_output_channels() const -> unsigned int {
	return  _output_channels_number;
}


auto audio_through(void* outputBuffer, void* /* inputBuffer */, unsigned int nBufferFrames, double /* streamTime */, RtAudioStreamStatus status, void* userData) -> int
{
	auto* buffer = (float*)outputBuffer;
	auto* rtawp  = (Player*)userData;

	auto const output_channels = rtawp->get_output_channels();
	auto const data_channels  = rtawp->get_data_channels(); // TODO regarder et utiliser les bonnes valeurs au bon endroit
	for (size_t i = 0; i < nBufferFrames; i++){
		for (size_t channel = 0; channel < output_channels; ++channel){
			auto const index_in_buffer = i * output_channels + channel;
			buffer[index_in_buffer]    = rtawp->get_data_at(rtawp->get_cursor() + index_in_buffer);
		}
	}

	rtawp->set_cursor(rtawp->get_cursor() + nBufferFrames * output_channels);
	if (rtawp->get_cursor() > rtawp->get_data_length())
		rtawp->set_cursor(0); // Loop from 0

	return 0;
}

} // namespace RtAudioW
