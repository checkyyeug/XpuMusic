# Compatibility Layer Tests CMakeLists.txt

# Test executable
add_executable(compat_tests
    test_main.cpp
)

target_include_directories(compat_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/compat
    ${CMAKE_SOURCE_DIR}/compat/tests
    ${CMAKE_SOURCE_DIR}/sdk/headers
)

target_link_libraries(compat_tests PRIVATE
    foobar_compat
    sdk_headers
)

# Add test
add_test(NAME compat_tests COMMAND compat_tests)
