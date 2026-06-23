#include "LumaScope/Standalone/WasapiLoopbackSourceAdapter.h"
#include "LumaScope/PluginProcessor.h"

#include <juce_core/juce_core.h>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <initguid.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <audioclient.h>
#include <ksmedia.h>
#include <comdef.h>
#include <comutil.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace lumascope
{

// ============================================================================
// Utility: safe release a COM interface pointer
// ============================================================================
template <typename T>
static void safeReleaseCom (T*& ptr) noexcept
{
    if (ptr != nullptr) { ptr->Release(); ptr = nullptr; }
}

// ============================================================================
// WasapiConversion implementations
// ============================================================================
namespace WasapiConversion
{

void convertFloat32 (const std::uint8_t* source, float* const* destChannels,
                     int numSourceChannels, int numDestChannels,
                     int frameCount) noexcept
{
    if (source == nullptr || destChannels == nullptr || frameCount <= 0
        || numSourceChannels <= 0 || numDestChannels <= 0)
        return;

    const auto* src = reinterpret_cast<const float*> (source);

    if (numSourceChannels == 1 && numDestChannels == 2)
    {
        // Mono source -> stereo: duplicate
        for (int f = 0; f < frameCount; ++f)
        {
            const float sample = src[f];
            destChannels[0][f] = sample;
            destChannels[1][f] = sample;
        }
    }
    else if (numDestChannels >= numSourceChannels)
    {
        // Source has fewer or equal channels: copy available, pad rest with 0
        for (int f = 0; f < frameCount; ++f)
        {
            for (int ch = 0; ch < numDestChannels; ++ch)
            {
                if (ch < numSourceChannels)
                    destChannels[ch][f] = src[f * numSourceChannels + ch];
                else
                    destChannels[ch][f] = 0.0f;
            }
        }
    }
    else
    {
        // Downmix: average all source channels into dest channels
        const int groupSize = numSourceChannels / numDestChannels;
        const int remainder = numSourceChannels % numDestChannels;

        for (int f = 0; f < frameCount; ++f)
        {
            int srcCh = 0;
            for (int ch = 0; ch < numDestChannels; ++ch)
            {
                const int chsInGroup = groupSize + (ch < remainder ? 1 : 0);
                float sum = 0.0f;
                for (int g = 0; g < chsInGroup; ++g)
                {
                    sum += src[f * numSourceChannels + srcCh];
                    ++srcCh;
                }
                destChannels[ch][f] = sum / static_cast<float> (chsInGroup);
            }
        }
    }
}

void convertInt16 (const std::uint8_t* source, float* const* destChannels,
                   int numSourceChannels, int numDestChannels,
                   int frameCount) noexcept
{
    if (source == nullptr || destChannels == nullptr || frameCount <= 0
        || numSourceChannels <= 0 || numDestChannels <= 0)
        return;

    constexpr float scale = 1.0f / 32768.0f;
    const auto* src = reinterpret_cast<const std::int16_t*> (source);

    // Convert to temporary interleaved float buffer for downmix
    // Prefer direct path for common cases
    if (numSourceChannels == numDestChannels)
    {
        for (int f = 0; f < frameCount; ++f)
            for (int ch = 0; ch < numDestChannels; ++ch)
                destChannels[ch][f] = static_cast<float> (src[f * numSourceChannels + ch]) * scale;
    }
    else
    {
        // Convert to temporary float interleaved buffer, then downmix
        // Use a small stack buffer for common frame sizes
        const auto totalSamples = static_cast<std::size_t> (frameCount) * static_cast<std::size_t> (numSourceChannels);
        std::vector<float> temp (totalSamples);
        for (std::size_t i = 0; i < totalSamples; ++i)
            temp[i] = static_cast<float> (src[i]) * scale;

        convertFloat32 (reinterpret_cast<const std::uint8_t*> (temp.data()),
                        destChannels, numSourceChannels, numDestChannels, frameCount);
    }
}

void convertInt24 (const std::uint8_t* source, float* const* destChannels,
                   int numSourceChannels, int numDestChannels,
                   int frameCount) noexcept
{
    if (source == nullptr || destChannels == nullptr || frameCount <= 0
        || numSourceChannels <= 0 || numDestChannels <= 0)
        return;

    constexpr float scale = 1.0f / 8388608.0f;
    const auto totalSamples = static_cast<std::size_t> (frameCount) * static_cast<std::size_t> (numSourceChannels);
    std::vector<float> temp (totalSamples);

    for (std::size_t i = 0; i < totalSamples; ++i)
    {
        // 3 bytes, little-endian, sign-extend
        std::uint32_t raw = static_cast<std::uint32_t> (source[i * 3])
                          | (static_cast<std::uint32_t> (source[i * 3 + 1]) << 8)
                          | (static_cast<std::uint32_t> (source[i * 3 + 2]) << 16);
        // Sign extend 24-bit to 32-bit
        if (raw & 0x800000u)
            raw |= 0xFF000000u;

        temp[i] = static_cast<float> (static_cast<std::int32_t> (raw)) * scale;
    }

    convertFloat32 (reinterpret_cast<const std::uint8_t*> (temp.data()),
                    destChannels, numSourceChannels, numDestChannels, frameCount);
}

void convertInt32 (const std::uint8_t* source, float* const* destChannels,
                   int numSourceChannels, int numDestChannels,
                   int frameCount) noexcept
{
    if (source == nullptr || destChannels == nullptr || frameCount <= 0
        || numSourceChannels <= 0 || numDestChannels <= 0)
        return;

    constexpr float scale = 1.0f / 2147483648.0f; // 2^31
    const auto* src = reinterpret_cast<const std::int32_t*> (source);

    if (numSourceChannels == numDestChannels)
    {
        for (int f = 0; f < frameCount; ++f)
            for (int ch = 0; ch < numDestChannels; ++ch)
                destChannels[ch][f] = static_cast<float> (src[f * numSourceChannels + ch]) * scale;
    }
    else
    {
        const auto totalSamples = static_cast<std::size_t> (frameCount) * static_cast<std::size_t> (numSourceChannels);
        std::vector<float> temp (totalSamples);
        for (std::size_t i = 0; i < totalSamples; ++i)
            temp[i] = static_cast<float> (src[i]) * scale;

        convertFloat32 (reinterpret_cast<const std::uint8_t*> (temp.data()),
                        destChannels, numSourceChannels, numDestChannels, frameCount);
    }
}

bool convertPacket (const std::uint8_t* source, int frameCount,
                    const void* format,
                    juce::AudioBuffer<float>& destBuffer) noexcept
{
    if (source == nullptr || format == nullptr || frameCount < 0)
        return false;

    if (frameCount == 0)
        return true;

    const auto* fmt = static_cast<const WAVEFORMATEX*> (format);
    const int channels = static_cast<int> (fmt->nChannels);
    if (channels <= 0)
        return false;

    const int destChannels = std::min (channels, maxOutputChannels);

    // Ensure destination buffer has enough space (this resize happens outside the hot path)
    if (destBuffer.getNumChannels() < destChannels || destBuffer.getNumSamples() < frameCount)
        destBuffer.setSize (destChannels, frameCount, false, false, true);

    // Prepare channel pointers
    float* dests[maxOutputChannels];
    for (int ch = 0; ch < destChannels; ++ch)
        dests[ch] = destBuffer.getWritePointer (ch);

    switch (fmt->wFormatTag)
    {
        case WAVE_FORMAT_IEEE_FLOAT:
            // 32-bit IEEE float
            if (fmt->wBitsPerSample != 32)
                return false;
            convertFloat32 (source, dests, channels, destChannels, frameCount);
            return true;

        case WAVE_FORMAT_PCM:
            switch (fmt->wBitsPerSample)
            {
                case 16:
                    convertInt16 (source, dests, channels, destChannels, frameCount);
                    return true;
                case 24:
                    convertInt24 (source, dests, channels, destChannels, frameCount);
                    return true;
                case 32:
                    convertInt32 (source, dests, channels, destChannels, frameCount);
                    return true;
                default:
                    return false; // Unsupported bit depth
            }

        case WAVE_FORMAT_EXTENSIBLE:
        {
            // WAVEFORMATEXTENSIBLE embeds WAVEFORMATEX at the start
            // Use the same sub-format detection via the SubFormat field
            // For now, handle PCM and float based on wBitsPerSample
            // The sub-format GUID tells us the actual sample type
            const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*> (fmt);
            if (ext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
            {
                if (fmt->wBitsPerSample != 32)
                    return false;
                convertFloat32 (source, dests, channels, destChannels, frameCount);
                return true;
            }
            else if (ext->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
            {
                switch (fmt->wBitsPerSample)
                {
                    case 16:
                        convertInt16 (source, dests, channels, destChannels, frameCount);
                        return true;
                    case 24:
                        convertInt24 (source, dests, channels, destChannels, frameCount);
                        return true;
                    case 32:
                        convertInt32 (source, dests, channels, destChannels, frameCount);
                        return true;
                    default:
                        return false;
                }
            }
            return false; // Unsupported sub-format
        }

        default:
            return false; // Unknown format tag
    }
}

double computeLevel (const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = buffer.getNumChannels();
    const auto samples = buffer.getNumSamples();

    if (channels <= 0 || samples <= 0)
        return 0.0;

    double sumAbs = 0.0;
    for (int ch = 0; ch < channels; ++ch)
    {
        const auto* data = buffer.getReadPointer (ch);
        for (int s = 0; s < samples; ++s)
            sumAbs += static_cast<double> (std::abs (data[s]));
    }

    return sumAbs / static_cast<double> (channels * samples);
}

} // namespace WasapiConversion

// ============================================================================
// WasapiLoopbackSourceAdapter
// ============================================================================

WasapiLoopbackSourceAdapter::WasapiLoopbackSourceAdapter (LumaScopeAudioProcessor& proc) noexcept
    : processor (proc)
{
}

WasapiLoopbackSourceAdapter::~WasapiLoopbackSourceAdapter()
{
    stop();
}

juce::String WasapiLoopbackSourceAdapter::start (const juce::String& endpointId)
{
    stop(); // Ensure any previous capture is stopped

    if (endpointId.isEmpty())
        return "Empty endpoint ID";

    // Strip the "wasapi-loopback-" prefix to get the raw endpoint ID
    juce::String rawId = endpointId;
    if (rawId.startsWith ("wasapi-loopback-"))
        rawId = rawId.substring (17); // length of "wasapi-loopback-"

    if (rawId.isEmpty())
        return "Invalid endpoint ID (empty after prefix strip)";

    // Initialize COM on this thread
    HRESULT hr = CoInitializeEx (nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED (hr) && hr != RPC_E_CHANGED_MODE)
        return "COM initialization failed";

    // Create device enumerator
    IMMDeviceEnumerator* enumerator = nullptr;
    hr = CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr,
                           CLSCTX_ALL, __uuidof (IMMDeviceEnumerator),
                           reinterpret_cast<void**> (&enumerator));
    if (FAILED (hr) || enumerator == nullptr)
    {
        CoUninitialize();
        return "Failed to create IMMDeviceEnumerator";
    }

    // Get the specific device by ID
    IMMDevice* device = nullptr;
    {
        const auto wideId = rawId.toWideCharPointer();
        hr = enumerator->GetDevice (wideId, &device);
    }

    if (FAILED (hr) || device == nullptr)
    {
        safeReleaseCom (enumerator);
        CoUninitialize();
        return "Endpoint not found: " + rawId;
    }

    // Retrieve the device state
    DWORD devState = 0;
    device->GetState (&devState);
    if (devState != DEVICE_STATE_ACTIVE)
    {
        safeReleaseCom (device);
        safeReleaseCom (enumerator);
        CoUninitialize();
        return "Endpoint is not active (state=" + juce::String (static_cast<int> (devState)) + ")";
    }

    // Activate IAudioClient
    IAudioClient* client = nullptr;
    hr = device->Activate (__uuidof (IAudioClient), CLSCTX_ALL, nullptr,
                           reinterpret_cast<void**> (&client));
    if (FAILED (hr) || client == nullptr)
    {
        safeReleaseCom (device);
        safeReleaseCom (enumerator);
        CoUninitialize();
        return "Failed to activate IAudioClient on endpoint";
    }

    // Get the mix format
    WAVEFORMATEX* fmt = nullptr;
    hr = client->GetMixFormat (&fmt);
    if (FAILED (hr) || fmt == nullptr)
    {
        safeReleaseCom (client);
        safeReleaseCom (device);
        safeReleaseCom (enumerator);
        CoUninitialize();
        return "Failed to get mix format from endpoint";
    }

    // Verify format is supported by our conversion
    bool formatSupported = false;
    if (fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT && fmt->wBitsPerSample == 32)
        formatSupported = true;
    else if (fmt->wFormatTag == WAVE_FORMAT_PCM)
        formatSupported = (fmt->wBitsPerSample == 16 || fmt->wBitsPerSample == 24 || fmt->wBitsPerSample == 32);
    else if (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        formatSupported = (fmt->wBitsPerSample == 16 || fmt->wBitsPerSample == 24 || fmt->wBitsPerSample == 32);

    if (! formatSupported)
    {
        CoTaskMemFree (fmt);
        safeReleaseCom (client);
        safeReleaseCom (device);
        safeReleaseCom (enumerator);
        CoUninitialize();
        return "Unsupported mix format (tag=" + juce::String (static_cast<int> (fmt->wFormatTag))
               + " bits=" + juce::String (static_cast<int> (fmt->wBitsPerSample)) + ")";
    }

    // Initialize loopback capture in shared mode
    REFERENCE_TIME bufferDuration = 100000; // 10 ms (100 ns units)
    hr = client->Initialize (AUDCLNT_SHAREMODE_SHARED,
                             AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                             bufferDuration, 0, fmt, nullptr);
    if (FAILED (hr))
    {
        CoTaskMemFree (fmt);
        safeReleaseCom (client);
        safeReleaseCom (device);
        safeReleaseCom (enumerator);
        CoUninitialize();

        if (hr == AUDCLNT_E_DEVICE_IN_USE)
            return "Endpoint is already in exclusive mode use by another application";
        else if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT)
            return "The endpoint does not support the shared-mode loopback format";
        else
            return "IAudioClient::Initialize failed (hr=0x" + juce::String::toHexString (static_cast<int> (hr)) + ")";
    }

    // Get the capture client
    IAudioCaptureClient* capture = nullptr;
    hr = client->GetService (__uuidof (IAudioCaptureClient),
                             reinterpret_cast<void**> (&capture));
    if (FAILED (hr) || capture == nullptr)
    {
        CoTaskMemFree (fmt);
        safeReleaseCom (client);
        safeReleaseCom (device);
        safeReleaseCom (enumerator);
        CoUninitialize();
        return "Failed to get IAudioCaptureClient";
    }

    // Store COM pointers
    audioClient = client;
    captureClient = capture;
    mixFormat = fmt;

    // Create event handle for capture notifications
    captureEvent = CreateEvent (nullptr, FALSE, FALSE, nullptr);
    if (captureEvent == nullptr)
    {
        CoTaskMemFree (fmt);
        safeReleaseCom (capture);
        safeReleaseCom (client);
        safeReleaseCom (device);
        safeReleaseCom (enumerator);
        CoUninitialize();
        return "Failed to create capture event handle";
    }

    hr = audioClient->SetEventHandle (static_cast<HANDLE> (captureEvent));
    if (FAILED (hr))
    {
        CloseHandle (static_cast<HANDLE> (captureEvent));
        captureEvent = nullptr;
        CoTaskMemFree (fmt);
        safeReleaseCom (capture);
        safeReleaseCom (client);
        safeReleaseCom (device);
        safeReleaseCom (enumerator);
        CoUninitialize();
        return "Failed to set event handle on audio client";
    }

    // Pre-allocate scratch buffer for expected max packet size
    const UINT32 framesPerBuffer = 1024; // ample for 10ms at 96kHz
    scratch.setSize (WasapiConversion::maxOutputChannels,
                     static_cast<int> (framesPerBuffer),
                     false, false, true);

    // Reset state
    shouldStop.store (false, std::memory_order_release);
    running.store (false, std::memory_order_release);
    recentLevel.store (0.0, std::memory_order_release);
    consecutiveSilentFrames.store (0, std::memory_order_release);

    // Store the device reference for the capture thread (needs it for format)
    // We release the enumerator but keep the device until capture thread starts.
    // The capture thread function will release the device reference.
    safeReleaseCom (enumerator);

    // Start capture on dedicated thread
    captureThread = std::make_unique<std::thread> (&WasapiLoopbackSourceAdapter::captureThreadFunc, this, device);

    return {}; // Success
}

void WasapiLoopbackSourceAdapter::stop()
{
    if (captureThread != nullptr && captureThread->joinable())
    {
        shouldStop.store (true, std::memory_order_release);
        if (captureEvent != nullptr)
            SetEvent (static_cast<HANDLE> (captureEvent)); // Wake up the thread
        captureThread->join();
        captureThread.reset();
    }

    if (captureEvent != nullptr)
    {
        CloseHandle (static_cast<HANDLE> (captureEvent));
        captureEvent = nullptr;
    }

    if (audioClient != nullptr)
    {
        audioClient->Stop();
        safeReleaseCom (captureClient);
        safeReleaseCom (audioClient);
    }

    if (mixFormat != nullptr)
    {
        CoTaskMemFree (mixFormat);
        mixFormat = nullptr;
    }

    running.store (false, std::memory_order_release);
}

SourceState WasapiLoopbackSourceAdapter::currentCaptureState() const noexcept
{
    if (! running.load (std::memory_order_acquire))
        return SourceState::stopped;

    const int silentCount = consecutiveSilentFrames.load (std::memory_order_acquire);
    if (silentCount >= silentFramesThreshold)
        return SourceState::silent;

    return SourceState::active;
}

void WasapiLoopbackSourceAdapter::resetSilenceDetection() noexcept
{
    consecutiveSilentFrames.store (0, std::memory_order_release);
    recentLevel.store (0.0, std::memory_order_release);
}

void WasapiLoopbackSourceAdapter::ensureScratchCapacity (int samples)
{
    if (scratch.getNumSamples() < samples)
    {
        scratch.setSize (scratch.getNumChannels(),
                         samples,
                         false, false, true);
    }
}

void WasapiLoopbackSourceAdapter::captureThreadFunc (IMMDevice* device)
{
    // No COM initialization needed — the device/client were created on this thread's
    // ancestors and the thread was started after initialization.
    // However, we need COM initialized for potential COM calls in the loop.
    HRESULT hr = CoInitializeEx (nullptr, COINIT_APARTMENTTHREADED);
    const bool comInitialized = SUCCEEDED (hr) || hr == RPC_E_CHANGED_MODE;

    // Release the device reference — we don't need it anymore
    safeReleaseCom (device);

    if (audioClient == nullptr || captureClient == nullptr || mixFormat == nullptr)
    {
        running.store (false, std::memory_order_release);
        if (comInitialized) CoUninitialize();
        return;
    }

    // Start the audio client
    hr = audioClient->Start();
    if (FAILED (hr))
    {
        running.store (false, std::memory_order_release);
        if (comInitialized) CoUninitialize();
        return;
    }

    running.store (true, std::memory_order_release);

    const auto* mixFmt = static_cast<const WAVEFORMATEX*> (mixFormat);
    const int channels = static_cast<int> (mixFmt->nChannels);
    const int frameSize = static_cast<int> (mixFmt->nBlockAlign);

    while (! shouldStop.load (std::memory_order_acquire))
    {
        DWORD waitResult = WaitForSingleObject (static_cast<HANDLE> (captureEvent), 100);
        if (waitResult != WAIT_OBJECT_0)
        {
            if (waitResult == WAIT_TIMEOUT)
                continue; // Check shouldStop
            break; // Unexpected error
        }

        // Read all available packets
        UINT32 packetSize = 0;
        HRESULT getSizeResult = captureClient->GetNextPacketSize (&packetSize);
        if (FAILED (getSizeResult))
            break;

        while (packetSize > 0)
        {
            BYTE* data = nullptr;
            UINT32 framesAvailable = 0;
            DWORD flags = 0;

            HRESULT getBufferResult = captureClient->GetBuffer (&data, &framesAvailable,
                                                                 &flags, nullptr, nullptr);
            if (FAILED (getBufferResult))
                break;

            if (framesAvailable > 0 && data != nullptr)
            {
                // Ensure scratch buffer is large enough (non-hot-path-safe resize)
                ensureScratchCapacity (static_cast<int> (framesAvailable));

                if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
                {
                    // Silent buffer: fill with zeros
                    const int destChannels = std::min (channels, WasapiConversion::maxOutputChannels);
                    for (int ch = 0; ch < destChannels; ++ch)
                        std::memset (scratch.getWritePointer (ch), 0,
                                     static_cast<std::size_t> (framesAvailable) * sizeof (float));
                }
                else
                {
                    // Convert the WASAPI packet to float
                    WasapiConversion::convertPacket (data, static_cast<int> (framesAvailable),
                                                     static_cast<const void*> (mixFormat), scratch);
                }

                // Compute level for silence detection
                if (framesAvailable > 0)
                {
                    double level = WasapiConversion::computeLevel (scratch);

                    // Exponential moving average for smoothing
                    constexpr double alpha = 0.1;
                    double prev = recentLevel.load (std::memory_order_relaxed);
                    double smoothed = alpha * level + (1.0 - alpha) * prev;
                    recentLevel.store (smoothed, std::memory_order_release);

                    // Update silence detection
                    constexpr double silenceThreshold = 1.0e-6;
                    if (smoothed < silenceThreshold)
                        consecutiveSilentFrames.fetch_add (1, std::memory_order_acq_rel);
                    else
                        consecutiveSilentFrames.store (0, std::memory_order_release);
                }

                // Feed the converterd buffer to the analyzer
                processor.pushStandaloneAudioBlock (scratch);
            }

            captureClient->ReleaseBuffer (framesAvailable);

            // Check for more packets
            getSizeResult = captureClient->GetNextPacketSize (&packetSize);
            if (FAILED (getSizeResult))
                break;
        }
    }

    audioClient->Stop();
    running.store (false, std::memory_order_release);

    if (comInitialized)
        CoUninitialize();
}

} // namespace lumascope
