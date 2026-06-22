cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(LumaScope VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LUMASCOPE_WEBVIEW_DEV_SERVER "" CACHE STRING "Debug-only Vite server (empty or http://127.0.0.1:5174)")
option(LUMASCOPE_RELEASE_BUILD "Configure an official release build" OFF)
if (NOT LUMASCOPE_WEBVIEW_DEV_SERVER STREQUAL ""
    AND NOT LUMASCOPE_WEBVIEW_DEV_SERVER STREQUAL "http://127.0.0.1:5174")
  message(FATAL_ERROR "LUMASCOPE_WEBVIEW_DEV_SERVER must be empty or exactly http://127.0.0.1:5174")
endif()
if (NOT LUMASCOPE_WEBVIEW_DEV_SERVER STREQUAL ""
    AND (LUMASCOPE_RELEASE_BUILD OR CMAKE_BUILD_TYPE STREQUAL "Release"))
  message(FATAL_ERROR "LUMASCOPE_WEBVIEW_DEV_SERVER is forbidden in Release builds")
endif()

include(CTest)
include(cmake/Dependencies.cmake)

add_subdirectory(plugin)
add_subdirectory(tests/native)
