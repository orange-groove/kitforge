#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"
#include "../Models/DrumPieceTypes.h"

namespace DemoKitSampleBindings
{
    juce::String sampleFileNameFor (DrumPieceType type, const juce::String& articulationName);

    /** Assigns relative sample paths from kitRoot/samples/ based on piece type + articulation name. */
    void bindSamplePathsFromFolder (KitModel& model, const juce::File& kitRoot);

    bool kitHasAssignedSamples (const KitModel& model);
}
