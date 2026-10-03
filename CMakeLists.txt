cmake_minimum_required(VERSION 3.14)

project(spiration.api VERSION 0.1.0 LANGUAGES C CXX)

add_library(spiration.api INTERFACE)
target_include_directories(spiration.api INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(spiration.api INTERFACE c_std_99)

add_library(spiration::api ALIAS spiration.api)

include(GNUInstallDirs)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
