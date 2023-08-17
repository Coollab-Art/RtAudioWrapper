#include <iostream>
#include <rtaudio/RtAudio.h>
#include <cstdlib>
#include <cmath>
#include <memory>

class RtAudioW {

	private :
		RtAudio audio;
		RtAudio::StreamParameters parameters;
		unsigned int sampleRate;
		unsigned int bufferFrames; // 256 sample frames
		unsigned int cursor;
		double duration;
		void* data;

	public:

		RtAudioW(int channels = 2, int samplerate = 44100);

		auto is_API_available() -> int ;

		auto is_device_available() -> unsigned int ;

		auto open(RtAudioCallback callback, void* data=nullptr, bool output=true) -> void;

		auto isOpen() -> bool ;

		auto play() -> RtAudioErrorType ;

		auto stop() -> RtAudioErrorType ;

		auto seek(double time) -> void ;

		auto close() -> RtAudioErrorType ;

};