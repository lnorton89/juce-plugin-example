#pragma once

#include "LumaScope/Analyzer/SpectrumAnalyzer.h"
#include "LumaScope/Analyzer/SpectrumSnapshot.h"
#include "LumaScope/SnapshotMailbox.h"
#include "LumaScope/Standalone/StandaloneSourceController.h"

#include <juce_audio_processors/juce_audio_processors.h>

class LumaScopeAudioProcessor final : public juce::AudioProcessor
{
public:
    LumaScopeAudioProcessor();
    ~LumaScopeAudioProcessor() override;

    void prepareToPlay (double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void pushStandaloneAudioBlock (const juce::AudioBuffer<float>& block) noexcept;
    bool readLatestSpectrumSnapshot (lumascope::SpectrumSnapshot&, std::uint32_t&) const noexcept;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    lumascope::StandaloneSourceController* getStandaloneSourceController() const noexcept { return standaloneSourceController.get(); }

private:
    lumascope::SpectrumAnalyzer analyzer;
    lumascope::SnapshotMailbox snapshotMailbox;
    std::uint32_t lastPublishedAnalyzerSequence = 0;
    std::unique_ptr<lumascope::StandaloneSourceController> standaloneSourceController;
};
