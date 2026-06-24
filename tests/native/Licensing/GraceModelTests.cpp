#include "LumaScope/Licensing/GraceModel.h"
#include <juce_core/juce_core.h>
#include <iostream>

int runGraceModelTests()
{
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    lumascope::GraceModel model;

    // Helper: create a time from now and offset in days
    const auto now = juce::Time::getCurrentTime();

    // testGraceActive_ZeroElapsed
    {
        auto info = model.checkOfflineGrace(now, now);
        expect(info.status == lumascope::GraceInfo::Status::active,
               "zero elapsed -> active");
        expect(info.remainingDays == 7, "zero elapsed -> 7 days remaining");
    }

    // testGraceActive_FiveDaysElapsed
    {
        auto past = now - juce::RelativeTime::days(5);
        auto info = model.checkOfflineGrace(past, now);
        expect(info.status == lumascope::GraceInfo::Status::active,
               "5 days elapsed -> active");
        expect(info.remainingDays == 2, "5 days elapsed -> 2 remaining");
        expect(info.elapsedDays == 5, "5 days elapsed reported");
    }

    // testGraceWarning_SixDaysElapsed
    {
        auto past = now - juce::RelativeTime::days(6);
        auto info = model.checkOfflineGrace(past, now);
        expect(info.status == lumascope::GraceInfo::Status::warning,
               "6 days elapsed -> warning");
        expect(info.remainingDays == 1, "6 days elapsed -> 1 remaining");
    }

    // testGraceWarning_TwentyThreeHoursRemaining
    {
        auto past = now - juce::RelativeTime::days(6) - juce::RelativeTime::hours(1);
        auto info = model.checkOfflineGrace(past, now);
        expect(info.status == lumascope::GraceInfo::Status::warning,
               "6 days 1 hour elapsed -> warning");
        expect(info.remainingDays == 1, "6 days 1 hour -> 1 day remaining (generous rounding)");
    }

    // testGraceExpired_SevenDaysElapsed
    {
        auto past = now - juce::RelativeTime::days(7);
        auto info = model.checkOfflineGrace(past, now);
        expect(info.status == lumascope::GraceInfo::Status::expired,
               "7 days elapsed -> expired");
    }

    // testGraceExpired_EightDaysElapsed
    {
        auto past = now - juce::RelativeTime::days(8);
        auto info = model.checkOfflineGrace(past, now);
        expect(info.status == lumascope::GraceInfo::Status::expired,
               "8 days elapsed -> expired");
    }

    // testRollbackDetection_ClockWentBackward
    {
        auto lastVerified = now - juce::RelativeTime::days(6);
        auto lastKnownSystem = now - juce::RelativeTime::days(3);
        auto rolledBackNow = now - juce::RelativeTime::days(5);
        auto info = model.checkOfflineGraceWithRollback(
            lastVerified, lastKnownSystem, rolledBackNow);
        expect(info.rollbackDetected, "clock rollback detected");
        expect(info.status == lumascope::GraceInfo::Status::active,
               "rollback -> uses lastKnownSystemTime, 3 days elapsed since verified -> active");
        expect(info.remainingDays == 4, "rollback -> 4 days remaining");
    }

    // testRollbackDetection_ClockWentBackwardByLessThanTolerance
    {
        auto lastVerified = now - juce::RelativeTime::days(3);
        auto lastKnownSystem = now;
        auto slightlyBack = now - juce::RelativeTime::minutes(30);
        auto info = model.checkOfflineGraceWithRollback(
            lastVerified, lastKnownSystem, slightlyBack);
        expect(!info.rollbackDetected,
               "30 min backward -> no rollback detection");
        expect(info.status == lumascope::GraceInfo::Status::active,
               "3 days elapsed -> active");
    }

    // testCalculateRemainingDays
    {
        expect(lumascope::GraceModel::calculateRemainingDays(0.0) == 7,
               "0 elapsed -> 7 remaining");
        expect(lumascope::GraceModel::calculateRemainingDays(5.0) == 2,
               "5 elapsed -> 2 remaining");
        expect(lumascope::GraceModel::calculateRemainingDays(6.999999) == 1,
               "6.999999 elapsed -> 1 remaining");
        expect(lumascope::GraceModel::calculateRemainingDays(7.0) == 0,
               "7 elapsed -> 0 remaining");
        expect(lumascope::GraceModel::calculateRemainingDays(8.0) == 0,
               "8 elapsed -> 0 remaining (clamped)");
    }

    if (failures > 0)
        std::cerr << "runGraceModelTests: " << failures << " failure(s)\n";
    return failures;
}
