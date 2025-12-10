# WAV Decoder Plugin
add_library(plugin_wav_decoder SHARED
    wav_decoder.cpp
)

target_include_directories(plugin_wav_decoder
    PRIVATE
        ${CMAKE_SOURCE_DIR}/sdk/headers
)

target_link_libraries(plugin_wav_decoder
    PRIVATE
        sdk_headers
)

# Set compile options (platform-specific)
if(MSVC)
    target_compile_options(plugin_wav_decoder PRIVATE /W4)
else()
    target_compile_options(plugin_wav_decoder PRIVATE
        -Wall -Wextra
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
    )
endif()

# Don't add 'lib' prefix to plugin
set_target_properties(plugin_wav_decoder PROPERTIES
    PREFIX ""
    OUTPUT_NAME "wav_decoder"
)

# FLAC Decoder Plugin
# Try to find FLAC library using multiple methods
set(FLAC_FOUND FALSE)

# Method 1: Try find_package (works on Windows with vcpkg or conan)
find_package(FLAC QUIET)
if(FLAC_FOUND)
    set(FLAC_LIBRARIES FLAC::FLAC)
    message(STATUS "FLAC library found via find_package")
else()
    # Method 2: Try pkg-config (works on Linux)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(FLAC flac)
        if(FLAC_FOUND)
            message(STATUS "FLAC library found via pkg-config")
        endif()
    endif()
endif()

# Method 3: Manual search (fallback)
if(NOT FLAC_FOUND)
    find_path(FLAC_INCLUDE_DIR FLAC/stream_decoder.h
        PATHS
            "C:/Program Files/FLAC/include"
            "C:/FLAC/include"
            "/usr/include"
            "/usr/local/include"
    )
    
    find_library(FLAC_LIBRARY
        NAMES FLAC libFLAC FLAC_static
        PATHS
            "C:/Program Files/FLAC/lib"
            "C:/FLAC/lib"
            "/usr/lib"
            "/usr/local/lib"
    )
    
    if(FLAC_INCLUDE_DIR AND FLAC_LIBRARY)
        set(FLAC_FOUND TRUE)
        set(FLAC_LIBRARIES ${FLAC_LIBRARY})
        set(FLAC_INCLUDE_DIRS ${FLAC_INCLUDE_DIR})
        message(STATUS "FLAC library found manually at ${FLAC_LIBRARY}")
    endif()
endif()

add_library(plugin_flac_decoder SHARED
    flac_decoder.cpp
)

target_include_directories(plugin_flac_decoder
    PRIVATE
        ${CMAKE_SOURCE_DIR}/sdk/headers
)

if(FLAC_FOUND)
    target_link_libraries(plugin_flac_decoder
        PRIVATE
            sdk_headers
            ${FLAC_LIBRARIES}
    )
    if(FLAC_INCLUDE_DIRS)
        target_include_directories(plugin_flac_decoder PRIVATE ${FLAC_INCLUDE_DIRS})
    endif()
    message(STATUS "Building FLAC decoder plugin with libFLAC support")
else()
    target_link_libraries(plugin_flac_decoder
        PRIVATE
            sdk_headers
    )
    target_compile_definitions(plugin_flac_decoder PRIVATE NO_FLAC)
    message(WARNING "FLAC library not found, FLAC decoder will be stub only")
    message(STATUS "To build with FLAC support:")
    message(STATUS "  - Windows: Install FLAC via vcpkg (vcpkg install flac)")
    message(STATUS "  - Linux: Install libflac-dev (apt-get install libflac-dev)")
    message(STATUS "  - macOS: Install flac via Homebrew (brew install flac)")
endif()

# Set compile options (platform-specific)
if(MSVC)
    target_compile_options(plugin_flac_decoder PRIVATE /W4)
else()
    target_compile_options(plugin_flac_decoder PRIVATE
        -Wall -Wextra
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
    )
endif()

set_target_properties(plugin_flac_decoder PROPERTIES
    PREFIX ""
    OUTPUT_NAME "flac_decoder"
)

# MP3 Decoder Plugin (using minimp3 header-only library)
add_library(plugin_mp3_decoder SHARED
    mp3_decoder.cpp
)

target_include_directories(plugin_mp3_decoder
    PRIVATE
        ${CMAKE_SOURCE_DIR}/sdk/headers
        ${CMAKE_SOURCE_DIR}/sdk/external/minimp3
)

target_link_libraries(plugin_mp3_decoder
    PRIVATE
        sdk_headers
)

# Set compile options (platform-specific)
if(MSVC)
    target_compile_options(plugin_mp3_decoder PRIVATE /W4)
else()
    target_compile_options(plugin_mp3_decoder PRIVATE
        -Wall -Wextra
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
    )
endif()

set_target_properties(plugin_mp3_decoder PROPERTIES
    PREFIX ""
    OUTPUT_NAME "mp3_decoder"
)

message(STATUS "Building MP3 decoder plugin with minimp3 (header-only library)")
