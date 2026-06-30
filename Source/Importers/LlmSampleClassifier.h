#pragma once

#include <JuceHeader.h>
#include "../Models/SampleMetadata.h"
#include <vector>

/** Uses an LLM to group a kit's distinct sample-name patterns into pieces +
    articulations. Robust to arbitrary vendor naming where keyword rules fail.

    Falls back gracefully: returns false (leaving samples untouched) when there is
    no API key, the kit has too many distinct patterns, the call fails, or the
    response is invalid. On success it sets each sample's instrumentType,
    articulation, pieceGroupKey, and optional layerScheme ("auto", "roundRobin", "velocity"). */
class LlmSampleClassifier
{
public:
    static bool classify (const juce::String& kitName, std::vector<SampleMetadata>& samples);

private:
    static juce::String systemPrompt();
};
