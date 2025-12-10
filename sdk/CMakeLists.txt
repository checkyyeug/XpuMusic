# SDK headers library (header-only)
add_library(sdk_headers INTERFACE)

target_include_directories(sdk_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

# Install SDK headers
install(DIRECTORY headers/
    DESTINATION include/music-player
    FILES_MATCHING PATTERN "*.h"
)
