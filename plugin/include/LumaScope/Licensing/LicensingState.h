#pragma once
#include <atomic>
#include <cstdint>
#include <juce_core/juce_core.h>

namespace lumascope {

enum class LicenseStatus : std::uint8_t {
    uninitialized = 0,
    not_activated,
    activating,
    activated,
    offline_grace,
    revalidation_required,
    revoked,
    corrupt,
    service_unavailable,
    deactivating
};

const char* toString(LicenseStatus status) noexcept;
bool isTransitionState(LicenseStatus status) noexcept;
bool isErrorState(LicenseStatus status) noexcept;

class LicensingState {
public:
    LicensingState() = default;
    LicensingState(const LicensingState&) = delete;
    LicensingState& operator=(const LicensingState&) = delete;

    void updateStatus(LicenseStatus newStatus) noexcept {
        status_.store(newStatus, std::memory_order_release);
        sequence_.fetch_add(1, std::memory_order_release);
    }

    LicenseStatus readStatus() const noexcept {
        return status_.load(std::memory_order_acquire);
    }

    bool hasChanged(std::uint32_t& lastSeen) const noexcept {
        const auto current = sequence_.load(std::memory_order_acquire);
        if (current == lastSeen) return false;
        lastSeen = current;
        return true;
    }

    std::uint32_t currentSequence() const noexcept {
        return sequence_.load(std::memory_order_acquire);
    }

private:
    std::atomic<LicenseStatus> status_{LicenseStatus::uninitialized};
    std::atomic<std::uint32_t> sequence_{0};
};

} // namespace lumascope
