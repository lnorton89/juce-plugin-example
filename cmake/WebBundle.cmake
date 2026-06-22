find_program(NPM_EXECUTABLE NAMES npm.cmd npm REQUIRED)

set(LUMASCOPE_UI_DIR "${CMAKE_SOURCE_DIR}/ui")
set(LUMASCOPE_WEB_BUILD_DIR "${LUMASCOPE_UI_DIR}/dist")
set(LUMASCOPE_WEB_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/generated/lumascope-web.zip")
set(LUMASCOPE_NPM_STAMP "${CMAKE_CURRENT_BINARY_DIR}/generated/npm-ci.stamp")
file(GLOB_RECURSE LUMASCOPE_UI_INPUTS CONFIGURE_DEPENDS
  "${LUMASCOPE_UI_DIR}/src/*"
  "${LUMASCOPE_UI_DIR}/index.html"
  "${LUMASCOPE_UI_DIR}/vite.config.ts"
  "${LUMASCOPE_UI_DIR}/tsconfig.json")

add_custom_command(
  OUTPUT "${LUMASCOPE_NPM_STAMP}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
  COMMAND "${NPM_EXECUTABLE}" ci
  COMMAND "${CMAKE_COMMAND}" -E touch "${LUMASCOPE_NPM_STAMP}"
  DEPENDS "${LUMASCOPE_UI_DIR}/package.json" "${LUMASCOPE_UI_DIR}/package-lock.json"
  WORKING_DIRECTORY "${LUMASCOPE_UI_DIR}"
  COMMENT "Installing pinned LumaScope UI dependencies")

add_custom_command(
  OUTPUT "${LUMASCOPE_WEB_ARCHIVE}"
  COMMAND "${NPM_EXECUTABLE}" run build
  COMMAND powershell.exe -NoProfile -File "${CMAKE_SOURCE_DIR}/scripts/New-DeterministicZip.ps1"
          -SourceDirectory "${LUMASCOPE_WEB_BUILD_DIR}" -OutputPath "${LUMASCOPE_WEB_ARCHIVE}"
  DEPENDS "${LUMASCOPE_NPM_STAMP}" ${LUMASCOPE_UI_INPUTS}
  WORKING_DIRECTORY "${LUMASCOPE_UI_DIR}"
  COMMENT "Building and archiving the LumaScope UI")

add_custom_target(LumaScopeWebArchive DEPENDS "${LUMASCOPE_WEB_ARCHIVE}")
set_source_files_properties("${LUMASCOPE_WEB_ARCHIVE}" PROPERTIES GENERATED TRUE)
juce_add_binary_data(LumaScopeWebBundle
  HEADER_NAME LumaScopeWebBundle.h
  NAMESPACE LumaScopeWebBundleData
  SOURCES "${LUMASCOPE_WEB_ARCHIVE}")
add_dependencies(LumaScopeWebBundle LumaScopeWebArchive)
