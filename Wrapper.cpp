#include <iostream>
#include <rtaudio/RtAudio.h>
#include <cstdlib>
#include <cmath>
#include <memory>

#include "Wrapper.h"

RtAudioW::RtAudioW(int channels, int samplerate){
	if (is_API_available() && is_device_available()){
		parameters.deviceId = audio.getDefaultOutputDevice();
		parameters.nChannels = channels;
		parameters.firstChannel = 0;
		sampleRate = samplerate;
		bufferFrames = 256;
		cursor = 0;
		duration = 0;
		data = nullptr;
	}
}

auto RtAudioW::is_API_available() -> int {

	// Get the list of device IDs
	std::vector<RtAudio::Api> apis;
	RtAudio::getCompiledApi(apis);
	if (apis[0] == RtAudio::Api::RTAUDIO_DUMMY ){
		std::cerr << "No api found\n";
		return 0;
	}
	return 1;

}

auto RtAudioW::is_device_available() -> unsigned int {

	RtAudio audio;

	std::vector< unsigned int > ids = audio.getDeviceIds();
	std::cerr << "Devices found : " << ids.size() << "\n";

	return ids.size();

}

auto RtAudioW::open(RtAudioCallback callback, void* data, bool output) -> void{
	RtAudioErrorType err;
	RtAudio::StreamParameters * out;
	RtAudio::StreamParameters * in;

	if (output) {
		out = &parameters;
		in = nullptr;
	} else {
		out = nullptr;
		in = &parameters;
	}

	err = audio.openStream(out,
							in,
							RTAUDIO_FLOAT64,
							sampleRate,
							&bufferFrames,
							callback,
							data);

}

auto RtAudioW::play() -> RtAudioErrorType {
	audio.startStream();
}

auto RtAudioW::stop() -> RtAudioErrorType {
	audio.stopStream();
}

auto RtAudioW::seek(double time) -> void {
	double timeRatio = time/duration;
	cursor = (unsigned int)(sampleRate*parameters.nChannels*(timeRatio - (int)(timeRatio)));
}

auto RtAudioW::isOpen() -> bool {
	return audio.isStreamOpen();
}

auto RtAudioW::close() -> RtAudioErrorType {
	audio.closeStream();
}

