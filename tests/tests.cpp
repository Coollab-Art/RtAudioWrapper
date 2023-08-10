#include <iostream>
#include <rtaudio/RtAudio.h>
#include <cstdlib>
#include <cmath>

// Two-channel wave generator.
auto sound_generator( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
			double streamTime, RtAudioStreamStatus status, void *userData ) -> int
	{
	static float freq = 880.0;
	static double x = 0;
	unsigned int i, j;
	double *buffer = (double *) outputBuffer;
	double *lastValues = (double *) userData;
	if ( status )
		std::cout << "Stream underflow detected!" << std::endl;
	// Write interleaved audio data.
	for ( i=0; i<nBufferFrames; i++ ) {
		for ( j=0; j<2; j++ ) {
		*buffer++ = lastValues[j];
		lastValues[j] = sin(2*M_PI*freq*x);
		}
		x+=1.f/44100.f;
	}
	if (freq > 200.0) {
		x=freq*x/(freq-0.1); // "Connects" the successive sinusoids at the same value
		freq -= 0.1;
	}
	std::cout << freq << "\n";
	return 0;

}

int main(){

	RtAudio audio;


	// Get the list of device IDs
	std::vector<RtAudio::Api> apis;
	RtAudio::getCompiledApi(apis);
	if (apis[0] == RtAudio::Api::RTAUDIO_DUMMY ){
		std::cout << "No api found\n";
		return 1;
	}
	std::vector< unsigned int > ids = audio.getDeviceIds();
	if ( ids.size() == 0 ) {
		std::cout << "No devices found." << std::endl;
	} else {
		// Scan through devices for various capabilities
		RtAudio::DeviceInfo info;
		for ( unsigned int n=0; n<ids.size(); n++ ) {

			info = audio.getDeviceInfo( ids[n] );

			// Print, for example, the name and maximum number of output channels for each device
			std::cout << "device name = " << info.name << std::endl;
			std::cout << ": maximum output channels = " << info.outputChannels << std::endl;
		}
	}

	RtAudio dac;
	if ( dac.getDeviceCount() < 1 ) {
		std::cout << "\nNo audio devices found!\n";
		exit( 0 );
	}
	RtAudio::StreamParameters parameters;
	parameters.deviceId = dac.getDefaultOutputDevice();
	parameters.nChannels = 2;
	parameters.firstChannel = 0;
	unsigned int sampleRate = 44100;
	unsigned int bufferFrames = 256; // 256 sample frames
	double data[2] = {0, 0};
	RtAudioErrorType err;
	err = dac.openStream( &parameters,
						NULL,
						RTAUDIO_FLOAT64,
						sampleRate,
						&bufferFrames,
						&sound_generator,
						(void *)&data );
	std::cout << "Opening stream : " << err << "\n";
	err = dac.startStream();
	std::cout << "Starting stream : " << err << "\n";

	char input;
	std::cout << "\nPlaying ... press <enter> to quit.\n";
	std::cin.get( input );
	dac.stopStream();
	std::cout << "Stoping stream : " << err << "\n";

	if ( dac.isStreamOpen() ) dac.closeStream();
	return 0;
}