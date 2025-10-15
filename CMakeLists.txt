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

add_library(agx STATIC)
target_sources(agx PRIVATE agx.cpp)
target_include_directories(agx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(agx PUBLIC anari::anari)

## Example app ##

add_executable(agx_example agx_example.cpp)
target_link_libraries(agx_example PRIVATE agx)
