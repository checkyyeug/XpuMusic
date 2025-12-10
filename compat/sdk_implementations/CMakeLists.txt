# 用于测试 SDK 实现的简单 CMakeLists
# 可以直接使用 MinGW g++ 编译

cmake_minimum_required(VERSION 3.10)
project(SDKImplTest CXX)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含 foobar_sdk 头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../foobar_sdk)

# 创建小的 SDK 实现库
add_library(sdk_impl STATIC
    service_base.cpp
    abort_callback.impl.cpp
    file_info_impl.cpp
)

# 测试可执行文件
add_executable(test_sdk test_sdk_impl.cpp)
target_link_libraries(test_sdk sdk_impl)

# 复制可执行文件到方便的位置
add_custom_command(TARGET test_sdk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
        $<TARGET_FILE:test_sdk>
        ${CMAKE_CURRENT_SOURCE_DIR}/test_sdk.exe
)
