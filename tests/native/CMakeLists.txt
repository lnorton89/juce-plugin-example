set(WEB_FIXTURE_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/web-fixture.zip")
file(GLOB WEB_FIXTURE_FILES RELATIVE "${CMAKE_SOURCE_DIR}/tests/fixtures/web" "${CMAKE_SOURCE_DIR}/tests/fixtures/web/*")
file(ARCHIVE_CREATE OUTPUT "${WEB_FIXTURE_ARCHIVE}" PATHS ${WEB_FIXTURE_FILES} FORMAT zip MTIME "2020-01-01" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/fixtures/web")
juce_add_binary_data(TestWebResources HEADER_NAME TestWebResources.h NAMESPACE TestWebResourcesData SOURCES "${WEB_FIXTURE_ARCHIVE}")

add_executable(LumaScopeNativeTests HostBridgeTests.cpp SpectrumAnalyzerTests.cpp WebResourcesTests.cpp
  "${CMAKE_SOURCE_DIR}/plugin/source/HostBridge.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/WebResources.cpp")
target_include_directories(LumaScopeNativeTests PRIVATE "${CMAKE_SOURCE_DIR}/plugin/include")
target_compile_features(LumaScopeNativeTests PRIVATE cxx_std_20)
target_compile_definitions(LumaScopeNativeTests PRIVATE
  LUMASCOPE_FIXTURE_UI_READY="${CMAKE_SOURCE_DIR}/tests/fixtures/bridge/ui-ready-v1.json"
  LUMASCOPE_FIXTURE_HOST_INFO="${CMAKE_SOURCE_DIR}/tests/fixtures/bridge/host-info-v1.json"
  LUMASCOPE_FIXTURE_UNSUPPORTED="${CMAKE_SOURCE_DIR}/tests/fixtures/bridge/unsupported-version.json"
  LUMASCOPE_FIXTURE_MALFORMED="${CMAKE_SOURCE_DIR}/tests/fixtures/bridge/malformed.json")
target_link_libraries(LumaScopeNativeTests PRIVATE TestWebResources juce::juce_gui_extra)
add_test(NAME LumaScopeNativeTests COMMAND LumaScopeNativeTests)
