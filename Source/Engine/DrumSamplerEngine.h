#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"
#include "ChokeGroupManager.h"
#include "DrumVoice.h"
#include "SampleLoader.h"
#include <array>
#include <atomic>
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
    void collectReferencedPaths (const KitModel& model, juce::StringArray& paths) const;
    void preloadSamples (const juce::StringArray& paths);
    void syncFromModel (const KitModel& model, const juce::StringArray& referencedPaths);

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                       const KitModel& model, int startSample, int numSamples);

    void triggerMidiNote (const KitModel& model, int midiNote, float velocity);
    void triggerPiece (const juce::String& pieceId, float velocity);
    void triggerArticulation (const juce::String& pieceId, const juce::String& articulationId, float velocity);
    void queueTriggerPiece (const juce::String& pieceId, float velocity);
    void queueTriggerArticulation (const juce::String& pieceId, const juce::String& articulationId, float velocity);

    /** Preview an arbitrary already-decoded file (call preloadSamples first). */
    void queuePreviewFile (const juce::String& absolutePath, float velocity);

    SampleLoader& getSampleLoader() { return sampleLoader; }

    static constexpr int kNumMidiNotes = 128;

    /** Copies the lock-free per-note hit counters (size kNumMidiNotes) for UI hit
        feedback. Safe to call from the message thread; counters are bumped on the
        audio thread whenever a note actually plays. */
    void readHitCounters (juce::uint32* dest) const;

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
        juce::String articulationId;
        bool usePieceId = false;
        bool isPreview = false;
        juce::String previewPath;
    };

    SampleLoader sampleLoader;
    ChokeGroupManager chokeManager;
    std::array<DrumVoice, kMaxVoices> voices;

    std::vector<MidiMapping> midiMap;
    std::unordered_map<std::string, int> roundRobinIndices;
    double hostSampleRate = 44100.0;

    /** Monotonic hit count per MIDI note; bumped on the audio thread, polled by the UI. */
    std::array<std::atomic<juce::uint32>, kNumMidiNotes> hitCounters {};

    juce::CriticalSection triggerLock;
    std::vector<PendingTrigger> pendingTriggers;

    void buildMidiMap (const KitModel& model);
    void processPendingTriggers (const KitModel& model);
    void triggerPiece (const KitModel& model, const juce::String& pieceId, float velocity);
    void triggerPieceArticulation (const KitModel& model, const juce::String& pieceId,
                                   const juce::String& articulationId, float velocity);
    void playArticulation (const DrumPiece& piece, const Articulation& art, float velocity);
    void playPreview (const juce::String& absolutePath, float velocity);

    DrumVoice* allocateVoice();
    void releaseFinishedVoices();
    void stopAllVoices();
};
