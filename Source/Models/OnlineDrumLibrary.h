#pragma once

#include <JuceHeader.h>

/** Entry from the online SFZ drum library catalog manifest. */
struct OnlineDrumLibrary
{
    juce::String id;
    juce::String name;
    juce::String format; // "sfz", "kitforgepack"
    juce::String license;
    double sizeMb = 0.0;
    juce::String homepageUrl;
    juce::String downloadUrl;
    juce::String description;
    juce::StringArray tags;
    bool recommended = false;

    bool hasDownloadUrl() const { return downloadUrl.trim().isNotEmpty(); }
};
