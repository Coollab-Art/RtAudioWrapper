#include <cmath>
#include <cstdlib>
#include <iostream>
#include "RtAudioWrapper/RtAudioWrapper.hpp"

auto sound_generator(void*, void*, unsigned int, double, RtAudioStreamStatus, void*) -> int;
void devicesAPITest();
void RtAudioWTest(bool is_ci);
void RtAudioTest();

auto main(int argc, char* argv[]) -> int
{
    bool const is_ci = argc > 1 && strcmp(argv[1], "-CI") == 0; // NOLINT(*pointer-arithmetic)

    auto p1 = RtAudioW::Player{};
    auto p2 = std::move(p1); // Test that the move constructor works properly (original RtAudio lib had a bug in its move constructor)
    // devicesAPITest();
    // RtAudioTest();
    RtAudioWTest(is_ci);
    return 0;
}

// Two-channel wave generator.
auto sound_generator(void* outputBuffer, void* /* inputBuffer */, unsigned int nBufferFrames, double /* streamTime */, RtAudioStreamStatus status, void* /* userData */) -> int
{
    static float  freq = 880.0;
    static double x    = 0;
    unsigned int  i, j;
    double*       buffer = (double*)outputBuffer;
    // double*       lastValues = (double*)userData;
    if (status)
        std::cout << "Stream underflow detected!" << std::endl;
    // Write interleaved audio data.
    for (i = 0; i < nBufferFrames; i++)
    {
        for (j = 0; j < 2; j++)
        {
            *buffer++ = 0.3 * sin(2 * 3.141592653f * freq * x);
        }
        x += 1.f / 44100.f;
    }
    if (freq > 200.0)
    {
        x = freq * x / (freq - 0.1); // "Connects" the successive sinusoids at the same value
        freq -= 0.1f;
    }
    std::cout << freq << "\n";
    return 0;
}

auto dummy_generator(void* outputBuffer, void* /* inputBuffer */, unsigned int nBufferFrames, double /* streamTime */, RtAudioStreamStatus /* status */, void* /* userData */) -> int
{
    double* buf = (double*)outputBuffer;
    for (size_t i = 0; i < nBufferFrames; i++)
    {
        for (size_t j = 0; j < 2; j++)
        {
            *buf++ = 0;
        }
    }
    return 0;
}

void devicesAPITest()
{
    RtAudio audio;
    // Get the list of device IDs
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    if (apis[0] == RtAudio::Api::RTAUDIO_DUMMY)
    {
        std::cout << "No api found\n";
    }
    std::vector<unsigned int> ids = audio.getDeviceIds();
    if (ids.size() == 0)
    {
        std::cout << "No devices found." << std::endl;
    }
    else
    {
        // Scan through devices for various capabilities
        RtAudio::DeviceInfo info;
        for (unsigned int n = 0; n < ids.size(); n++)
        {
            info = audio.getDeviceInfo(ids[n]);

            // Print, for example, the name and maximum number of output channels for each device
            std::cout << "device name = " << info.name << std::endl;
            std::cout << ": maximum output channels = " << info.outputChannels << std::endl;
        }
    }
}

void RtAudioWTest(bool is_ci)
{
    RtAudioW::Player wrap;
    RtAudioErrorType err;

    wrap.open(std::vector<float>{}, 44100, 2, &sound_generator);
    // wrap.open(&dummy_generator);

    char input   = '\n';
    int  playing = 0;
    while (input == '\n')
    {
        if (!playing)
        {
            err = wrap.play();
            std::cout << "Starting stream : " << err << "\n";
            std::cout << "\nPlaying ... press <enter> to stop.\n";
            playing = 1;
        }
        else
        {
            err = wrap.pause();
            std::cout << "Stoping stream : " << err << "\n";
            std::cout << "\nStopping ... press <enter> to play.\n";
            playing = 0;
        }
        if (is_ci)
            input = ' ';
        else
            std::cin.get(input);
    }
    if (wrap.is_open())
    {
        wrap.pause();
    }
}

void RtAudioTest()
{
    RtAudio dac;
    if (dac.getDeviceCount() < 1)
    {
        std::cout << "\nNo audio devices found!\n";
        exit(0);
    }
    RtAudio::StreamParameters parameters;
    parameters.deviceId           = dac.getDefaultOutputDevice();
    parameters.nChannels          = 2;
    parameters.firstChannel       = 0;
    unsigned int     sampleRate   = 44100;
    unsigned int     bufferFrames = 256; // 256 sample frames
    double           data[2]      = {0, 0};
    RtAudioErrorType err;
    err = dac.openStream(&parameters, NULL, RTAUDIO_FLOAT64, sampleRate, &bufferFrames, &sound_generator, (void*)&data);

    std::cout << "Opening stream : " << err << "\n";
    char input   = '\n';
    int  playing = 0;
    while (input == '\n')
    {
        if (!playing)
        {
            err = dac.startStream();
            std::cout << "Starting stream : " << err << "\n";
            std::cout << "\nPlaying ... press <enter> to stop.\n";
            playing = 1;
        }
        else
        {
            err = dac.stopStream();
            std::cout << "Stoping stream : " << err << "\n";
            std::cout << "\nStopping ... press <enter> to play.\n";
            playing = 0;
        }
        std::cin.get(input);
    }

    if (dac.isStreamOpen())
        dac.closeStream();
}
