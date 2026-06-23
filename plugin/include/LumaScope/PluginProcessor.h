#pragma once

#include "LumaScope/Analyzer/SpectrumAnalyzer.h"
#include "LumaScope/Analyzer/SpectrumSnapshot.h"
#include "LumaScope/SnapshotMailbox.h"

#include <juce_audio_processors/juce_audio_processors.h>

class LumaScopeAudioProcessor final : public juce::AudioProcessor
{
public:
    LumaScopeAudioProcessor();

    void prepareToPlay (double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
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

private:
    lumascope::SpectrumAnalyzer analyzer;
    lumascope::SnapshotMailbox snapshotMailbox;
    std::uint32_t lastPublishedAnalyzerSequence = 0;
};
