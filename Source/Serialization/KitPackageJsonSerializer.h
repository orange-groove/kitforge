#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"

/** Reads/writes native package kit.json (format version 1). */
class KitPackageJsonSerializer
{
public:
    static juce::var kitToPackageVar (const KitModel& kit, const juce::String& packageId);
    static void kitFromPackageVar (KitModel& model, const juce::var& v);

    static bool readKitFromFile (const juce::File& file, KitModel& modelOut, juce::String& errorOut);
    static bool writeKitToFile (const juce::File& file, const KitModel& kit, const juce::String& packageId, juce::String& errorOut);

    static int countSamplesInKit (const KitModel& kit);
};
