include("${CMAKE_SOURCE_DIR}/cmake/WebBundle.cmake")

if(LUMASCOPE_RELEASE_BUILD AND LUMASCOPE_WEBVIEW_DEV_SERVER)
  message(FATAL_ERROR "LUMASCOPE_WEBVIEW_DEV_SERVER must be empty in Release builds. "
    "Set the environment variable to empty or use a Debug preset.")
endif()
if(LUMASCOPE_WEBVIEW_DEV_SERVER AND NOT LUMASCOPE_WEBVIEW_DEV_SERVER MATCHES "^http://127\\.0\\.0\\.1:5174/?$")
  message(FATAL_ERROR "LUMASCOPE_WEBVIEW_DEV_SERVER must be exactly "
    "'http://127.0.0.1:5174' (canonical loopback) or empty. "
    "Got: '${LUMASCOPE_WEBVIEW_DEV_SERVER}'. "
    "Hostnames, remote addresses, HTTPS, alternate ports, credentials, "
    "paths, queries, and fragments are rejected.")
endif()

juce_add_plugin(LumaScope
  COMPANY_NAME "Signal Foundry Audio"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  BUNDLE_ID "audio.signal-foundry.LumaScope"
  PLUGIN_MANUFACTURER_CODE SgFd
  PLUGIN_CODE LmSc
  FORMATS VST3 Standalone
  PRODUCT_NAME "LumaScope"
  NEEDS_WEB_BROWSER TRUE
  NEEDS_WEBVIEW2 TRUE)

target_sources(LumaScope PRIVATE
  include/LumaScope/PluginEditor.h
  include/LumaScope/PluginProcessor.h
  include/LumaScope/Analyzer/AnalyzerConfig.h
  include/LumaScope/Analyzer/SpectrumAnalyzer.h
  include/LumaScope/Analyzer/SpectrumSnapshot.h
  include/LumaScope/HostBridge.h
  include/LumaScope/WebResources.h
  source/Analyzer/AnalyzerConfig.cpp
  source/Analyzer/SpectrumAnalyzer.cpp
  source/HostBridge.cpp
  source/PluginEditor.cpp
  source/PluginProcessor.cpp
  source/WebResources.cpp)

target_include_directories(LumaScope PUBLIC include)
target_compile_features(LumaScope PUBLIC cxx_std_20)
target_compile_definitions(LumaScope PUBLIC
  JUCE_WEB_BROWSER=1
  JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  LUMASCOPE_WEBVIEW_DEV_SERVER="$<$<CONFIG:Debug>:${LUMASCOPE_WEBVIEW_DEV_SERVER}>"
  LUMASCOPE_BUILD_MARKER="${PROJECT_VERSION}-$<CONFIG>")
target_link_libraries(LumaScope PRIVATE
  LumaScopeWebBundle
  juce::juce_audio_utils
  juce::juce_dsp
  juce::juce_gui_extra
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_dependencies(LumaScope LumaScopeWebBundle)
