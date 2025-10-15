## Copyright 2025 Jefferson Amstutz
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(agx LANGUAGES CXX)

## Dependencies ##

find_package(anari REQUIRED)

## Main library ##

add_library(agx INTERFACE)
target_include_directories(agx INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(agx INTERFACE anari::anari)

## Header info tool ##

add_executable(agx_info agx_info.cpp)
target_link_libraries(agx_info PRIVATE agx)

## Example apps ##

add_executable(agx_read_example agx_read_example.cpp)
target_link_libraries(agx_read_example PRIVATE agx)

add_executable(agx_write_example agx_write_example.cpp)
target_link_libraries(agx_write_example PRIVATE agx)
