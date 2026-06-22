cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(LumaScope VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(cmake/Dependencies.cmake)

add_subdirectory(plugin)
add_subdirectory(tests/native)

