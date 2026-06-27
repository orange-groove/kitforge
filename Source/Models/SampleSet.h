#pragma once

#include <JuceHeader.h>
#include "DrumPieceTypes.h"
#include "SampleRef.h"
#include "../Engine/SampleLayer.h"
#include <vector>

/**
    A swappable musical unit drawn from an installed library: one articulation's
    worth of velocity layers + round robins (e.g. "Kick Center", "Snare Rimshot",
    "Ride Bell"). Built by `SampleIndexService` from installed `.kitforge` kits.
*/
struct SampleSet
{
    juce::String id;
    juce::String libraryId;
    juce::String displayName;
    DrumPieceType instrumentType = DrumPieceType::accessory;
    juce::String articulation;
    juce::StringArray tags;

    /** Full playable content (velocity layers → round-robin DrumSamples). */
    std::vector<SampleLayer> layers;

    int sampleCount = 0;
    int velocityLayerCount = 0;
    int roundRobinCount = 0;

    juce::String sourceKitName;
    juce::String sourcePackPath;
    SampleRef previewSampleRef;

    /** True when every referenced sample currently resolves on disk. */
    bool allSamplesPresent = true;

    /** Lightweight metadata payload for UI cards (no sample buffers/paths). */
    juce::var toVar() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("id", id);
        obj->setProperty ("libraryId", libraryId);
        obj->setProperty ("displayName", displayName);
        obj->setProperty ("instrumentType", drumPieceTypeToString (instrumentType));
        obj->setProperty ("articulation", articulation);

        juce::Array<juce::var> tagVars;
        for (const auto& t : tags)
            tagVars.add (t);
        obj->setProperty ("tags", tagVars);

        obj->setProperty ("sampleCount", sampleCount);
        obj->setProperty ("velocityLayerCount", velocityLayerCount);
        obj->setProperty ("roundRobinCount", roundRobinCount);
        obj->setProperty ("sourceKitName", sourceKitName);
        obj->setProperty ("sourcePackPath", sourcePackPath);
        obj->setProperty ("allSamplesPresent", allSamplesPresent);
        return juce::var (obj);
    }
};
