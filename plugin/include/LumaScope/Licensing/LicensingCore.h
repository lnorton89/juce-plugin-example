#pragma once
#include "LicensingState.h"
#include "LocalEntitlementStore.h"
#include "GraceModel.h"
#include "EntitlementToken.h"
#include "TokenVerifier.h"
#include <juce_core/juce_core.h>
#include <memory>
#include <mutex>

namespace lumascope {

struct LicenseDetail {
    LicenseStatus status = LicenseStatus::not_activated;
    std::string activationId;
    std::string lastVerifiedTime;
    int offlineGraceRemainingDays = -1;
    std::string errorCode;
    std::string message;
};

class LicensingCore {
public:
    explicit LicensingCore(std::unique_ptr<TokenVerifier> verifier,
                           std::unique_ptr<LocalEntitlementStore> store);

    void loadFromDisk();

    void handleActivationResponse(const SignedEntitlementToken& token,
                                   const juce::Time& serverTime);

    void handleValidationResponse(const SignedEntitlementToken& token,
                                   const juce::Time& serverTime);

    void handleDeactivationResponse();

    void handleServerError(const std::string& errorCode,
                           const std::string& errorMessage);

    void handleAuthoritativeFailure(const std::string& errorCode,
                                    const std::string& errorMessage);

    void checkGrace();

    LicensingState& statusAtom() noexcept { return state_; }
    const LicensingState& statusAtom() const noexcept { return state_; }

    LicenseDetail currentDetail() const noexcept;
    LicenseStatus currentStatus() const noexcept;

    LocalEntitlementStore& store() noexcept { return *store_; }

    juce::Time lastKnownSystemTime() const noexcept { return lastKnownSystemTime_; }

    void setMachineIdForTesting(const std::string& id) { testMachineId_ = id; }

private:
    std::string getMachineId() const;
    std::string testMachineId_;
    LicensingState state_;
    GraceModel graceModel_;
    std::unique_ptr<TokenVerifier> verifier_;
    std::unique_ptr<LocalEntitlementStore> store_;

    mutable std::mutex detailMutex_;
    LicenseDetail detail_;

    juce::Time lastKnownSystemTime_;

    void setStatus(LicenseStatus newStatus,
                   const std::string& activationId = {},
                   const std::string& errorCode = {},
                   const std::string& errorMessage = {},
                   const std::string& lastVerified = {},
                   int graceDays = -1);

    void saveEntitlement(const SignedEntitlementToken& token,
                         const juce::Time& serverTime);
    void clearEntitlement();
    std::string buildStorageJson(const SignedEntitlementToken& token,
                                 const juce::Time& serverTime) const;
};

} // namespace lumascope
