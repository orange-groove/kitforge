#pragma once

#include <JuceHeader.h>
#include "DrumPieceTypes.h"

/** Parsed or indexed metadata for a single sample file. */
struct SampleMetadata
{
    juce::String id;
    juce::String filePath;
    juce::String libraryId;
    juce::String kitId;

    DrumPieceType instrumentType = DrumPieceType::accessory;
    int instrumentIndex = 0;
    juce::String articulation;
    int midiNote = 0;
    juce::String chokeGroupId;
    juce::StringArray tags;

    int minVelocity = 1;
    int maxVelocity = 127;
    int velocityValue = 0;
    int roundRobinIndex = 0;
    float confidence = 0.0f;

    float brightness = 0.5f;
    float transientStrength = 0.5f;
    float decayLength = 0.5f;
    float lowEndAmount = 0.5f;

    int matchScoreForTags (const juce::StringArray& desiredTags) const
    {
        int score = 0;

        for (const auto& tag : desiredTags)
            if (tags.contains (tag, true))
                ++score;

        return score;
    }
};
