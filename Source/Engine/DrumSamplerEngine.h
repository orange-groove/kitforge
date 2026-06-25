#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"
#include "ChokeGroupManager.h"
#include "DrumVoice.h"
#include "SampleLoader.h"
#include <array>
#include <unordered_map>
#include <vector>

/** Core drum sampler engine: MIDI mapping, voice pool, velocity layers, round robins, chokes. */
class DrumSamplerEngine
{
public:
    static constexpr int kMaxVoices = 64;

    DrumSamplerEngine();

    void prepare (double sampleRate, int samplesPerBlock);
    void rebuildFromModel (const KitModel& model);

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                       const KitModel& model, int startSample, int numSamples);

    void triggerMidiNote (const KitModel& model, int midiNote, float velocity);
    void triggerPiece (const juce::String& pieceId, float velocity);
    void queueTriggerPiece (const juce::String& pieceId, float velocity);

    SampleLoader& getSampleLoader() { return sampleLoader; }

private:
    struct MidiMapping
    {
        juce::String pieceId;
        juce::String articulationId;
    };

    struct PendingTrigger
    {
        int midiNote = 0;
        float velocity = 127.0f;
        juce::String pieceId;
        bool usePieceId = false;
    };

    SampleLoader sampleLoader;
    ChokeGroupManager chokeManager;
    std::array<DrumVoice, kMaxVoices> voices;

    std::vector<MidiMapping> midiMap;
    std::unordered_map<std::string, int> roundRobinIndices;
    double hostSampleRate = 44100.0;

    juce::AbstractFifo pendingFifo { 128 };
    std::array<PendingTrigger, 128> pendingTriggers {};

    void buildMidiMap (const KitModel& model);
    void collectReferencedPaths (const KitModel& model, juce::StringArray& paths) const;
    void processPendingTriggers (const KitModel& model);
    void triggerPiece (const KitModel& model, const juce::String& pieceId, float velocity);
    void triggerArticulation (const DrumPiece& piece, const Articulation& art, float velocity);

    DrumVoice* allocateVoice();
    void releaseFinishedVoices();
    void stopAllVoices();
};
