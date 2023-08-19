#include <cstring>
#include <iostream>
#include <rtaudio/RtAudio.h>
#include <cstdlib>
#include <cmath>
#include <memory>

#include "Wrapper.h"

namespace RtAudioW {

Player::Player(int channels, int samplerate){
	if (is_API_available() && is_device_available()){
		parameters.deviceId = audio.getDefaultOutputDevice();
		parameters.nChannels = channels;
		parameters.firstChannel = 0;
		sampleRate = samplerate;
		bufferFrames = 256;
		dataLength = 0;
		cursor = 0;
		duration = 0;
		data = nullptr;
	}
}

auto Player::is_API_available() -> int {

	// Get the list of device IDs
	std::vector<RtAudio::Api> apis;
	RtAudio::getCompiledApi(apis);
	if (apis[0] == RtAudio::Api::RTAUDIO_DUMMY ){
		std::cerr << "No api found\n";
		return 0;
	}
	return 1;

}

auto Player::is_device_available() -> unsigned int {

	RtAudio audio;

	std::vector< unsigned int > ids = audio.getDeviceIds();
	std::cerr << "Devices found : " << ids.size() << "\n";

	return ids.size();

}

auto Player::open(std::vector<float> & data, RtAudioCallback callback, bool output) -> void{
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

	this->dataLength = data.size();
	this->data = data.data();

	err = audio.openStream(out,
							in,
							RTAUDIO_FLOAT32,
							sampleRate,
							&bufferFrames,
							callback,
							this);

}

auto Player::play() -> RtAudioErrorType {
	audio.startStream();
}

auto Player::stop() -> RtAudioErrorType {
	audio.stopStream();
}

auto Player::seek(double time) -> unsigned int {
	cursor = (unsigned int)(sampleRate*parameters.nChannels*time)%dataLength;
	return cursor;
}

auto Player::isOpen() -> bool {
	return audio.isStreamOpen();
}

auto Player::close() -> RtAudioErrorType {
	audio.closeStream();
}

auto audio_through(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData ) -> int {

	float *buffer = (float *) outputBuffer;
	Player *rtawp = (Player *) userData;
	if ( status )
		std::cout << "Stream underflow detected!" << std::endl;

	std::vector<float> test;
	for (size_t i = 0; i<nBufferFrames; i++){
		if (rtawp->cursor+i*2+1 < rtawp->dataLength) {
			test.push_back(rtawp->data[rtawp->cursor+i*2]);
			test.push_back(rtawp->data[rtawp->cursor+i*2+1]);
		} else {
			test.push_back(0);
			test.push_back(0);
			rtawp->cursor = 0; // Loop from 0
		}
	}

	std::memcpy(buffer, test.data(), nBufferFrames*sizeof(float)*2);

	rtawp->cursor += nBufferFrames*2;
	return 0;
}

}
