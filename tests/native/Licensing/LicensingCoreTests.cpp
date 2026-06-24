#include "LumaScope/Licensing/LicensingCore.h"
#include "LumaScope/Licensing/PublicKeyRing.h"
#include "LumaScope/Licensing/TokenVerifier.h"
#include "LumaScope/Licensing/MachineIdentity.h"
#include "LumaScope/Licensing/EntitlementToken.h"
#include "LumaScope/Licensing/BuiltinPublicKeyRing.h"
#include <juce_core/juce_core.h>
#include <iostream>
#include <memory>
#include <thread>

static juce::File createTempDir()
{
    auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto dir = tmp.getNonexistentChildFile("lms_liccore_test_", "", 0);
    dir.createDirectory();
    return dir;
}

// Build a valid token for testing from fixture data
static lumascope::SignedEntitlementToken loadFixtureToken()
{
    juce::File f(LUMASCOPE_FIXTURE_ENTITLEMENT_V1);
    auto json = juce::JSON::parse(f);
    auto parsed = lumascope::parseSignedEntitlement(json);
    return parsed.value_or(lumascope::SignedEntitlementToken{});
}

static std::unique_ptr<lumascope::TokenVerifier> createTestVerifier()
{
    auto parsed = lumascope::PublicKeyRing::parse(
        std::string(lumascope::builtinTestKeyRing));
    return std::make_unique<lumascope::TokenVerifier>(std::move(parsed.value()));
}

int runLicensingCoreTests()
{
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    // testInitialState (before loadFromDisk)
    {
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        expect(core.currentStatus() == lumascope::LicenseStatus::uninitialized,
               "initial status is uninitialized");
    }

    // testLoadFromDiskNoEntitlement
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.loadFromDisk();
        expect(core.currentStatus() == lumascope::LicenseStatus::not_activated,
               "no stored entitlement -> not_activated");
        dir.deleteRecursively();
    }

    // testActivationSuccess
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");
        auto token = loadFixtureToken();

        core.handleActivationResponse(token, juce::Time::getCurrentTime());
        expect(core.currentStatus() == lumascope::LicenseStatus::activated,
               "handleActivationResponse -> activated");
        expect(core.store().exists(), "entitlement file exists after activation");

        auto detail = core.currentDetail();
        expect(detail.offlineGraceRemainingDays == 7,
               "offlineGraceRemainingDays is 7 after activation");

        dir.deleteRecursively();
    }

    // testActivationInvalidToken (fails because canonical doesn't match signature)
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");
        auto token = loadFixtureToken();
        token.payload.machineId = "wrong_machine";
        token.canonical = lumascope::canonicalJson(
            lumascope::claimsToVar(token.payload));

        core.handleActivationResponse(token, juce::Time::getCurrentTime());
        expect(core.currentStatus() == lumascope::LicenseStatus::uninitialized,
               "invalid token does not change state");

        dir.deleteRecursively();
    }

    // testDeactivationSuccess
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");
        auto token = loadFixtureToken();

        core.handleActivationResponse(token, juce::Time::getCurrentTime());
        expect(core.currentStatus() == lumascope::LicenseStatus::activated,
               "activated before deactivation");

        core.handleDeactivationResponse();
        expect(core.currentStatus() == lumascope::LicenseStatus::not_activated,
               "deactivated -> not_activated");
        expect(!core.store().exists(), "entitlement cleared after deactivation");

        dir.deleteRecursively();
    }

    // testDeactivationWithoutActivation
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));

        core.handleDeactivationResponse();
        expect(core.currentStatus() == lumascope::LicenseStatus::not_activated,
               "deactivation without activation -> not_activated");

        dir.deleteRecursively();
    }

    // testServerErrorPreservesEntitlement
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");
        auto token = loadFixtureToken();

        core.handleActivationResponse(token, juce::Time::getCurrentTime());
        expect(core.currentStatus() == lumascope::LicenseStatus::activated,
               "activated before server error");

        core.handleServerError("network_error", "Connection refused");
        expect(core.currentStatus() == lumascope::LicenseStatus::service_unavailable,
               "server error -> service_unavailable");
        expect(core.store().exists(), "entitlement preserved after server error");

        dir.deleteRecursively();
    }

    // testAuthoritativeFailureRevoke
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");
        auto token = loadFixtureToken();

        core.handleActivationResponse(token, juce::Time::getCurrentTime());
        expect(core.currentStatus() == lumascope::LicenseStatus::activated,
               "activated before revoke");

        core.handleAuthoritativeFailure("license_revoked", "License revoked");
        expect(core.currentStatus() == lumascope::LicenseStatus::revoked,
               "authoritative failure -> revoked");
        expect(!core.store().exists(), "entitlement cleared after revoke");

        dir.deleteRecursively();
    }

    // testGraceActive
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");
        auto token = loadFixtureToken();

        core.handleActivationResponse(token, juce::Time::getCurrentTime());
        core.checkGrace();
        expect(core.currentStatus() == lumascope::LicenseStatus::activated,
               "checkGrace immediately after activation -> still activated");

        dir.deleteRecursively();
    }

    // testAtomicStatusThreadSafe
    {
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        auto& state = core.statusAtom();

        state.updateStatus(lumascope::LicenseStatus::activated);
        std::thread reader([&state, &expect]() {
            auto s = state.readStatus();
            expect(s == lumascope::LicenseStatus::activated,
                   "atomic status readable from another thread");
        });
        reader.join();
    }

    // testHasChangedTracking
    {
        lumascope::LicensingState state;
        juce::uint32 lastSeen = 0;

        expect(!state.hasChanged(lastSeen), "no change initially");
        state.updateStatus(lumascope::LicenseStatus::activated);
        expect(state.hasChanged(lastSeen), "hasChanged true after update");
        expect(!state.hasChanged(lastSeen), "hasChanged false on same sequence");
    }

    // testStateStringMapping
    {
        using LS = lumascope::LicenseStatus;
        expect(std::string(lumascope::toString(LS::uninitialized)) == "uninitialized", "");
        expect(std::string(lumascope::toString(LS::not_activated)) == "not_activated", "");
        expect(std::string(lumascope::toString(LS::activating)) == "activating", "");
        expect(std::string(lumascope::toString(LS::activated)) == "activated", "");
        expect(std::string(lumascope::toString(LS::offline_grace)) == "offline_grace", "");
        expect(std::string(lumascope::toString(LS::revalidation_required)) == "revalidation_required", "");
        expect(std::string(lumascope::toString(LS::revoked)) == "revoked", "");
        expect(std::string(lumascope::toString(LS::corrupt)) == "corrupt", "");
        expect(std::string(lumascope::toString(LS::service_unavailable)) == "service_unavailable", "");
        expect(std::string(lumascope::toString(LS::deactivating)) == "deactivating", "");
    }

    // testTransitionStates
    {
        using LS = lumascope::LicenseStatus;
        expect(lumascope::isTransitionState(LS::activating), "activating is transition");
        expect(lumascope::isTransitionState(LS::deactivating), "deactivating is transition");
        expect(!lumascope::isTransitionState(LS::activated), "activated not transition");
        expect(!lumascope::isTransitionState(LS::revoked), "revoked not transition");
    }

    // Helper lambda: wrap fixture token into store-compatible JSON with timestamps
    auto wrapWithTimestamps = [](const juce::var& fixtureJson,
                                 const juce::Time& verifiedTime) -> juce::var
    {
        auto root = new juce::DynamicObject();
        root->setProperty("lastVerifiedTime", verifiedTime.toISO8601(true));
        root->setProperty("lastKnownSystemTime", verifiedTime.toISO8601(true));
        root->setProperty("token", fixtureJson);
        return juce::var(root);
    };

    // testLoadFromDiskWithFixture
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);

        auto fixtureFile = juce::File(LUMASCOPE_FIXTURE_ENTITLEMENT_V1);
        auto fixtureJson = juce::JSON::parse(fixtureFile);
        auto now = juce::Time::getCurrentTime();
        auto wrapped = wrapWithTimestamps(fixtureJson, now);
        auto storeContent = juce::JSON::toString(wrapped, false).toStdString();
        store->write(storeContent);

        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");
        core.loadFromDisk();
        expect(core.currentStatus() == lumascope::LicenseStatus::activated,
               "loadFromDisk with valid fixture -> activated");
        auto detail = core.currentDetail();
        expect(detail.activationId == "act_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G",
               "activationId matches fixture");

        dir.deleteRecursively();
    }

    // testLoadFromDiskWithTamperedToken
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);

        auto fixtureFile = juce::File(LUMASCOPE_FIXTURE_ENTITLEMENT_V1);
        auto fixtureJson = juce::JSON::parse(fixtureFile);
        // Tamper the signature by appending 'X'
        auto* obj = fixtureJson.getDynamicObject();
        if (obj)
        {
            auto sig = obj->getProperty("signature").toString().toStdString();
            obj->setProperty("signature", juce::String(sig + "X"));
        }
        auto now = juce::Time::getCurrentTime();
        auto wrapped = wrapWithTimestamps(fixtureJson, now);
        auto storeContent = juce::JSON::toString(wrapped, false).toStdString();
        store->write(storeContent);

        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");
        core.loadFromDisk();
        expect(core.currentStatus() == lumascope::LicenseStatus::corrupt,
               "tampered signature -> corrupt");

        dir.deleteRecursively();
    }

    // testDeactivateThenLoad
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");
        auto token = loadFixtureToken();

        core.handleActivationResponse(token, juce::Time::getCurrentTime());
        expect(core.currentStatus() == lumascope::LicenseStatus::activated,
               "activated before deactivation");
        expect(core.store().exists(), "entitlement exists after activation");

        // Simulate deactivation
        core.handleDeactivationResponse();
        expect(core.currentStatus() == lumascope::LicenseStatus::not_activated,
               "deactivated -> not_activated");
        expect(!core.store().exists(), "entitlement cleared after deactivation");

        // Simulate restart: create fresh core with same store path
        auto store2 = std::make_unique<lumascope::LocalEntitlementStore>();
        store2->setBasePathForTesting(dir);
        auto core2 = lumascope::LicensingCore(createTestVerifier(), std::move(store2));
        core2.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");
        core2.loadFromDisk();
        expect(core2.currentStatus() == lumascope::LicenseStatus::not_activated,
               "restart after deactivation -> not_activated");

        dir.deleteRecursively();
    }

    // testGraceExpiryAfterLoad
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);

        // Create a token with a lastVerifiedTime 8 days ago (grace = 7 days)
        auto eightDaysAgo = juce::Time::getCurrentTime() - juce::RelativeTime::days(8);

        auto fixtureFile = juce::File(LUMASCOPE_FIXTURE_ENTITLEMENT_V1);
        auto fixtureJson = juce::JSON::parse(fixtureFile);
        auto wrapped = wrapWithTimestamps(fixtureJson, eightDaysAgo);
        auto storeContent = juce::JSON::toString(wrapped, false).toStdString();
        store->write(storeContent);

        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");
        core.loadFromDisk();
        // loadFromDisk should activate (valid signature, matching machine)
        // checkGrace should detect 8 days since lastVerified -> expired grace
        core.checkGrace();
        auto status = core.currentStatus();
        expect(status == lumascope::LicenseStatus::revalidation_required,
               "grace expiry after 8 days -> revalidation_required");

        dir.deleteRecursively();
    }

    // testHandleValidationSuccessExtendsGrace
    {
        auto dir = createTempDir();
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        store->setBasePathForTesting(dir);
        auto core = lumascope::LicensingCore(createTestVerifier(), std::move(store));
        core.setMachineIdForTesting("machine_derived_01JZ9Z4X4Y6Q8A9B0C1D2E3F4G");

        // Initial activation
        auto token = loadFixtureToken();
        auto now = juce::Time::getCurrentTime();
        core.handleActivationResponse(token, now);
        core.checkGrace();
        expect(core.currentStatus() == lumascope::LicenseStatus::activated,
               "initial activation -> activated");

        // Simulate 6 days passing and re-validation
        auto sixDaysLater = now + juce::RelativeTime::days(6);
        core.handleValidationResponse(token, sixDaysLater);
        core.checkGrace();
        expect(core.currentStatus() == lumascope::LicenseStatus::activated,
               "validation after 6 days extends grace -> still activated");

        dir.deleteRecursively();
    }

    // testErrorStates
    {
        using LS = lumascope::LicenseStatus;
        expect(lumascope::isErrorState(LS::revoked), "revoked is error");
        expect(lumascope::isErrorState(LS::corrupt), "corrupt is error");
        expect(!lumascope::isErrorState(LS::activated), "activated not error");
        expect(!lumascope::isErrorState(LS::not_activated), "not_activated not error");
    }

    if (failures > 0)
        std::cerr << "runLicensingCoreTests: " << failures << " failure(s)\n";
    return failures;
}
