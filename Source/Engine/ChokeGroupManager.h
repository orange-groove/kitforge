#pragma once

#include <JuceHeader.h>
#include <vector>

class DrumVoice;

/** Stops currently playing voices that share a choke group when a new note in that group fires. */
class ChokeGroupManager
{
public:
    void registerVoice (DrumVoice* voice, const juce::String& chokeGroupId);
    void unregisterVoice (DrumVoice* voice);

    /** Immediately stop all active voices in the given choke group. */
    void chokeGroup (const juce::String& chokeGroupId);

    void clear();

private:
    struct Entry
    {
        DrumVoice* voice = nullptr;
        juce::String chokeGroupId;
    };

    std::vector<Entry> activeVoices;
};
