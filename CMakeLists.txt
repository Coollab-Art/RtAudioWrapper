cmake_minimum_required(VERSION 3.8)

set(WARNINGS_AS_ERRORS_FOR_RTAUDIOWRAPPER OFF CACHE BOOL "ON iff you want to treat warnings as errors")

add_library(RtAudioWrapper)
add_library(RtAudioWrapper::RtAudioWrapper ALIAS RtAudioWrapper)
target_compile_features(RtAudioWrapper PRIVATE cxx_std_11)

# ---Add source files---
if(WARNINGS_AS_ERRORS_FOR_RTAUDIOWRAPPER)
    target_include_directories(RtAudioWrapper PUBLIC include)
else()
    target_include_directories(RtAudioWrapper SYSTEM PUBLIC include)
endif()

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*)
target_sources(RtAudioWrapper PRIVATE ${SOURCE_FILES})

# Set warning level
if(MSVC)
    target_compile_options(RtAudioWrapper PRIVATE /W4)
else()
    target_compile_options(RtAudioWrapper PRIVATE -Wall -Wextra -Wpedantic -pedantic-errors -Wconversion -Wsign-conversion)
endif()

# Maybe enable warnings as errors
if(WARNINGS_AS_ERRORS_FOR_RTAUDIOWRAPPER)
    if(MSVC)
        target_compile_options(RtAudioWrapper PRIVATE /WX)
    else()
        target_compile_options(RtAudioWrapper PRIVATE -Werror)
    endif()
endif()

# Add RtAudioLibrary
add_library(rtaudio)
target_sources(rtaudio PRIVATE lib/rtaudio/RtAudio.cpp)
target_include_directories(rtaudio PUBLIC lib)
target_link_libraries(RtAudioWrapper PUBLIC rtaudio)

# Check for Jack (any OS)
find_library(JACK_LIB jack)
find_package(PkgConfig)
pkg_check_modules(jack jack)

if(JACK_LIB OR jack_FOUND)
    set(HAVE_JACK TRUE)
endif()

# Check for Pulse (any OS)
pkg_check_modules(pulse libpulse-simple)

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
    # TODO Which API do we use ? All ?
    set(RTAUDIO_API_ALSA ON)
    set(RTAUDIO_API_PULSE ON)
    set(RTAUDIO_API_JACK ON)
endif()

# Tweak API-specific configuration.

# Jack
if(RTAUDIO_API_JACK AND jack_FOUND)
    set(NEED_PTHREAD ON)
    list(APPEND PKGCONFIG_REQUIRES "jack")
    list(APPEND API_DEFS "-D__UNIX_JACK__ ")
    list(APPEND API_LIST "jack")

    if(jack_FOUND)
        list(APPEND LINKLIBS ${jack_LIBRARIES})
        list(APPEND INCDIRS ${jack_INCLUDEDIR})
    else()
        list(APPEND LINKLIBS ${JACK_LIB})
    endif()
endif()

# ALSA
if(${RTAUDIO_API_ALSA})
    set(NEED_PTHREAD ON)
    find_package(ALSA)

    if(NOT ALSA_FOUND)
        message(FATAL_ERROR "ALSA API requested but no ALSA dev libraries found")
    endif()

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

        # Note: not an error on some systems
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

    # list(APPEND LINKLIBS ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})
    target_link_libraries(rtaudio PRIVATE ${COREAUDIO_LIB} ${COREFOUNDATION_LIB})

    # list(APPEND LIBS_REQUIRES "-framework CoreAudio -framework CoreFoundation")
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

foreach(X IN LISTS API_LIST API_DEFS)
    message(${X})
endforeach()

target_link_libraries(RtAudioWrapper PRIVATE ${API_LIST})

foreach(API_DEF IN LISTS API_DEFS)
    set_source_files_properties(lib/rtaudio/RtAudio.cpp PROPERTIES COMPILE_FLAGS ${API_DEF})
endforeach()