#include "RtAudioWrapper/RtAudioWrapper.hpp"

auto main() -> int
{
    // bool const is_ci = argc > 1 && strcmp(argv[1], "-CI") == 0; // NOLINT(*pointer-arithmetic)

    auto p1 = RtAudioW::Player{}; // This will assert if no API is available, which is something we want to detect.
    auto p2 = std::move(p1);      // Test that the move constructor works properly (original RtAudio lib had a bug in its move constructor)
    return 0;
}