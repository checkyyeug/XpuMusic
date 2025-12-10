# Unit tests
find_package(GTest)

if(GTest_FOUND)
    message(STATUS "Google Test found, building unit tests")
    
    # Enable testing
    enable_testing()
    
    # Test executable for config manager
    add_executable(test_config_manager test_config_manager.cpp)
    target_link_libraries(test_config_manager PRIVATE
        core_engine
        GTest::GTest
        GTest::Main
    )
    target_include_directories(test_config_manager PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/sdk/headers
    )
    gtest_discover_tests(test_config_manager)
    
    # Test executable for event bus
    add_executable(test_event_bus test_event_bus.cpp)
    target_link_libraries(test_event_bus PRIVATE
        core_engine
        GTest::GTest
        GTest::Main
    )
    target_include_directories(test_event_bus PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/sdk/headers
    )
    gtest_discover_tests(test_event_bus)
    
    # Set output directory
    set_target_properties(test_config_manager test_event_bus
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
    )
    
else()
    message(STATUS "Google Test not found, unit tests will not be built")
    message(STATUS "Install with: sudo apt-get install libgtest-dev")
endif()
