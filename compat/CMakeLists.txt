# foobar2000 Compatibility Layer CMakeLists.txt
# Architecture: Adapter pattern for foobar2000 plugin compatibility

# ============================================================================
# Core Compatibility Manager
# ============================================================================

add_library(foobar_compat STATIC
    foobar_compat_manager.cpp
    foobar_compat_manager.h
    logging.h
)

target_include_directories(foobar_compat PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/foobar_sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk_implementations
    ${CMAKE_SOURCE_DIR}/sdk/headers
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(foobar_compat PUBLIC
    core_engine
    Threads::Threads
)

# ============================================================================
# SDK Implementations (foobAR2000 API translation)
# ============================================================================

add_library(sdk_impl STATIC
    sdk_implementations/service_base.cpp
    sdk_implementations/service_base.h
    sdk_implementations/abort_callback.impl.cpp
    sdk_implementations/abort_callback.implified.h
    sdk_implementations/file_info_impl.cpp
    sdk_implementations/file_info_impl.h
    sdk_implementations/audio_chunk_impl.cpp
    sdk_implementations/audio_chunk_impl.h
    sdk_implementations/metadb_handle_impl.cpp
    sdk_implementations/metadb_handle_impl.h
)

target_include_directories(sdk_impl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/foobar_sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk_implementations
    ${CMAKE_SOURCE_DIR}/sdk/headers
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(sdk_impl PUBLIC
    foobar_compat
    Threads::Threads
)

# ============================================================================
# Plugin Loader (Dynamic foobar2000 plugin loading)
# ============================================================================

if(WIN32 OR UNIX)
    add_library(foobar_plugin_loader STATIC
        plugin_loader/plugin_loader.cpp
        plugin_loader/plugin_loader.h
        plugin_loader/service_registry_bridge.h
    )

    target_include_directories(foobar_plugin_loader PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/foobar_sdk
        ${CMAKE_CURRENT_SOURCE_DIR}/sdk_implementations
        ${CMAKE_SOURCE_DIR}/core
    )

    target_link_libraries(foobar_plugin_loader PUBLIC
        foobar_compat
        sdk_impl
        Threads::Threads
    )

    # Platform-specific plugin loading
    if(WIN32)
        target_link_libraries(foobar_plugin_loader PRIVATE
            kernel32
            user32
        )
    endif()

    target_link_libraries(foobar_compat PRIVATE foobar_plugin_loader)
endif()

# ============================================================================
# Data Migration Tools
# ============================================================================

add_library(data_migration STATIC
    migration/data_migration_manager.cpp
    migration/data_migration_manager.h
)

target_include_directories(data_migration PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/sdk/headers
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(data_migration PUBLIC
    foobar_compat
    Threads::Threads
)

target_link_libraries(foobar_compat PRIVATE data_migration)

# ============================================================================
# Adapter Layer (Stub - headers only for now)
# ============================================================================

# Note: Only headers exist currently. Implementation files will be added when adapters are ready
# add_library(foobar_adapters STATIC
#     adapters/input_decoder_adapter.cpp
#     adapters/input_decoder_adapter.h
# )

# ============================================================================
# Compatibility Tests
# ============================================================================

option(BUILD_COMPAT_TESTS "Build foobar2000 compatibility tests" OFF)

if(BUILD_COMPAT_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================================
# Build Summary
# ============================================================================

message(STATUS "foobar2000 Compatibility Layer Configuration:")
if(ENABLE_FOOBAR_COMPAT)
    message(STATUS "  ✓ Compatibility layer enabled")
    message(STATUS "  ✓ SDK implementations built")
    message(STATUS "  ✓ Plugin loader ready")
    message(STATUS "  ✓ Data migration tools available")
else()
    message(STATUS "  ⚠ Compatibility layer disabled")
endif()

