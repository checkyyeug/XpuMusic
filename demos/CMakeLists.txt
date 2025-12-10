# Demos CMakeLists.txt

# Multi-value metadata demo
add_executable(demo_multi_value_metadata
    multi_value_metadata_demo.cpp
)

# Include directories for the demo
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../compat/sdk_implementations
    ${CMAKE_CURRENT_SOURCE_DIR}/../compat/foobar_sdk
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/headers
)

# Link with the SDK implementation library
# Note: This assumes sdk_impl is built before this demo
if(TARGET sdk_impl)
    target_link_libraries(demo_multi_value_metadata
        sdk_impl
    )
else()
    message(STATUS "Note: demo_multi_value_metadata requires sdk_impl to be built first")
endif()

# Set C++ standard
set_property(TARGET demo_multi_value_metadata PROPERTY CXX_STANDARD 17)
set_property(TARGET demo_multi_value_metadata PROPERTY CXX_STANDARD_REQUIRED ON)

# Copy to binary directory for easy access
add_custom_command(TARGET demo_multi_value_metadata POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        $<TARGET_FILE:demo_multi_value_metadata>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/demo_multi_value_metadata${CMAKE_EXECUTABLE_SUFFIX}
)
