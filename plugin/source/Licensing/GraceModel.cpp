#include "LumaScope/Licensing/GraceModel.h"
#include <algorithm>
#include <cmath>

namespace lumascope {

GraceInfo GraceModel::checkOfflineGrace(
    const juce::Time& lastVerifiedTime,
    const juce::Time& currentSystemTime) const
{
    GraceInfo info;

    const auto elapsed = currentSystemTime - lastVerifiedTime;
    const double elapsedDays = elapsed.inDays();
    info.elapsedDays = static_cast<int>(std::floor(elapsedDays));

    const double remaining = gracePeriodDays - elapsedDays;
    info.remainingSeconds = remaining * 86400.0;

    if (remaining <= 0.0)
    {
        info.status = GraceInfo::Status::expired;
        info.remainingDays = 0;
    }
    else if (remaining <= warningThresholdDays)
    {
        info.status = GraceInfo::Status::warning;
        info.remainingDays = calculateRemainingDays(elapsedDays);
    }
    else
    {
        info.status = GraceInfo::Status::active;
        info.remainingDays = calculateRemainingDays(elapsedDays);
    }

    return info;
}

GraceInfo GraceModel::checkOfflineGraceWithRollback(
    const juce::Time& lastVerifiedTime,
    const juce::Time& lastKnownSystemTime,
    const juce::Time& currentSystemTime) const
{
    const double rollbackToleranceSeconds = rollbackToleranceHours * 3600.0;
    const double backwardJump = (currentSystemTime - lastKnownSystemTime).inSeconds();

    if (backwardJump < -rollbackToleranceSeconds)
    {
        GraceInfo info;
        info.rollbackDetected = true;

        const auto elapsed = lastKnownSystemTime - lastVerifiedTime;
        const double elapsedDays = elapsed.inDays();
        info.elapsedDays = static_cast<int>(std::floor(elapsedDays));

        const double remaining = gracePeriodDays - elapsedDays;
        info.remainingSeconds = remaining * 86400.0;

        if (remaining <= 0.0)
        {
            info.status = GraceInfo::Status::expired;
            info.remainingDays = 0;
        }
        else if (remaining <= warningThresholdDays)
        {
            info.status = GraceInfo::Status::warning;
            info.remainingDays = calculateRemainingDays(elapsedDays);
        }
        else
        {
            info.status = GraceInfo::Status::active;
            info.remainingDays = calculateRemainingDays(elapsedDays);
        }

        return info;
    }

    return checkOfflineGrace(lastVerifiedTime, currentSystemTime);
}

int GraceModel::calculateRemainingDays(double elapsedDays) noexcept
{
    const double remaining = gracePeriodDays - elapsedDays;
    return std::max(0, static_cast<int>(std::floor(remaining + 0.999999)));
}

} // namespace lumascope
