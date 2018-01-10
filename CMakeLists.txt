cmake_minimum_required(VERSION 3.8)
project(WechatJump)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
find_package(OpenCV REQUIRED)
add_executable(WechatJump ${SOURCE_FILES})
target_link_libraries(WechatJump ${OpenCV_LIBS})