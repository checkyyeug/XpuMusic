# Core engine library
add_library(core_engine STATIC
    core_engine.cpp
    service_registry.cpp
    event_bus.cpp
    plugin_host.cpp
    config_manager.cpp
    playback_engine.cpp
    playlist_manager.cpp
    visualization_engine.cpp
)

target_include_directories(core_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/sdk/headers
)

target_link_libraries(core_engine
    PUBLIC
        sdk_headers
        ${CMAKE_DL_LIBS}
        Threads::Threads
)

# Set compile options (platform-specific)
if(MSVC)
    target_compile_options(core_engine PRIVATE
        /W4
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Release>:/O2>
    )
else()
    target_compile_options(core_engine PRIVATE
        -Wall -Wextra
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
    )
endif()
