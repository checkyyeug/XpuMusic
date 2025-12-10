# Platform abstraction library
# Add platform-specific audio output sources
if(WIN32)
    set(PLATFORM_AUDIO_SOURCES windows/audio_output_wasapi.cpp)
    message(STATUS "Building Windows audio output with WASAPI")
elseif(APPLE)
    set(PLATFORM_AUDIO_SOURCES macos/audio_output_coreaudio.cpp)
    message(STATUS "Building macOS audio output with CoreAudio")
elseif(UNIX AND NOT APPLE)
    # Linux audio support disabled for now - would need proper ALSA setup
    set(PLATFORM_AUDIO_SOURCES "")
    message(STATUS "Linux audio support disabled - would need ALSA development libraries")
else()
    # Default case - no audio sources
    set(PLATFORM_AUDIO_SOURCES "")
    message(STATUS "Unknown platform - no audio output available")
endif()

add_library(platform_abstraction STATIC
    ${PLATFORM_AUDIO_SOURCES}
    audio_output_factory.cpp
)

# Platform-specific dependencies
if(UNIX AND NOT APPLE)
    # Find ALSA library on Linux
    find_package(ALSA)
    if(NOT ALSA_FOUND)
        message(WARNING "ALSA not found, audio output will be stub only")
        target_compile_definitions(platform_abstraction PRIVATE NO_ALSA)
    endif()
endif()

target_include_directories(platform_abstraction
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/sdk/headers
)

# Link platform-specific libraries
if(WIN32)
    target_link_libraries(platform_abstraction
        PUBLIC
            sdk_headers
        PRIVATE
            ole32
            winmm
    )
elseif(APPLE)
    target_link_libraries(platform_abstraction
        PUBLIC
            sdk_headers
        PRIVATE
            "-framework AudioUnit"
            "-framework CoreAudio"
            "-framework CoreFoundation"
    )
elseif(UNIX)
    if(ALSA_FOUND)
        target_link_libraries(platform_abstraction
            PUBLIC
                sdk_headers
                ${ALSA_LIBRARIES}
        )
        target_include_directories(platform_abstraction PRIVATE ${ALSA_INCLUDE_DIRS})
    else()
        target_link_libraries(platform_abstraction
            PUBLIC
                sdk_headers
        )
    endif()
endif()

# Set compile options (platform-specific)
if(MSVC)
    target_compile_options(platform_abstraction PRIVATE
        /W4
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Release>:/O2>
    )
else()
    target_compile_options(platform_abstraction PRIVATE
        -Wall -Wextra
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
    )
endif()
