# CPM.cmake v0.42.3 is bootstrapped into the repository-local dependency cache.
set(CPM_DOWNLOAD_VERSION 0.42.3)
set(CPM_DOWNLOAD_LOCATION "${LUMASCOPE_DEPENDENCY_DIR}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
  file(MAKE_DIRECTORY "${LUMASCOPE_DEPENDENCY_DIR}/cpm")
  file(DOWNLOAD
    "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
    "${CPM_DOWNLOAD_LOCATION}"
    TLS_VERIFY ON
    STATUS CPM_DOWNLOAD_STATUS)
  list(GET CPM_DOWNLOAD_STATUS 0 CPM_DOWNLOAD_CODE)
  if(NOT CPM_DOWNLOAD_CODE EQUAL 0)
    file(REMOVE "${CPM_DOWNLOAD_LOCATION}")
    message(FATAL_ERROR "Unable to download CPM.cmake v${CPM_DOWNLOAD_VERSION}: ${CPM_DOWNLOAD_STATUS}")
  endif()
endif()

include("${CPM_DOWNLOAD_LOCATION}")

