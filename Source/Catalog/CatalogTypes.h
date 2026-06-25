#pragma once

#include <JuceHeader.h>
#include <functional>

struct CatalogKitEntry
{
    juce::String id;
    juce::String name;
    juce::String format;
    double sizeMb = 0.0;
    juce::String downloadUrl;
    juce::String thumbnailUrl;
    juce::StringArray tags;
    juce::String description;
    juce::String version;
};

struct CatalogManifest
{
    int version = 1;
    juce::String catalogUrl;
    juce::Array<CatalogKitEntry> kits;

    static CatalogManifest fromVar (const juce::var& v);
    juce::var toVar() const;
};
