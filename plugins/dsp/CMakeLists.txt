# DSP Plugins

# Volume Control DSP Plugin
add_library(plugin_volume_dsp SHARED
    volume_control_dsp.cpp
)

target_include_directories(plugin_volume_dsp
    PRIVATE
        ${CMAKE_SOURCE_DIR}/sdk/headers
)

target_link_libraries(plugin_volume_dsp
    PRIVATE
        sdk_headers
)

# Set compile options (platform-specific)
if(MSVC)
    target_compile_options(plugin_volume_dsp PRIVATE /W4)
else()
    target_compile_options(plugin_volume_dsp PRIVATE
        -Wall -Wextra
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
    )
endif()

# Don't add 'lib' prefix to plugin
set_target_properties(plugin_volume_dsp PROPERTIES
    PREFIX ""
    OUTPUT_NAME "volume_dsp"
)

# 10-Band Equalizer DSP Plugin
add_library(plugin_equalizer_dsp SHARED
    equalizer_dsp.cpp
)

target_include_directories(plugin_equalizer_dsp
    PRIVATE
        ${CMAKE_SOURCE_DIR}/sdk/headers
)

target_link_libraries(plugin_equalizer_dsp
    PRIVATE
        sdk_headers
)

# Set compile options (platform-specific)
if(MSVC)
    target_compile_options(plugin_equalizer_dsp PRIVATE /W4)
else()
    target_compile_options(plugin_equalizer_dsp PRIVATE
        -Wall -Wextra
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
    )
endif()

# Don't add 'lib' prefix to plugin
set_target_properties(plugin_equalizer_dsp PROPERTIES
    PREFIX ""
    OUTPUT_NAME "equalizer_dsp"
)
