get_filename_component(_CONFIG_VALIDATION_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(_PROJECT_CONFIG_ROOT "${_CONFIG_VALIDATION_ROOT}")
include("${_CONFIG_VALIDATION_ROOT}/cmake/Config.cmake")

if(NOT PROJECT_CONFIG_NAME)
  message(FATAL_ERROR "PROJECT_CONFIG_NAME was not loaded from project-config.json")
endif()

if(NOT PROJECT_CONFIG_VERSION_STR MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
  message(FATAL_ERROR "PROJECT_CONFIG_VERSION_STR is not semver: ${PROJECT_CONFIG_VERSION_STR}")
endif()

if(NOT PROJECT_CONFIG_DEV_SERVER_URL MATCHES "^http://127\\.0\\.0\\.1:[0-9]+$")
  message(FATAL_ERROR "PROJECT_CONFIG_DEV_SERVER_URL must be a loopback development URL")
endif()

message(STATUS "ConfigValidation: ${PROJECT_CONFIG_NAME} ${PROJECT_CONFIG_VERSION_STR}")
