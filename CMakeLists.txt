cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 14)

add_library(spiration.api INTERFACE)
target_include_directories(spiration.api INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
add_library(spiration::api ALIAS spiration.api)
