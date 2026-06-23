#include "LumaScope/Standalone/StandaloneSourceController.h"
#include "LumaScope/Standalone/SourceModel.h"
#include "LumaScope/Standalone/WasapiLoopbackSourceAdapter.h"
#include "LumaScope/PluginProcessor.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace lumascope
{

// ============================================================================
// JuceInputSourceAdapter: JUCE AudioDeviceManager callback that feeds
// pre-converted float blocks into the existing analyzer ingress.
//
// Real-time safety:
// - Uses preallocated scratch conversion buffer sized at construction.
// - Does NOT allocate, lock, perform I/O, build JSON, or call WebView
//   inside audioDeviceIOCallbackWithContext.
// - Calls only pushStandaloneAudioBlock on the processor, which itself
//   is real-time safe (validated in Phase 2).
// ============================================================================
class JuceInputSourceAdapter final : public juce::AudioIODeviceCallback
{
public:
    JuceInputSourceAdapter (LumaScopeAudioProcessor& processorRef,
                            std::size_t maxExpectedSamples)
        : processor (processorRef),
          scratch (1, static_cast<int> (maxExpectedSamples))
    {
        scratch.clear();
    }

    ~JuceInputSourceAdapter() override = default;

    // JUCE 8 callback with context parameter
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override
    {
        juce::ignoreUnused (outputChannelData, numOutputChannels);

        if (inputChannelData == nullptr || numInputChannels <= 0 || numSamples <= 0)
            return;

        // Ensure scratch buffer is large enough (bounded resize)
        if (scratch.getNumSamples() < numSamples)
            scratch.setSize (1, numSamples, false, false, true);

        // Downmix all input channels to mono into scratch buffer
        auto* scratchData = scratch.getWritePointer (0);

        if (numInputChannels == 1)
        {
            // Single channel: direct copy
            std::copy (inputChannelData[0],
                       inputChannelData[0] + numSamples,
                       scratchData);
        }
        else
        {
            // Multi-channel: average all channels
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float sum = 0.0f;
                for (int ch = 0; ch < numInputChannels; ++ch)
                {
                    if (inputChannelData[ch] != nullptr)
                        sum += inputChannelData[ch][sample];
                }
                scratchData[sample] = sum / static_cast<float> (numInputChannels);
            }
        }

        // Feed the existing analyzer ingress (real-time safe)
        processor.pushStandaloneAudioBlock (scratch);
    }

    void audioDeviceAboutToStart (juce::AudioIODevice*) override
    {
        // Prepare step — no allocations beyond pre-sized scratch
    }

    void audioDeviceStopped() override
    {
        // Device stopped — state observed via controller on message thread
    }

    JuceInputSourceAdapter (const JuceInputSourceAdapter&) = delete;
    JuceInputSourceAdapter& operator= (const JuceInputSourceAdapter&) = delete;

private:
    LumaScopeAudioProcessor& processor;
    juce::AudioBuffer<float> scratch;
};

// ============================================================================
// StandaloneSourceControllerImpl: concrete implementation
// ============================================================================
class StandaloneSourceControllerImpl final : public StandaloneSourceController
{
public:
    explicit StandaloneSourceControllerImpl (LumaScopeAudioProcessor& proc)
        : processor (proc)
    {
    }

    ~StandaloneSourceControllerImpl() override
    {
        stop();
    }

    SourceList enumerateSources() override
    {
        SourceList list;

        // Enumerate JUCE input devices (microphones, interfaces, etc.)
        const auto& deviceTypes = deviceManager.getAvailableDeviceTypes();

        for (auto* type : deviceTypes)
        {
            if (type == nullptr)
                continue;

            const auto inputNames = type->getDeviceNames (true); // true = inputs

            for (int i = 0; i < inputNames.size(); ++i)
            {
                const auto name = inputNames[i];
                if (name.isNotEmpty())
                {
                    SourceDescriptor desc;
                    desc.id = "juce-input-" + name;
                    desc.displayName = name;
                    desc.mode = SourceMode::inputDevice;
                    list.inputDevices.push_back (std::move (desc));
                }
            }
        }

        // Enumerate WASAPI render endpoints for SystemOutput mode
        list.systemOutputs = enumerateRenderEndpoints();

        return list;
    }

    SourceStateSnapshot selectSource (const SourceSelection& selection) override
    {
        // Stop any existing adapter first
        stop();

        if (selection.id.isEmpty())
        {
            stateSnapshot.mode = selection.mode;
            stateSnapshot.state = SourceState::error;
            stateSnapshot.code = "invalid_selection";
            stateSnapshot.message = "No source identifier provided.";
            return stateSnapshot;
        }

        // Update state to starting
        stateSnapshot.mode = selection.mode;
        stateSnapshot.state = SourceState::starting;
        stateSnapshot.selectedSourceId = selection.id;
        stateSnapshot.selectedSourceName = selection.displayName;
        stateSnapshot.code = {};
        stateSnapshot.message = {};

        if (selection.mode == SourceMode::inputDevice)
        {
            // Try to open the JUCE input device
            juce::String errorResult = deviceManager.initialise (
                2,     // max input channels
                0,     // max output channels (capture only)
                nullptr,
                true); // select default device on failure

            if (errorResult.isNotEmpty())
            {
                stateSnapshot.state = SourceState::error;
                stateSnapshot.code = "device_init_failed";
                stateSnapshot.message = "Could not open input device: " + errorResult.substring (0, 128);
                return stateSnapshot;
            }

            // Create and register the input adapter
            juceAdapter = std::make_unique<JuceInputSourceAdapter> (processor, 8192);
            deviceManager.addAudioCallback (juceAdapter.get());

            if (deviceManager.getCurrentAudioDevice() != nullptr
                && deviceManager.getCurrentAudioDevice()->isOpen())
            {
                stateSnapshot.state = SourceState::active;
            }
            else
            {
                stateSnapshot.state = SourceState::starting;
            }
        }
        else if (selection.mode == SourceMode::systemOutput)
        {
            // Start WASAPI loopback capture
            wasapiAdapter = std::make_unique<WasapiLoopbackSourceAdapter> (processor);
            juce::String errorResult = wasapiAdapter->start (selection.id);

            if (errorResult.isNotEmpty())
            {
                stateSnapshot.state = SourceState::error;
                stateSnapshot.code = "endpoint_start_failed";
                stateSnapshot.message = errorResult.substring (0, 256);
                wasapiAdapter.reset();
                return stateSnapshot;
            }

            stateSnapshot.state = SourceState::active;
        }
        else
        {
            stateSnapshot.state = SourceState::error;
            stateSnapshot.code = "unknown_mode";
            stateSnapshot.message = "Unknown source mode.";
        }

        return stateSnapshot;
    }

    SourceStateSnapshot stop() override
    {
        if (juceAdapter != nullptr)
        {
            deviceManager.removeAudioCallback (juceAdapter.get());
            juceAdapter.reset();
        }

        deviceManager.closeAudioDevice();

        if (wasapiAdapter != nullptr)
        {
            wasapiAdapter->stop();
            wasapiAdapter.reset();
        }

        stateSnapshot = {};
        stateSnapshot.state = SourceState::stopped;

        return stateSnapshot;
    }

    SourceStateSnapshot currentStateSnapshot() const override
    {
        // For WASAPI adapter, poll the live capture state for active/silent transitions
        if (wasapiAdapter != nullptr && wasapiAdapter->isRunning())
        {
            const auto captureState = wasapiAdapter->currentCaptureState();
            if (captureState != stateSnapshot.state
                && (captureState == SourceState::active || captureState == SourceState::silent))
            {
                stateSnapshot.state = captureState;
            }
        }

        return stateSnapshot;
    }

    StandaloneSourceControllerImpl (const StandaloneSourceControllerImpl&) = delete;
    StandaloneSourceControllerImpl& operator= (const StandaloneSourceControllerImpl&) = delete;

private:
    LumaScopeAudioProcessor& processor;
    juce::AudioDeviceManager deviceManager;
    std::unique_ptr<JuceInputSourceAdapter> juceAdapter;
    std::unique_ptr<WasapiLoopbackSourceAdapter> wasapiAdapter;
    mutable SourceStateSnapshot stateSnapshot;
};

// Factory function
std::unique_ptr<StandaloneSourceController> createStandaloneSourceController (
    LumaScopeAudioProcessor& processor)
{
    return std::make_unique<StandaloneSourceControllerImpl> (processor);
}

} // namespace lumascope
