#include "LumaScope/PluginEditor.h"
#include "LumaScopeWebBundle.h"

namespace
{
auto makeBrowserOptions (LumaScopeAudioProcessorEditor* editor,
                         lumascope::WebResources& resources)
{
    return juce::WebBrowserComponent::Options {}
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled()
        .withResourceProvider ([&resources] (const juce::String& path) { return resources.get (path); })
        .withInitialisationData ("protocolVersion", lumascope::HostBridge::protocolVersion)
        .withEventListener (lumascope::HostBridge::uiReadyEvent,
                            [editor] (const juce::var& payload) { editor->handleUiReady (payload); });
}
}

juce::String LumaScopeAudioProcessorEditor::hostMode()
{
   #if JucePlugin_Build_Standalone
    return "Standalone";
   #else
    return "VST3";
   #endif
}

LumaScopeAudioProcessorEditor::LumaScopeAudioProcessorEditor (LumaScopeAudioProcessor& processor)
    : AudioProcessorEditor (&processor),
      resources (LumaScopeWebBundleData::lumascopeweb_zip,
                 static_cast<std::size_t> (LumaScopeWebBundleData::lumascopeweb_zipSize)),
      bridge (hostMode(), "embedded", JucePlugin_VersionString),
      browser (makeBrowserOptions (this, resources))
{
    addAndMakeVisible (browser);
    setSize (960, 600);
    setResizable (true, true);
    setResizeLimits (560, 360, 1920, 1200);
    browser.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
}

void LumaScopeAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour (0xff0b0f12));
    graphics.setColour (juce::Colour (0xfff2f7f8));
    graphics.setFont (24.0f);
    graphics.drawFittedText ("LumaScope", getLocalBounds(), juce::Justification::centred, 1);
}

void LumaScopeAudioProcessorEditor::resized()
{
    browser.setBounds (getLocalBounds());
}

void LumaScopeAudioProcessorEditor::handleUiReady (const juce::var& payload)
{
    const auto response = bridge.handleUiReady (payload);
    browser.emitEventIfBrowserIsVisible (response.eventId, response.payload);

   #if JUCE_DEBUG && JucePlugin_Build_Standalone
    const auto resultPath = juce::SystemStats::getEnvironmentVariable ("LUMASCOPE_SMOKE_RESULT_FILE", {});
    if (resultPath.isNotEmpty())
    {
        auto result = juce::var (new juce::DynamicObject());
        auto* object = result.getDynamicObject();
        object->setProperty ("status", response.ready ? "ready" : "error");
        object->setProperty ("protocolVersion", lumascope::HostBridge::protocolVersion);
        object->setProperty ("uiSource", "embedded");
        if (! response.ready)
            object->setProperty ("errorCode", response.payload["code"]);
        juce::File (resultPath).replaceWithText (juce::JSON::toString (result, true));
        juce::JUCEApplicationBase::quit();
    }
   #endif
}
