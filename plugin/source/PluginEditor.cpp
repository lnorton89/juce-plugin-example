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
   #if JucePlugin_Build_Standalone
    options = options
        .withEventListener (lumascope::HostBridge::sourceSelectEvent,
                            [editor] (const juce::var& payload) { editor->handleSourceSelect (payload); })
        .withEventListener (lumascope::HostBridge::sourceStopEvent,
                            [editor] (const juce::var& payload) { editor->handleSourceStop (payload); });
   #endif
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
    juce::String simulation;
   #if JUCE_DEBUG
    simulation = juce::SystemStats::getEnvironmentVariable ("LUMASCOPE_SIMULATE_WEB_FAILURE", {});
   #endif
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

    if (response.ready)
    {
        bridgeReady = true;
        startTimerHz (60);
    }

    writeSmokeResult (response.ready ? "ready" : "error",
                      response.ready ? juce::String {} : response.payload["code"].toString());
}

void LumaScopeAudioProcessorEditor::handleSourceSelect (const juce::var& payload)
{
    auto& audioProcessor = static_cast<LumaScopeAudioProcessor&> (*getAudioProcessor());
    auto* controller = audioProcessor.getStandaloneSourceController();
    if (controller == nullptr)
    {
        browser.emitEventIfBrowserIsVisible (lumascope::HostBridge::bridgeErrorEvent,
            lumascope::HostBridge::makeError ("unsupported_mode", "Source selection is only available in standalone mode."));
        return;
    }

    const auto* object = payload.getDynamicObject();
    if (object == nullptr)
    {
        browser.emitEventIfBrowserIsVisible (lumascope::HostBridge::bridgeErrorEvent,
            lumascope::HostBridge::makeError ("malformed_payload", "source.select payload must be a JSON object."));
        return;
    }

    const auto version = object->getProperty ("protocolVersion");
    if (static_cast<int> (version) != lumascope::HostBridge::protocolVersion)
    {
        browser.emitEventIfBrowserIsVisible (lumascope::HostBridge::bridgeErrorEvent,
            lumascope::HostBridge::makeError ("protocol_mismatch", "Protocol version mismatch in source.select."));
        return;
    }

    const auto modeStr = object->getProperty ("mode").toString();
    lumascope::SourceMode mode;
    if (modeStr == "InputDevice")
        mode = lumascope::SourceMode::inputDevice;
    else if (modeStr == "SystemOutput")
        mode = lumascope::SourceMode::systemOutput;
    else
    {
        browser.emitEventIfBrowserIsVisible (lumascope::HostBridge::bridgeErrorEvent,
            lumascope::HostBridge::makeError ("invalid_selection", "Unknown source mode: " + modeStr.substring (0, 32)));
        return;
    }

    const auto sourceId = object->getProperty ("sourceId").toString();
    if (sourceId.isEmpty())
    {
        browser.emitEventIfBrowserIsVisible (lumascope::HostBridge::bridgeErrorEvent,
            lumascope::HostBridge::makeError ("invalid_selection", "source.select must include a non-empty sourceId."));
        return;
    }

    lumascope::SourceSelection selection;
    selection.mode = mode;
    selection.id = sourceId.substring (0, 256);

    // Find display name from current source list
    const auto currentList = controller->enumerateSources();
    const auto& inputDevices = currentList.inputDevices;
    const auto& systemOutputs = currentList.systemOutputs;
    bool found = false;
    for (const auto& desc : inputDevices)
    {
        if (desc.id == selection.id)
        {
            selection.displayName = desc.displayName;
            found = true;
            break;
        }
    }
    if (! found)
    {
        for (const auto& desc : systemOutputs)
        {
            if (desc.id == selection.id)
            {
                selection.displayName = desc.displayName;
                found = true;
                break;
            }
        }
    }

    if (! found)
    {
        browser.emitEventIfBrowserIsVisible (lumascope::HostBridge::bridgeErrorEvent,
            lumascope::HostBridge::makeError ("source_not_found", "The selected source is not in the current device list."));
        return;
    }

    // Emit source state event from the new state
    const auto state = controller->selectSource (selection);
    browser.emitEventIfBrowserIsVisible (lumascope::HostBridge::sourceStateEvent,
        lumascope::HostBridge::makeSourceStateSnapshot (state));
}

void LumaScopeAudioProcessorEditor::handleSourceStop (const juce::var& payload)
{
    juce::ignoreUnused (payload);
    auto& audioProcessor = static_cast<LumaScopeAudioProcessor&> (*getAudioProcessor());
    auto* controller = audioProcessor.getStandaloneSourceController();
    if (controller == nullptr)
    {
        browser.emitEventIfBrowserIsVisible (lumascope::HostBridge::bridgeErrorEvent,
            lumascope::HostBridge::makeError ("unsupported_mode", "Source stop is only available in standalone mode."));
        return;
    }

    const auto state = controller->stop();
    browser.emitEventIfBrowserIsVisible (lumascope::HostBridge::sourceStateEvent,
        lumascope::HostBridge::makeSourceStateSnapshot (state));
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
    if (! bridgeReady)
    {
        showFallback ("handshake_timeout", "The interface did not complete protocol v1 startup. Rebuild the native app and UI from the same checkout.");
        return;
    }

    auto& audioProcessor = static_cast<LumaScopeAudioProcessor&> (*getAudioProcessor());
    snapshotPoller.poll (juce::Time::getMillisecondCounterHiRes(),
                         [&audioProcessor] (lumascope::SpectrumSnapshot& snapshot, std::uint32_t& lastSeenSequence)
                         {
                             return audioProcessor.readLatestSpectrumSnapshot (snapshot, lastSeenSequence);
                         },
                         [this] (const lumascope::SpectrumSnapshot& snapshot)
                         {
                             browser.emitEventIfBrowserIsVisible (lumascope::HostBridge::spectrumSnapshotEvent,
                                                                  lumascope::HostBridge::makeSpectrumSnapshot (snapshot));
                         });
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
