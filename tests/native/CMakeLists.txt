set(WEB_FIXTURE_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/web-fixture.zip")
file(GLOB WEB_FIXTURE_FILES RELATIVE "${CMAKE_SOURCE_DIR}/tests/fixtures/web" "${CMAKE_SOURCE_DIR}/tests/fixtures/web/*")
file(ARCHIVE_CREATE OUTPUT "${WEB_FIXTURE_ARCHIVE}" PATHS ${WEB_FIXTURE_FILES} FORMAT zip MTIME "2020-01-01" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests/fixtures/web")
juce_add_binary_data(TestWebResources HEADER_NAME TestWebResources.h NAMESPACE TestWebResourcesData SOURCES "${WEB_FIXTURE_ARCHIVE}")

# Licensing sources (expanded during plan execution)
set(LICENSING_SOURCES
  "${CMAKE_SOURCE_DIR}/plugin/source/Licensing/MachineIdentity.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Licensing/EntitlementToken.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Licensing/PublicKeyRing.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Licensing/TokenVerifier.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Licensing/LocalEntitlementStore.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Licensing/GraceModel.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Licensing/LicensingState.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Licensing/LicensingCore.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Licensing/ActivationClient.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Licensing/tweetnacl/tweetnacl.c"
)

set(LICENSING_TEST_SOURCES
  "${CMAKE_SOURCE_DIR}/tests/native/Licensing/MachineIdentityTests.cpp"
  "${CMAKE_SOURCE_DIR}/tests/native/Licensing/EntitlementTokenTests.cpp"
  "${CMAKE_SOURCE_DIR}/tests/native/Licensing/TokenVerifierTests.cpp"
  "${CMAKE_SOURCE_DIR}/tests/native/Licensing/LocalEntitlementStoreTests.cpp"
  "${CMAKE_SOURCE_DIR}/tests/native/Licensing/GraceModelTests.cpp"
  "${CMAKE_SOURCE_DIR}/tests/native/Licensing/LicensingCoreTests.cpp"
  "${CMAKE_SOURCE_DIR}/tests/native/Licensing/ActivationClientTests.cpp"
)

add_executable(${PROJECT_CONFIG_TEST_TARGET} AudioConversionTests.cpp EditorSnapshotPollingTests.cpp HostBridgeTests.cpp PluginProcessorTests.cpp RealtimeHandoffTests.cpp SourcePreferenceStoreTests.cpp SpectrumAnalyzerTests.cpp StandaloneSourceControllerTests.cpp WasapiEndpointModelTests.cpp WebResourcesTests.cpp
  "${CMAKE_SOURCE_DIR}/plugin/source/HostBridge.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Analyzer/AnalyzerConfig.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Analyzer/SpectrumAnalyzer.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/PluginProcessor.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Standalone/SourcePreferenceStore.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Standalone/StandaloneSourceController.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Standalone/windows/WasapiEndpointEnumerator.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Standalone/windows/WasapiLoopbackSourceAdapter.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/Standalone/windows/WasapiDeviceNotifications.cpp"
  "${CMAKE_SOURCE_DIR}/plugin/source/WebResources.cpp"
  ${LICENSING_SOURCES}
  ${LICENSING_TEST_SOURCES})
target_include_directories(${PROJECT_CONFIG_TEST_TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/plugin/include" "${PROJECT_CONFIG_GENERATED_INCLUDE_DIR}")
target_compile_features(${PROJECT_CONFIG_TEST_TARGET} PRIVATE cxx_std_20)
target_compile_definitions(${PROJECT_CONFIG_TEST_TARGET} PRIVATE
  JucePlugin_Name="${PROJECT_CONFIG_NAME}"
  JucePlugin_VersionString="${PROJECT_CONFIG_VERSION_STR}"
  JucePlugin_Build_Standalone=0
  LUMASCOPE_NATIVE_TESTS=1
  LUMASCOPE_PLUGIN_PROCESSOR_SOURCE="${CMAKE_SOURCE_DIR}/plugin/source/PluginProcessor.cpp"
  LUMASCOPE_SNAPSHOT_MAILBOX_HEADER="${CMAKE_SOURCE_DIR}/plugin/include/LumaScope/SnapshotMailbox.h"
  LUMASCOPE_WEBVIEW_DEV_SERVER=""
  LUMASCOPE_ACTIVATION_BASE_URL="http://localhost:8787"
  LUMASCOPE_BUILD_MARKER="${PROJECT_CONFIG_VERSION_STR}-test"
  LUMASCOPE_FIXTURE_UI_READY="${CMAKE_SOURCE_DIR}/tests/fixtures/bridge/ui-ready-v1.json"
  LUMASCOPE_FIXTURE_HOST_INFO="${CMAKE_SOURCE_DIR}/tests/fixtures/bridge/host-info-v1.json"
  LUMASCOPE_FIXTURE_SPECTRUM_SNAPSHOT="${CMAKE_SOURCE_DIR}/tests/fixtures/bridge/spectrum-snapshot-v1.json"
  LUMASCOPE_FIXTURE_UNSUPPORTED="${CMAKE_SOURCE_DIR}/tests/fixtures/bridge/unsupported-version.json"
  LUMASCOPE_FIXTURE_MALFORMED="${CMAKE_SOURCE_DIR}/tests/fixtures/bridge/malformed.json"
  LUMASCOPE_FIXTURE_ENTITLEMENT_V1="${CMAKE_SOURCE_DIR}/tests/worker/fixtures/entitlement-v1.json"
  LUMASCOPE_FIXTURE_KEY_RING="${CMAKE_SOURCE_DIR}/tests/native/fixtures/public-key-ring.json")
target_link_libraries(${PROJECT_CONFIG_TEST_TARGET} PRIVATE TestWebResources juce::juce_audio_devices juce::juce_audio_processors juce::juce_dsp juce::juce_gui_extra bcrypt crypt32)
add_test(NAME ${PROJECT_CONFIG_TEST_TARGET} COMMAND ${PROJECT_CONFIG_TEST_TARGET})
