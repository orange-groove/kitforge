#pragma once

#include "../Models/OnlineDrumLibrary.h"

struct DrumLibraryCatalogManifest
{
    int version = 1;
    juce::String catalogUrl;
    juce::Array<OnlineDrumLibrary> libraries;

    static DrumLibraryCatalogManifest fromVar (const juce::var& v);
    juce::var toVar() const;
};
