#pragma once

#include <JuceHeader.h>

/** A playable kit installed from catalog, SFZ import, AI builder, or pack file. */
struct InstalledKit
{
    juce::String id;
    juce::String name;
    juce::String source;          // "catalog", "import", "ai", "user"
    juce::String libraryId;       // parent library if applicable
    juce::String kitJsonPath;     // path to kit.json
    juce::String artworkPath;
    juce::String licensePath;
    juce::String creditsPath;
    juce::StringArray tags;
    int64_t installedAtMs = 0;

    bool isValid() const { return id.isNotEmpty() && kitJsonPath.isNotEmpty(); }

    juce::File getKitJsonFile() const { return juce::File (kitJsonPath); }
};
