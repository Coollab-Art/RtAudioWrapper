#include "RtAudioWrapper/RtAudioWrapper.hpp"

auto main() -> int
{
    // bool const is_ci = argc > 1 && strcmp(argv[1], "-CI") == 0; // NOLINT(*pointer-arithmetic)

    RtAudioW::Player player{}; // This will assert if no API is available, which is something we want to detect.
    return 0;
}