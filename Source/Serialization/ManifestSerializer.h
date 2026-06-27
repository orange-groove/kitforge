#pragma once

#include <JuceHeader.h>
#include "../Models/KitManifest.h"

class ManifestSerializer
{
public:
    static juce::var manifestToVar (const KitManifest& manifest);
    static KitManifest manifestFromVar (const juce::var& v);

    static bool readFromFile (const juce::File& file, KitManifest& manifestOut, juce::String& errorOut);
    static bool writeToFile (const juce::File& file, const KitManifest& manifest, juce::String& errorOut);
};
