#include <iostream>
#include <rtaudio/RtAudio.h>

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
}