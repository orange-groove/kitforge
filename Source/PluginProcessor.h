#pragma once

#include <JuceHeader.h>
#include "Models/KitModel.h"
#include "Engine/DrumSamplerEngine.h"
#include "Midi/MidiLearnManager.h"
#include "Core/KitForgeServices.h"

class KitForgeAudioProcessor final : public juce::AudioProcessor
{
public:
    KitForgeAudioProcessor();
    ~KitForgeAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    KitModel& getKitModel() { return kitModel; }
    const KitModel& getKitModel() const { return kitModel; }

    KitForgeServices& getServices() { return services; }
    const KitForgeServices& getServices() const { return services; }

    MidiLearnManager& getMidiLearnManager() { return midiLearnManager; }
    DrumSamplerEngine& getSamplerEngine() { return samplerEngine; }

    void triggerPiece (const juce::String& pieceId, float velocity = 127.0f);
    void triggerArticulation (const juce::String& pieceId, const juce::String& articulationId,
                              float velocity = 127.0f);
    void rebuildEngine();
    juce::CriticalSection& getModelLock() { return modelLock; }

private:
    KitModel kitModel;
    DrumSamplerEngine samplerEngine;
    MidiLearnManager midiLearnManager;
    KitForgeServices services;
    juce::CriticalSection modelLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KitForgeAudioProcessor)
};
