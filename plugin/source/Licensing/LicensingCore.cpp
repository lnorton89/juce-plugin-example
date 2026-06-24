#include "LumaScope/Licensing/LicensingCore.h"
#include "LumaScope/Licensing/MachineIdentity.h"
#include <juce_core/juce_core.h>

namespace lumascope {

std::string LicensingCore::getMachineId() const
{
    if (!testMachineId_.empty())
        return testMachineId_;
    return deriveMachineIdentifier();
}

LicensingCore::LicensingCore(std::unique_ptr<TokenVerifier> verifier,
                             std::unique_ptr<LocalEntitlementStore> store)
    : verifier_(std::move(verifier))
    , store_(std::move(store))
{
}

void LicensingCore::loadFromDisk()
{
    auto stored = store_->read();
    if (!stored)
    {
        setStatus(LicenseStatus::not_activated);
        return;
    }

    auto storedJson = juce::JSON::parse(*stored);
    auto* obj = storedJson.getDynamicObject();
    if (!obj)
    {
        setStatus(LicenseStatus::corrupt, {}, "parse_error", "Stored data is not valid JSON");
        return;
    }

    auto& props = obj->getProperties();
    auto tokenVar = props["token"];
    if (tokenVar.isVoid() || !tokenVar.getDynamicObject())
    {
        setStatus(LicenseStatus::corrupt, {}, "parse_error", "Stored data missing token");
        return;
    }

    auto token = parseSignedEntitlement(tokenVar);
    if (!token)
    {
        setStatus(LicenseStatus::corrupt, {}, "parse_error", "Failed to parse stored token");
        return;
    }

    auto result = verifier_->verify(*token);
    if (!result.valid)
    {
        setStatus(LicenseStatus::corrupt, {}, result.errorCode, result.errorMessage);
        return;
    }

    auto currentMachine = getMachineId();
    if (token->payload.machineId != currentMachine)
    {
        setStatus(LicenseStatus::corrupt, {}, "machine_mismatch",
                  "Stored token was issued for a different machine");
        return;
    }

    auto lastVerifiedStr = props["lastVerifiedTime"].toString().toStdString();
    auto lastKnownSystemStr = props["lastKnownSystemTime"].toString().toStdString();

    juce::Time lastVerifiedTime;
    if (!lastVerifiedStr.empty())
        lastVerifiedTime = juce::Time::fromISO8601(lastVerifiedStr);

    if (!lastKnownSystemStr.empty())
        lastKnownSystemTime_ = juce::Time::fromISO8601(lastKnownSystemStr);

    if (lastVerifiedTime.toMilliseconds() == 0)
    {
        setStatus(LicenseStatus::corrupt, {}, "missing_time", "Missing lastVerifiedTime");
        return;
    }

    auto graceInfo = graceModel_.checkOfflineGraceWithRollback(
        lastVerifiedTime, lastKnownSystemTime_, juce::Time::getCurrentTime());

    if (graceInfo.status == GraceInfo::Status::expired)
    {
        setStatus(LicenseStatus::revalidation_required, token->payload.activationId,
                  {}, {}, lastVerifiedStr, graceInfo.remainingDays);
    }
    else if (graceInfo.status == GraceInfo::Status::warning)
    {
        setStatus(LicenseStatus::offline_grace, token->payload.activationId,
                  {}, {}, lastVerifiedStr, graceInfo.remainingDays);
    }
    else
    {
        setStatus(LicenseStatus::activated, token->payload.activationId,
                  {}, {}, lastVerifiedStr, graceInfo.remainingDays);
    }
}

void LicensingCore::handleActivationResponse(const SignedEntitlementToken& token,
                                              const juce::Time& serverTime)
{
    auto result = verifier_->verify(token);
    if (!result.valid)
        return;

    auto currentMachine = getMachineId();
    if (token.payload.machineId != currentMachine)
        return;

    saveEntitlement(token, serverTime);
    setStatus(LicenseStatus::activated, token.payload.activationId,
              {}, {}, serverTime.toISO8601(true).toStdString(), 7);
}

void LicensingCore::handleValidationResponse(const SignedEntitlementToken& token,
                                              const juce::Time& serverTime)
{
    auto result = verifier_->verify(token);
    if (!result.valid)
        return;

    saveEntitlement(token, serverTime);
    setStatus(LicenseStatus::activated, token.payload.activationId,
              {}, {}, serverTime.toISO8601(true).toStdString(), 7);
}

void LicensingCore::handleDeactivationResponse()
{
    clearEntitlement();
    setStatus(LicenseStatus::not_activated);
}

void LicensingCore::handleServerError(const std::string& errorCode,
                                       const std::string& errorMessage)
{
    auto current = state_.readStatus();
    if (current == LicenseStatus::activated
        || current == LicenseStatus::offline_grace
        || current == LicenseStatus::revalidation_required)
    {
        setStatus(LicenseStatus::service_unavailable, detail_.activationId,
                  errorCode, errorMessage, detail_.lastVerifiedTime,
                  detail_.offlineGraceRemainingDays);
    }
}

void LicensingCore::handleAuthoritativeFailure(const std::string& errorCode,
                                                const std::string& errorMessage)
{
    if (errorCode == "license_revoked" || errorCode == "license_expired")
    {
        clearEntitlement();
        setStatus(LicenseStatus::revoked, {}, errorCode, errorMessage);
    }
    else if (errorCode == "activation_not_found" || errorCode == "machine_count_exceeded")
    {
        clearEntitlement();
        setStatus(LicenseStatus::not_activated, {}, errorCode, errorMessage);
    }
    else
    {
        setStatus(LicenseStatus::service_unavailable, detail_.activationId,
                  errorCode, errorMessage, detail_.lastVerifiedTime,
                  detail_.offlineGraceRemainingDays);
    }
}

void LicensingCore::checkGrace()
{
    auto current = state_.readStatus();
    if (current != LicenseStatus::activated
        && current != LicenseStatus::offline_grace
        && current != LicenseStatus::revalidation_required)
        return;

    auto storeContent = store_->read();
    if (!storeContent)
        return;

    auto storedJson = juce::JSON::parse(*storeContent);
    auto* obj = storedJson.getDynamicObject();
    if (!obj) return;

    auto& props = obj->getProperties();
    auto lastVerifiedStr = props["lastVerifiedTime"].toString().toStdString();
    if (lastVerifiedStr.empty()) return;

    auto lastVerifiedTime = juce::Time::fromISO8601(lastVerifiedStr);
    auto lastKnownSystemStr = props["lastKnownSystemTime"].toString().toStdString();
    if (!lastKnownSystemStr.empty())
        lastKnownSystemTime_ = juce::Time::fromISO8601(lastKnownSystemStr);

    auto graceInfo = graceModel_.checkOfflineGraceWithRollback(
        lastVerifiedTime, lastKnownSystemTime_, juce::Time::getCurrentTime());

    if (graceInfo.status == GraceInfo::Status::expired)
    {
        setStatus(LicenseStatus::revalidation_required, detail_.activationId,
                  {}, {}, lastVerifiedStr, graceInfo.remainingDays);
    }
    else if (graceInfo.status == GraceInfo::Status::warning)
    {
        setStatus(LicenseStatus::offline_grace, detail_.activationId,
                  {}, {}, lastVerifiedStr, graceInfo.remainingDays);
    }
}

LicenseDetail LicensingCore::currentDetail() const noexcept
{
    std::lock_guard<std::mutex> lock(detailMutex_);
    return detail_;
}

LicenseStatus LicensingCore::currentStatus() const noexcept
{
    return state_.readStatus();
}

void LicensingCore::setStatus(LicenseStatus newStatus,
                               const std::string& activationId,
                               const std::string& errorCode,
                               const std::string& errorMessage,
                               const std::string& lastVerified,
                               int graceDays)
{
    state_.updateStatus(newStatus);

    std::lock_guard<std::mutex> lock(detailMutex_);
    detail_.status = newStatus;
    detail_.activationId = activationId;
    detail_.errorCode = errorCode;
    detail_.message = errorMessage;
    detail_.lastVerifiedTime = lastVerified;
    detail_.offlineGraceRemainingDays = graceDays;
}

void LicensingCore::saveEntitlement(const SignedEntitlementToken& token,
                                     const juce::Time& serverTime)
{
    auto json = buildStorageJson(token, serverTime);
    store_->write(json);
}

void LicensingCore::clearEntitlement()
{
    store_->clear();
    lastKnownSystemTime_ = {};
}

std::string LicensingCore::buildStorageJson(const SignedEntitlementToken& token,
                                             const juce::Time& serverTime) const
{
    auto root = new juce::DynamicObject();
    root->setProperty("lastVerifiedTime", serverTime.toISO8601(true));
    root->setProperty("lastKnownSystemTime", juce::Time::getCurrentTime().toISO8601(true));

    auto tokenObj = new juce::DynamicObject();
    tokenObj->setProperty("payload", claimsToVar(token.payload));
    tokenObj->setProperty("signature", juce::String(token.signature));
    tokenObj->setProperty("kid", juce::String(token.kid));
    tokenObj->setProperty("algorithm", juce::String(token.algorithm));
    tokenObj->setProperty("canonical", juce::String(token.canonical));
    root->setProperty("token", juce::var(tokenObj));

    return juce::JSON::toString(juce::var(root), false).toStdString();
}

} // namespace lumascope
