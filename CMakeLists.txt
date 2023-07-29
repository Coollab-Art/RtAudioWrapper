cmake_minimum_required(VERSION 3.8)

set(WARNINGS_AS_ERRORS_FOR_RTAUDIOWRAPPER OFF CACHE BOOL "ON iff you want to treat warnings as errors")

add_library(RtAudioWrapper INTERFACE)
add_library(RtAudioWrapper::RtAudioWrapper ALIAS RtAudioWrapper)
target_compile_features(RtAudioWrapper INTERFACE cxx_std_11)
if(WARNINGS_AS_ERRORS_FOR_RTAUDIOWRAPPER)
    target_include_directories(RtAudioWrapper INTERFACE include)
else()
    target_include_directories(RtAudioWrapper SYSTEM INTERFACE include)
endif()
