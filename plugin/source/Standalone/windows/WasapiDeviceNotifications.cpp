#include "LumaScope/Standalone/WasapiDeviceNotifications.h"

#include <juce_core/juce_core.h>

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <initguid.h>
#include <mmdeviceapi.h>

#include <atomic>
#include <cstdint>

namespace lumascope
{

// Flag constants are defined in WasapiDeviceNotifications.h (kFlagEndpointAdded etc.)

// ============================================================================
// Minimal IMMNotificationClient implementation for render endpoint monitoring.
// ============================================================================
class WasapiNotificationClient final : public IMMNotificationClient
{
public:
    WasapiNotificationClient()
        : refCount (1)
    {
    }

    // IUnknown
    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return refCount.fetch_add (1, std::memory_order_relaxed) + 1;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const auto count = refCount.fetch_sub (1, std::memory_order_acq_rel) - 1;
        if (count == 0)
            delete this;
        return 0;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void** obj) override
    {
        if (obj == nullptr)
            return E_POINTER;

        if (riid == __uuidof (IUnknown) || riid == __uuidof (IMMNotificationClient))
        {
            *obj = static_cast<IMMNotificationClient*> (this);
            AddRef();
            return S_OK;
        }

        *obj = nullptr;
        return E_NOINTERFACE;
    }

    // IMMNotificationClient
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged (LPCWSTR deviceId, DWORD newState) override
    {
        juce::ignoreUnused (deviceId, newState);
        flags.fetch_or (kFlagStateChanged, std::memory_order_release);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded (LPCWSTR deviceId) override
    {
        juce::ignoreUnused (deviceId);
        flags.fetch_or (kFlagEndpointAdded, std::memory_order_release);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved (LPCWSTR deviceId) override
    {
        juce::ignoreUnused (deviceId);
        flags.fetch_or (kFlagEndpointRemoved, std::memory_order_release);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged (EDataFlow flow, ERole role, LPCWSTR newDeviceId) override
    {
        juce::ignoreUnused (flow, role, newDeviceId);
        if (flow == eRender)
            flags.fetch_or (kFlagDefaultChanged, std::memory_order_release);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged (LPCWSTR deviceId, const PROPERTYKEY key) override
    {
        juce::ignoreUnused (deviceId, key);
        return S_OK;
    }

    // Query notification flags and clear atomically.
    int consumeFlags() noexcept
    {
        return flags.exchange (0, std::memory_order_acq_rel);
    }

    bool hasNotifications() const noexcept
    {
        return flags.load (std::memory_order_acquire) != 0;
    }

private:
    std::atomic<LONG> refCount;
    std::atomic<int> flags { 0 };
};

// ============================================================================
// Registration state: wraps the client and enumerator in one heap struct so
// the caller only sees an opaque void* handle.
// ============================================================================
struct NotificationHandle
{
    WasapiNotificationClient* client = nullptr;
    IMMDeviceEnumerator* enumerator = nullptr;
};

void* registerEndpointNotifications() noexcept
{
    HRESULT hr = CoInitializeEx (nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED (hr) && hr != RPC_E_CHANGED_MODE)
        return nullptr;

    auto* handle = new (std::nothrow) NotificationHandle();
    if (handle == nullptr)
    {
        CoUninitialize();
        return nullptr;
    }

    // Create device enumerator
    IMMDeviceEnumerator* enumerator = nullptr;
    hr = CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr,
                           CLSCTX_ALL, __uuidof (IMMDeviceEnumerator),
                           reinterpret_cast<void**> (&enumerator));
    if (FAILED (hr) || enumerator == nullptr)
    {
        delete handle;
        CoUninitialize();
        return nullptr;
    }
    handle->enumerator = enumerator;

    // Create notification client
    auto* client = new WasapiNotificationClient();
    client->AddRef(); // Enumerator holds a reference
    handle->client = client;

    // Register
    hr = enumerator->RegisterEndpointNotificationCallback (client);
    if (FAILED (hr))
    {
        client->Release();
        enumerator->Release();
        delete handle;
        CoUninitialize();
        return nullptr;
    }

    return static_cast<void*> (handle);
}

void unregisterEndpointNotifications (void* handlePtr) noexcept
{
    if (handlePtr == nullptr)
        return;

    auto* handle = static_cast<NotificationHandle*> (handlePtr);

    if (handle->enumerator != nullptr && handle->client != nullptr)
        handle->enumerator->UnregisterEndpointNotificationCallback (handle->client);

    if (handle->client != nullptr)
        handle->client->Release();

    if (handle->enumerator != nullptr)
        handle->enumerator->Release();

    delete handle;
    CoUninitialize();
}

int consumeEndpointNotificationFlags (void* handlePtr) noexcept
{
    if (handlePtr == nullptr)
        return 0;

    auto* handle = static_cast<NotificationHandle*> (handlePtr);
    return handle->client->consumeFlags();
}

} // namespace lumascope
