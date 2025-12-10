# AudioBackend.cmake - Cross-platform audio backend detection and selection
# This module automatically detects available audio backends and selects the best one

# Initialize variables
set(AUDIO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/audio_output_stub.cpp")
set(AUDIO_LIBS "")
set(AUDIO_INCLUDE_DIRS "")
set(AUDIO_BACKEND_NAME "stub")

message(STATUS "Audio Backend Detection:")

#----------------------------------------------------------
# Helper function to add backend
#----------------------------------------------------------
macro(add_audio_backend NAME SOURCE_FILE LIBS_VAR INCLUDE_VAR)
    list(APPEND AUDIO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/${SOURCE_FILE}")
    if(${LIBS_VAR})
        list(APPEND AUDIO_LIBS ${${LIBS_VAR}})
    endif()
    if(${INCLUDE_VAR})
        list(APPEND AUDIO_INCLUDE_DIRS ${${INCLUDE_VAR}})
    endif()
    set(AUDIO_BACKEND_NAME "${NAME}")
    message(STATUS "  → ${NAME} audio output")
endmacro()

#----------------------------------------------------------
# Windows: 1. WASAPI (always available), 2. WinMM
#----------------------------------------------------------
if(WIN32)
    # WASAPI is always available on Windows Vista and later
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/audio_output_wasapi.cpp")
        add_audio_backend("WASAPI" "audio_output_wasapi.cpp" "" "")
        list(APPEND AUDIO_LIBS "ole32")
    else()
        message(STATUS "  → WASAPI source not found, using stub")
    endif()

#----------------------------------------------------------
# macOS: 1. CoreAudio, 2. AudioToolbox
#----------------------------------------------------------
elseif(APPLE)
    # Find CoreAudio framework
    find_library(COREAUDIO_LIB CoreAudio REQUIRED)
    find_library(AUDIOTOOLBOX_LIB AudioToolbox REQUIRED)
    find_library(COREFOUNDATION_LIB CoreFoundation REQUIRED)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/audio_output_coreaudio.cpp")
        add_audio_backend("CoreAudio" "audio_output_coreaudio.cpp" "COREAUDIO_LIB;AUDIOTOOLBOX_LIB;COREFOUNDATION_LIB" "")
    else()
        message(STATUS "  → CoreAudio source not found, using stub")
    endif()

#----------------------------------------------------------
# Linux: 1. ALSA, 2. PulseAudio, 3. JACK, 4. Stub
#----------------------------------------------------------
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "GNU")
    # Initialize pkg-config
    find_package(PkgConfig QUIET)

    # 1. Try ALSA
    if(PkgConfig_FOUND)
        pkg_check_modules(ALSA QUIET alsa)
        if(ALSA_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/audio_output_alsa.cpp")
            set(ALSA_LIBS_VAR "ALSA_LIBRARIES")
            set(ALSA_INCLUDE_VAR "ALSA_INCLUDE_DIRS")
            add_audio_backend("ALSA" "audio_output_alsa.cpp" "ALSA_LIBS_VAR" "ALSA_INCLUDE_VAR")
        endif()
    endif()

    # 2. Try PulseAudio if ALSA not found
    if(NOT AUDIO_BACKEND_NAME STREQUAL "ALSA" AND PKG_CONFIG_FOUND)
        pkg_check_modules(PULSE QUIET libpulse-simple)
        if(PULSE_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/audio_output_pulse.cpp")
            set(PULSE_LIBS_VAR "PULSE_LIBRARIES")
            set(PULSE_INCLUDE_VAR "PULSE_INCLUDE_DIRS")
            add_audio_backend("PulseAudio" "audio_output_pulse.cpp" "PULSE_LIBS_VAR" "PULSE_INCLUDE_VAR")
        endif()
    endif()

    # 3. Try JACK if ALSA and PulseAudio not found
    if(NOT AUDIO_BACKEND_NAME STREQUAL "ALSA" AND NOT AUDIO_BACKEND_NAME STREQUAL "PulseAudio" AND PKG_CONFIG_FOUND)
        pkg_check_modules(JACK QUIET jack)
        if(JACK_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/audio_output_jack.cpp")
            set(JACK_LIBS_VAR "JACK_LIBRARIES")
            set(JACK_INCLUDE_VAR "JACK_INCLUDE_DIRS")
            add_audio_backend("JACK" "audio_output_jack.cpp" "JACK_LIBS_VAR" "JACK_INCLUDE_VAR")
        endif()
    endif()

    # 4. If nothing found, use stub
    if(AUDIO_BACKEND_NAME STREQUAL "stub")
        message(STATUS "  → No Linux audio backend found, using stub")
    endif()

#----------------------------------------------------------
# Other Unix-like systems (BSD, etc.)
#----------------------------------------------------------
elseif(UNIX)
    # Try PulseAudio first (most portable)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PULSE QUIET libpulse-simple)
        if(PULSE_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/audio_output_pulse.cpp")
            set(PULSE_LIBS_VAR "PULSE_LIBRARIES")
            set(PULSE_INCLUDE_VAR "PULSE_INCLUDE_DIRS")
            add_audio_backend("PulseAudio" "audio_output_pulse.cpp" "PULSE_LIBS_VAR" "PULSE_INCLUDE_VAR")
        endif()
    endif()

    # Fallback to stub
    if(AUDIO_BACKEND_NAME STREQUAL "stub")
        message(STATUS "  → Using stub audio backend")
    endif()

#----------------------------------------------------------
# Unknown platform
#----------------------------------------------------------
else()
    message(STATUS "  → Unknown platform, using stub audio backend")
endif()

#----------------------------------------------------------
# Compile definitions for the selected backend
#----------------------------------------------------------
if(AUDIO_BACKEND_NAME STREQUAL "WASAPI")
    add_compile_definitions(AUDIO_BACKEND_WASAPI)
elseif(AUDIO_BACKEND_NAME STREQUAL "CoreAudio")
    add_compile_definitions(AUDIO_BACKEND_COREAUDIO)
elseif(AUDIO_BACKEND_NAME STREQUAL "ALSA")
    add_compile_definitions(AUDIO_BACKEND_ALSA)
elseif(AUDIO_BACKEND_NAME STREQUAL "PulseAudio")
    add_compile_definitions(AUDIO_BACKEND_PULSE)
elseif(AUDIO_BACKEND_NAME STREQUAL "JACK")
    add_compile_definitions(AUDIO_BACKEND_JACK)
else()
    add_compile_definitions(AUDIO_BACKEND_STUB)
endif()

#----------------------------------------------------------
# Export results to parent scope
#----------------------------------------------------------
set(AUDIO_SOURCES ${AUDIO_SOURCES} PARENT_SCOPE)
set(AUDIO_LIBS ${AUDIO_LIBS} PARENT_SCOPE)
set(AUDIO_INCLUDE_DIRS ${AUDIO_INCLUDE_DIRS} PARENT_SCOPE)
set(AUDIO_BACKEND_NAME ${AUDIO_BACKEND_NAME} PARENT_SCOPE)

#----------------------------------------------------------
# Debug information
#----------------------------------------------------------
message(STATUS "Selected audio backend: ${AUDIO_BACKEND_NAME}")
message(STATUS "Audio sources: ${AUDIO_SOURCES}")
if(AUDIO_LIBS)
    message(STATUS "Audio libraries: ${AUDIO_LIBS}")
endif()
if(AUDIO_INCLUDE_DIRS)
    message(STATUS "Audio includes: ${AUDIO_INCLUDE_DIRS}")
endif()