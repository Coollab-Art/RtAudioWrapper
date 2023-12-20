cmake_minimum_required(VERSION 3.8)

add_library(RtAudioWrapper)
add_library(RtAudioWrapper::RtAudioWrapper ALIAS RtAudioWrapper)
target_compile_features(RtAudioWrapper PRIVATE cxx_std_14)

# ---Add source files---
target_sources(RtAudioWrapper PRIVATE lib/rtaudio/RtAudio.cpp)
target_include_directories(RtAudioWrapper SYSTEM PUBLIC include lib)

# ---Configure backend---
find_package(PkgConfig)

# Check for known non-Linux unix-likes
if(CMAKE_SYSTEM_NAME MATCHES "kNetBSD.*|NetBSD.*")
    set(xBSD ON)
elseif(UNIX AND NOT APPLE)
    set(LINUX ON)
endif()

# API Options
if(WIN32)
    set(RTAUDIO_API_WASAPI ON)
elseif(APPLE)
    set(RTAUDIO_API_CORE ON)
elseif(xBSD)
    set(RTAUDIO_API_OSS ON)
elseif(LINUX)
    set(RTAUDIO_API_ALSA ON)
    set(RTAUDIO_API_PULSE ON)
endif()

# Check for Pulse
if(LINUX)
    pkg_check_modules(pulse libpulse-simple)
endif()

# Tweak API-specific configuration.

# ALSA
if(${RTAUDIO_API_ALSA})
    set(NEED_PTHREAD ON)
    find_package(ALSA REQUIRED)

    list(APPEND INCDIRS ${ALSA_INCLUDE_DIR})
    list(APPEND LINKLIBS ${ALSA_LIBRARIES})
    list(APPEND PKGCONFIG_REQUIRES "alsa")
    list(APPEND API_DEFS "-D__LINUX_ALSA__ ")
    list(APPEND API_LIST "asound")
endif()

# OSS
if(${RTAUDIO_API_OSS})
    set(NEED_PTHREAD ON)
    find_library(OSSAUDIO_LIB ossaudio)

    if(OSSAUDIO_LIB)
        list(APPEND LINKLIBS ossaudio)
    endif()

    list(APPEND API_DEFS "-D__LINUX_OSS__ ")
    list(APPEND API_LIST "oss")
endif()

# Pulse
if(${RTAUDIO_API_PULSE})
    set(NEED_PTHREAD ON)
    find_library(PULSE_LIB pulse)
    find_library(PULSESIMPLE_LIB pulse-simple)
    list(APPEND LINKLIBS ${PULSE_LIB} ${PULSESIMPLE_LIB})
    list(APPEND PKGCONFIG_REQUIRES "libpulse-simple")
    list(APPEND API_DEFS "-D__LINUX_PULSE__ ")
    list(APPEND API_LIST "pulse")
    list(APPEND API_LIST "pulse-simple")
endif()

# CoreAudio
if(${RTAUDIO_API_CORE})
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(COREFOUNDATION_LIB CoreFoundation)

    target_link_libraries(RtAudioWrapper PRIVATE ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})

    list(APPEND API_DEFS "-D__MACOSX_CORE__ ")
endif()

# WASAPI
if(${RTAUDIO_API_WASAPI})
    list(APPEND LINKLIBS ksuser mfplat mfuuid wmcodecdspuuid winmm ole32)
    list(APPEND API_DEFS "-D__WINDOWS_WASAPI__ ")
endif()

# pthread
if(${NEED_PTHREAD})
    find_package(Threads REQUIRED
        CMAKE_THREAD_PREFER_PTHREAD
        THREADS_PREFER_PTHREAD_FLAG)
    list(APPEND LINKLIBS Threads::Threads)
endif()

target_link_libraries(RtAudioWrapper PRIVATE ${API_LIST})

foreach(API_DEF IN LISTS API_DEFS)
    set_source_files_properties(lib/rtaudio/RtAudio.cpp PROPERTIES COMPILE_FLAGS ${API_DEF})
endforeach()

# ---Install license---
install(FILES "lib/rtaudio/LICENSE" DESTINATION "license/rtaudio")