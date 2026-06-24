#pragma once
#include <juce_core/juce_core.h>
#include <cstdint>

namespace lumascope {

struct GraceInfo {
    enum class Status : std::uint8_t {
        active,
        warning,
        expired
    };

    Status status = Status::active;
    int remainingDays = 7;
    int elapsedDays = 0;
    bool rollbackDetected = false;
    double remainingSeconds = 0.0;
};

class GraceModel {
public:
    static constexpr double gracePeriodDays = 7.0;
    static constexpr double warningThresholdDays = 1.0;
    static constexpr double rollbackToleranceHours = 1.0;

    GraceInfo checkOfflineGrace(
        const juce::Time& lastVerifiedTime,
        const juce::Time& currentSystemTime) const;

    GraceInfo checkOfflineGraceWithRollback(
        const juce::Time& lastVerifiedTime,
        const juce::Time& lastKnownSystemTime,
        const juce::Time& currentSystemTime) const;

    static int calculateRemainingDays(double elapsedDays) noexcept;
};

} // namespace lumascope
