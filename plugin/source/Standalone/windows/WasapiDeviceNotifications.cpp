#include <juce_core/juce_core.h>

#include <initguid.h>
#include <mmdeviceapi.h>

#include <atomic>

namespace lumascope
{

// Forward declarations used by tests and controller
static constexpr int kFlagEndpointAdded    = 1;
static constexpr int kFlagEndpointRemoved  = 2;
static constexpr int kFlagDefaultChanged   = 4;
static constexpr int kFlagStateChanged     = 8;

// ============================================================================
// Minimal IMMNotificationClient implementation for render endpoint monitoring.
//
// Signals the controller when render endpoints change so the source list
// can be refreshed and the selected source can be invalidated if it is
// the endpoint that was removed or changed.
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

    // Query notification flags and clear.
    int consumeFlags() noexcept
    {
        return flags.exchange (0, std::memory_order_acq_rel);
    }

    bool hasNotifications() const noexcept
    {
        return flags.load (std::memory_order_acquire) != 0;
    }

private:
    std::atomic<int> refCount;
    std::atomic<int> flags { 0 };
};

// In a future task, the notification client will be registered with the
// IMMDeviceEnumerator and connected to the controller to handle D-05
// (endpoint removal stops capture), D-06 (no fallback), and D-07 (bounded
// same-source retry).

} // namespace lumascope
