#include "LumaScope/PluginProcessor.h"
#include "LumaScope/Licensing/BuiltinPublicKeyRing.h"
#include "LumaScope/Licensing/EntitlementToken.h"
#include "LumaScope/Licensing/LocalEntitlementStore.h"
#include "LumaScope/Licensing/MachineIdentity.h"
#include "LumaScope/Licensing/PublicKeyRing.h"
#include "LumaScope/Licensing/TokenVerifier.h"

#ifndef LUMASCOPE_NATIVE_TESTS
 #include "LumaScope/PluginEditor.h"
#endif

LumaScopeAudioProcessor::LumaScopeAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
   #if JucePlugin_Build_Standalone
    standaloneSourceController = lumascope::createStandaloneSourceController (*this);
   #endif

    auto keyRing = lumascope::PublicKeyRing::parse(std::string(lumascope::builtinTestKeyRing));
    if (keyRing)
    {
        auto verifier = std::make_unique<lumascope::TokenVerifier>(std::move(*keyRing));
        auto store = std::make_unique<lumascope::LocalEntitlementStore>();
        licensingCore = std::make_unique<lumascope::LicensingCore>(
            std::move(verifier), std::move(store));
        activationClient = std::make_unique<lumascope::ActivationClient>(
            juce::String(LUMASCOPE_ACTIVATION_BASE_URL));
    }
}

LumaScopeAudioProcessor::~LumaScopeAudioProcessor() = default;

void LumaScopeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    analyzer.prepare (sampleRate);
    snapshotMailbox.clear();
    lastPublishedAnalyzerSequence = 0;
}

void LumaScopeAudioProcessor::releaseResources()
{
    snapshotMailbox.clear();
    lastPublishedAnalyzerSequence = 0;
}

bool LumaScopeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void LumaScopeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);

    if (buffer.getNumChannels() <= 0 || buffer.getNumSamples() <= 0)
        return;

    analyzer.pushAudioBlock (buffer);

    lumascope::SpectrumSnapshot snapshot;
    if (analyzer.copyLatestSnapshot (snapshot) && snapshot.sequence != lastPublishedAnalyzerSequence)
    {
        if (snapshotMailbox.publish (snapshot))
            lastPublishedAnalyzerSequence = snapshot.sequence;
    }
}

bool LumaScopeAudioProcessor::readLatestSpectrumSnapshot (lumascope::SpectrumSnapshot& snapshot,
                                                          std::uint32_t& lastSeenSequence) const noexcept
{
    return snapshotMailbox.readLatest (snapshot, lastSeenSequence);
}

void LumaScopeAudioProcessor::activateLicense(const std::string& licenseKey)
{
    if (!licensingCore || !activationClient)
        return;

    auto machineId = lumascope::deriveMachineIdentifier();
    storedLicenseKey_ = licenseKey;

    licensingCore->statusAtom().updateStatus(lumascope::LicenseStatus::activating);

    activationClient->activate({licenseKey, machineId},
        [this, licenseKey](const lumascope::ActivationResult& result) {
            handleActivationResult(result, licenseKey, false);
        });
}

void LumaScopeAudioProcessor::deactivateLicense()
{
    if (!licensingCore || !activationClient || storedLicenseKey_.empty())
        return;

    auto machineId = lumascope::deriveMachineIdentifier();

    licensingCore->statusAtom().updateStatus(lumascope::LicenseStatus::deactivating);

    activationClient->deactivate({storedLicenseKey_, machineId},
        [this](const lumascope::ActivationResult& result) {
            handleActivationResult(result, storedLicenseKey_, true);
        });
}

void LumaScopeAudioProcessor::validateLicense()
{
    if (!licensingCore || !activationClient || storedLicenseKey_.empty())
        return;

    auto machineId = lumascope::deriveMachineIdentifier();

    activationClient->validate({storedLicenseKey_, machineId},
        [this](const lumascope::ActivationResult& result) {
            handleActivationResult(result, storedLicenseKey_, false);
        });
}

void LumaScopeAudioProcessor::handleActivationResult(const lumascope::ActivationResult& result,
                                                      const std::string& licenseKey,
                                                      bool isDeactivation)
{
    juce::ignoreUnused(licenseKey);

    if (!licensingCore)
        return;

    if (result.type == lumascope::ActivationResult::Type::networkError)
    {
        if (isDeactivation)
        {
            licensingCore->handleServerError("network_error", result.errorMessage);
        }
        else
        {
            licensingCore->handleServerError("network_error", result.errorMessage);
        }
        return;
    }

    if (result.statusCode >= 500)
    {
        licensingCore->handleServerError("server_error", result.errorMessage);
        return;
    }

    if (result.statusCode >= 400 && result.statusCode < 500)
    {
        if (result.errorCode == "license_revoked" || result.errorCode == "license_expired")
        {
            licensingCore->handleAuthoritativeFailure(result.errorCode, result.errorMessage);
        }
        else if (isDeactivation && result.statusCode != 200)
        {
            licensingCore->handleServerError(result.errorCode, result.errorMessage);
        }
        else if (!isDeactivation)
        {
            licensingCore->handleServerError(result.errorCode, result.errorMessage);
        }
        return;
    }

    if (result.type == lumascope::ActivationResult::Type::success)
    {
        auto json = juce::JSON::parse(juce::String(result.responseBody));
        if (auto* obj = json.getDynamicObject())
        {
            auto token = lumascope::parseSignedEntitlement(json);
            if (token)
            {
                if (isDeactivation)
                {
                    licensingCore->handleDeactivationResponse();
                }
                else
                {
                    licensingCore->handleActivationResponse(*token, juce::Time::getCurrentTime());
                }
            }
        }
    }
}

void LumaScopeAudioProcessor::pushStandaloneAudioBlock (const juce::AudioBuffer<float>& block) noexcept
{
    // Reuses the same analyzer ingress contract used by processBlock.
    // Must be called from a non-audio-thread context, or from a JUCE capture
    // callback that is already real-time safe.
    if (block.getNumChannels() <= 0 || block.getNumSamples() <= 0)
        return;

    analyzer.pushAudioBlock (block);

    lumascope::SpectrumSnapshot snapshot;
    if (analyzer.copyLatestSnapshot (snapshot) && snapshot.sequence != lastPublishedAnalyzerSequence)
    {
        if (snapshotMailbox.publish (snapshot))
            lastPublishedAnalyzerSequence = snapshot.sequence;
    }
}

juce::AudioProcessorEditor* LumaScopeAudioProcessor::createEditor()
{
   #ifdef LUMASCOPE_NATIVE_TESTS
    return nullptr;
   #else
    return new LumaScopeAudioProcessorEditor (*this);
   #endif
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LumaScopeAudioProcessor();
}
