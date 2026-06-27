#pragma once

#include <JuceHeader.h>
#include "../Models/SampleRef.h"

/** Metadata for a single audio file within a velocity layer's round-robin set. */
struct DrumSample
{
    juce::String id;
    juce::String filePath;          // resolved absolute path (runtime) or package-relative (embedded export)
    SampleRef sampleRef;            // optional cross-library reference; resolves into filePath off the audio thread
    int rootMidiNote = 60;
    float gain = 1.0f;
    float pan = 0.0f;
    float pitch = 1.0f;
    int startOffsetSamples = 0;
    int endOffsetSamples = -1; // -1 = use full buffer length

    static juce::String makeId() { return juce::Uuid().toString(); }

    int getEndSample (int totalLength) const
    {
        return endOffsetSamples >= 0 ? juce::jmin (endOffsetSamples, totalLength) : totalLength;
    }
};
