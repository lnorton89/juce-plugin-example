cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

include(cmake/Config.cmake)

project(${PROJECT_CONFIG_NAME} VERSION ${PROJECT_CONFIG_VERSION_STR} LANGUAGES CXX)

set(PROJECT_CONFIG_GENERATED_INCLUDE_DIR "${CMAKE_BINARY_DIR}/generated/include")
configure_file(
  "${CMAKE_SOURCE_DIR}/cmake/templates/ProjectConfig.h.in"
  "${PROJECT_CONFIG_GENERATED_INCLUDE_DIR}/LumaScope/ProjectConfig.h"
  @ONLY)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LUMASCOPE_WEBVIEW_DEV_SERVER "" CACHE STRING "Debug-only Vite server (empty or http://127.0.0.1:5174)")
set(LUMASCOPE_ACTIVATION_BASE_URL "http://127.0.0.1:8787" CACHE STRING "Activation API base URL")
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
