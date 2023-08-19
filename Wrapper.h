#include <iostream>
#include <rtaudio/RtAudio.h>
#include <cstdlib>
#include <cmath>
#include <memory>

namespace RtAudioW {

auto audio_through(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData ) -> int ;

class Player {

	private :
	public:
		RtAudio audio;
		RtAudio::StreamParameters parameters;
		unsigned int sampleRate;
		unsigned int bufferFrames; // 256 sample frames
		unsigned int cursor;
		unsigned int dataLength;
		double duration;
		float* data;


		explicit Player(int channels = 2, int samplerate = 44100);

		auto is_API_available() -> int ;

		auto is_device_available() -> unsigned int ;

		auto open(std::vector<float> & data, RtAudioCallback callback = &audio_through, bool output=true) -> void;

		auto isOpen() -> bool ;

		auto play() -> RtAudioErrorType ;

		auto stop() -> RtAudioErrorType ;

		auto seek(double time) -> unsigned int ;

		auto close() -> RtAudioErrorType ;

};

}