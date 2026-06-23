#include "LumaScope/Standalone/SourceModel.h"

#include <juce_core/juce_core.h>

#include <initguid.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys.h>
#include <comdef.h>
#include <comutil.h>

#include <vector>

namespace lumascope
{

// ============================================================================
// Helper: release a COM interface pointer safely.
// ============================================================================
template <typename T>
static void safeRelease (T*& ptr) noexcept
{
    if (ptr != nullptr) { ptr->Release(); ptr = nullptr; }
}

// ============================================================================
// Helper: extract the friendly name from an IMMDevice's property store.
// Returns empty string if the property is unavailable.
// ============================================================================
static juce::String getDeviceFriendlyName (IMMDevice* device) noexcept
{
    if (device == nullptr)
        return {};

    IPropertyStore* props = nullptr;
    HRESULT hr = device->OpenPropertyStore (STGM_READ, &props);
    if (FAILED (hr) || props == nullptr)
        return {};

    PROPVARIANT var;
    PropVariantInit (&var);

    hr = props->GetValue (PKEY_Device_FriendlyName, &var);
    juce::String result;

    if (SUCCEEDED (hr) && var.vt == VT_LPWSTR && var.pwszVal != nullptr)
        result = juce::String (var.pwszVal).substring (0, 256);

    PropVariantClear (&var);
    safeRelease (props);
    return result;
}

// ============================================================================
// Helper: get the endpoint ID string from an IMMDevice.
// ============================================================================
static juce::String getEndpointId (IMMDevice* device) noexcept
{
    if (device == nullptr)
        return {};

    LPWSTR idStr = nullptr;
    HRESULT hr = device->GetId (&idStr);
    if (FAILED (hr) || idStr == nullptr)
        return {};

    juce::String result (idStr);
    CoTaskMemFree (idStr);
    return result;
}

// ============================================================================
// enumerateRenderEndpoints: enumerate all eRender endpoints and return
// them as SourceDescriptor entries for the SystemOutput mode.
//
// This function initializes COM on the calling thread if needed.
// Returns an empty vector on failure (COM not available, no endpoints, etc.).
// ============================================================================
std::vector<SourceDescriptor> enumerateRenderEndpoints() noexcept
{
    std::vector<SourceDescriptor> endpoints;

    // Initialize COM on this thread (safe to call multiple times per thread)
    HRESULT hr = CoInitializeEx (nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED (hr) && hr != RPC_E_CHANGED_MODE)
        return endpoints; // COM not available

    // Create the device enumerator
    IMMDeviceEnumerator* enumerator = nullptr;
    hr = CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr,
                           CLSCTX_ALL, __uuidof (IMMDeviceEnumerator),
                           reinterpret_cast<void**> (&enumerator));
    if (FAILED (hr) || enumerator == nullptr)
    {
        if (SUCCEEDED (hr) || hr == RPC_E_CHANGED_MODE)
            CoUninitialize();
        return endpoints;
    }

    // Enumerate all render endpoints (all states so UI can show inactive ones)
    IMMDeviceCollection* collection = nullptr;
    hr = enumerator->EnumAudioEndpoints (eRender,
                                         DEVICE_STATE_ACTIVE
                                         | DEVICE_STATE_DISABLED
                                         | DEVICE_STATE_NOTPRESENT
                                         | DEVICE_STATE_UNPLUGGED,
                                         &collection);
    if (FAILED (hr) || collection == nullptr)
    {
        safeRelease (enumerator);
        if (SUCCEEDED (hr) || hr == RPC_E_CHANGED_MODE)
            CoUninitialize();
        return endpoints;
    }

    UINT count = 0;
    collection->GetCount (&count);

    for (UINT index = 0; index < count; ++index)
    {
        IMMDevice* device = nullptr;
        hr = collection->Item (index, &device);
        if (FAILED (hr) || device == nullptr)
            continue;

        const auto rawId = getEndpointId (device);
        const auto name = getDeviceFriendlyName (device);

        if (rawId.isNotEmpty())
        {
            SourceDescriptor desc;
            // D-03 / T-03-02-01: Mode-specific prefix discriminator
            desc.id = "wasapi-loopback-" + rawId;
            desc.displayName = name.isNotEmpty() ? name : rawId.substring (0, 256);
            desc.mode = SourceMode::systemOutput;
            endpoints.push_back (std::move (desc));
        }

        safeRelease (device);
    }

    safeRelease (collection);
    safeRelease (enumerator);
    CoUninitialize();

    return endpoints;
}

// ============================================================================
// Helper: build a single render endpoint SourceDescriptor from raw data.
// Used by the controller to construct enumerated entries.
// ============================================================================
SourceDescriptor makeRenderEndpointDescriptor (const juce::String& rawEndpointId,
                                                const juce::String& displayName) noexcept
{
    SourceDescriptor desc;
    desc.id = "wasapi-loopback-" + rawEndpointId;
    desc.displayName = displayName.isNotEmpty() ? displayName : rawEndpointId.substring (0, 256);
    desc.mode = SourceMode::systemOutput;
    return desc;
}

} // namespace lumascope
