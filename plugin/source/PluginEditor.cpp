#include "LumaScope/PluginEditor.h"
#include "LumaScopeWebBundle.h"

namespace
{
constexpr auto canonicalDevServer = "http://127.0.0.1:5174";

auto makeBrowserOptions (LumaScopeAudioProcessorEditor* editor,
                         lumascope::WebResources& resources,
                         const juce::String& allowedOrigin)
{
    auto options = juce::WebBrowserComponent::Options {}
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled()
        .withInitialisationData ("protocolVersion", lumascope::HostBridge::protocolVersion)
        .withEventListener (lumascope::HostBridge::uiReadyEvent,
                            [editor] (const juce::var& payload) { editor->handleUiReady (payload); });
    if (allowedOrigin.isNotEmpty())
        return options.withResourceProvider ([&resources] (const juce::String& path) { return resources.get (path); },
                                             allowedOrigin);
    return options.withResourceProvider ([&resources] (const juce::String& path) { return resources.get (path); });
}
}

LumaScopeAudioProcessorEditor::Browser::Browser (LumaScopeAudioProcessorEditor& ownerIn,
                                                  const Options& options)
    : WebBrowserComponent (options), owner (ownerIn) {}

bool LumaScopeAudioProcessorEditor::Browser::pageLoadHadNetworkError (const juce::String& errorInfo)
{
    owner.handleBrowserNetworkError (errorInfo);
    return false;
}

juce::String LumaScopeAudioProcessorEditor::hostMode()
{
   #if JucePlugin_Build_Standalone
    return "Standalone";
   #else
    return "VST3";
   #endif
}

juce::String LumaScopeAudioProcessorEditor::configuredDevServer()
{
    return LUMASCOPE_WEBVIEW_DEV_SERVER;
}

bool LumaScopeAudioProcessorEditor::isCanonicalDevServer (const juce::String& value)
{
    if (value.isEmpty())
        return false;
    const juce::URL url (value);
    return value == canonicalDevServer && url.isWellFormed() && url.getScheme() == "http"
        && url.getDomain() == "127.0.0.1" && url.getPort() == 5174
        && url.getSubPath().isEmpty() && url.getQueryString().isEmpty()
        && url.getAnchorString().isEmpty() && url.getOrigin() == canonicalDevServer;
}

LumaScopeAudioProcessorEditor::LumaScopeAudioProcessorEditor (LumaScopeAudioProcessor& processor)
    : AudioProcessorEditor (&processor),
      resources (LumaScopeWebBundleData::lumascopeweb_zip,
                 static_cast<std::size_t> (LumaScopeWebBundleData::lumascopeweb_zipSize)),
      bridge (hostMode(), isCanonicalDevServer (configuredDevServer()) ? "vite" : "embedded",
              JucePlugin_VersionString, LUMASCOPE_BUILD_MARKER),
      browser (*this, makeBrowserOptions (this, resources,
                                         isCanonicalDevServer (configuredDevServer()) ? configuredDevServer() : juce::String {})),
      uiSource (isCanonicalDevServer (configuredDevServer()) ? "vite" : "embedded")
{
    addAndMakeVisible (browser);
    setSize (960, 600);
    setResizable (true, true);
    setResizeLimits (560, 360, 1920, 1200);
    const auto simulation = juce::SystemStats::getEnvironmentVariable ("LUMASCOPE_SIMULATE_WEB_FAILURE", {});
    if (simulation == "webview2")
    {
        showFallback ("webview2_unavailable", "The WebView2 runtime is unavailable. Install or repair Evergreen WebView2, then reopen LumaScope.");
        return;
    }
    if (simulation == "resource")
    {
        showFallback ("embedded_resource_unavailable", "The packaged interface could not be loaded. Rebuild LumaScope and reopen it.");
        return;
    }
    if (! juce::WebBrowserComponent::areOptionsSupported (makeBrowserOptions (this, resources,
            uiSource == "vite" ? configuredDevServer() : juce::String {})))
    {
        showFallback ("webview2_unavailable", "The WebView2 runtime is unavailable. Install or repair Evergreen WebView2, then reopen LumaScope.");
        return;
    }
    browser.goToURL (uiSource == "vite" ? configuredDevServer()
                                        : juce::WebBrowserComponent::getResourceProviderRoot());
    startTimer (simulation == "handshake" ? 250 : 10000);
}

void LumaScopeAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour (0xff0b0f12));
    const auto fallbackBounds = getLocalBounds().reduced (16);
    graphics.setColour (juce::Colour (0xff121a20));
    graphics.fillRoundedRectangle (fallbackBounds.toFloat(), 10.0f);
    graphics.setColour (juce::Colour (0xff24323a));
    graphics.drawRoundedRectangle (fallbackBounds.toFloat(), 10.0f, 1.0f);
    graphics.setColour (juce::Colour (0xfff2f7f8));
    graphics.setFont (juce::FontOptions { 24.0f });
    graphics.drawFittedText ("LumaScope", fallbackBounds.withTrimmedBottom (fallbackBounds.getHeight() / 2),
                             juce::Justification::centredBottom, 1);
    graphics.setColour (juce::Colour (0xff91a4ae));
    graphics.setFont (juce::FontOptions { 14.0f });
    graphics.drawFittedText (fallbackMessage.isNotEmpty() ? fallbackMessage
                                                          : "The interface could not start. Check the WebView2 runtime and reopen LumaScope.",
                             fallbackBounds.withTrimmedTop (fallbackBounds.getHeight() / 2),
                             juce::Justification::centredTop, 2);
}

void LumaScopeAudioProcessorEditor::resized()
{
    browser.setBounds (getLocalBounds());
}

void LumaScopeAudioProcessorEditor::handleUiReady (const juce::var& payload)
{
    stopTimer();
    const auto response = bridge.handleUiReady (payload);
    browser.emitEventIfBrowserIsVisible (response.eventId, response.payload);

    writeSmokeResult (response.ready ? "ready" : "error",
                      response.ready ? juce::String {} : response.payload["code"].toString());
}

void LumaScopeAudioProcessorEditor::handleBrowserNetworkError (const juce::String& errorInfo)
{
    const auto code = uiSource == "vite" ? "development_server_unavailable" : "embedded_resource_unavailable";
    const auto message = uiSource == "vite"
        ? "Development server unavailable. Start npm --prefix ui run dev or switch to embedded assets."
        : "The packaged interface could not be loaded. Rebuild LumaScope and reopen it.";
    showFallback (code, juce::String (message) + " " + errorInfo.substring (0, 128));
}

void LumaScopeAudioProcessorEditor::timerCallback()
{
    showFallback ("handshake_timeout", "The interface did not complete protocol v1 startup. Rebuild the native app and UI from the same checkout.");
}

void LumaScopeAudioProcessorEditor::showFallback (juce::String code, juce::String message)
{
    stopTimer();
    fallbackCode = std::move (code);
    fallbackMessage = std::move (message);
    browser.setVisible (false);
    repaint();
    writeSmokeResult ("error", fallbackCode);
}

void LumaScopeAudioProcessorEditor::writeSmokeResult (juce::String status, juce::String errorCode)
{
   #if JUCE_DEBUG && JucePlugin_Build_Standalone
    const auto resultPath = juce::SystemStats::getEnvironmentVariable ("LUMASCOPE_SMOKE_RESULT_FILE", {});
    if (resultPath.isEmpty())
        return;
    auto result = juce::var (new juce::DynamicObject());
    auto* object = result.getDynamicObject();
    object->setProperty ("status", status);
    object->setProperty ("protocolVersion", lumascope::HostBridge::protocolVersion);
    object->setProperty ("uiSource", uiSource);
    if (errorCode.isNotEmpty())
        object->setProperty ("errorCode", errorCode);
    juce::File (resultPath).replaceWithText (juce::JSON::toString (result, true));
    juce::JUCEApplicationBase::quit();
   #else
    juce::ignoreUnused (status, errorCode);
   #endif
}
