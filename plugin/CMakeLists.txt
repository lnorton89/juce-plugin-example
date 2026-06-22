include("${CMAKE_SOURCE_DIR}/cmake/WebBundle.cmake")

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
  include/LumaScope/HostBridge.h
  include/LumaScope/WebResources.h
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
  juce::juce_gui_extra
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
add_dependencies(LumaScope LumaScopeWebBundle)
