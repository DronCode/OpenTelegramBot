cmake_minimum_required(VERSION 3.15)
project(ICVReactor)

find_package(CURL REQUIRED)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE REACTOR_SOURCES project/*.cpp project/*.cc)

include_directories(
        ${CURL_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/project
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/json/single_include
        )

add_subdirectory(modules/fmt)
add_subdirectory(modules/spdlog)

add_executable(ICVReactor ${REACTOR_SOURCES})
target_link_libraries(ICVReactor pthread ${CURL_LIBRARIES} fmt::fmt spdlog::spdlog pthread)