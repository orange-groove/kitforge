#pragma once

#include <JuceHeader.h>

/** Metadata for an installed drum library on disk. */
struct InstalledLibrary
{
    juce::String id;
    juce::String name;
    juce::String format;          // "sfz", "kitforgepack", "wav"
    juce::String license;
    juce::String version;
    juce::String installedPath;   // ~/Documents/KitForge/Libraries/<id>/
    juce::String sourcePath;      // .../source/
    juce::String kitForgePath;    // .../kitforge/
    juce::String sfzFileUsed;
    juce::StringArray tags;
    int64_t installedAtMs = 0;
    int64_t sizeBytes = 0;
    int sampleCount = 0;
    int pieceCount = 0;

    // Legacy fields (flat .kitforge folder layout)
    juce::String rootPath;
    juce::String licensePath;
    juce::String creditsPath;

    bool isValid() const { return id.isNotEmpty() && getRoot().exists(); }

    juce::File getRoot() const
    {
        if (installedPath.isNotEmpty())
            return juce::File (installedPath);

        return juce::File (rootPath);
    }

    juce::File getKitJsonFile() const
    {
        if (kitForgePath.isNotEmpty())
            return juce::File (kitForgePath).getChildFile ("kit.json");

        const auto root = getRoot();

        if (root.getChildFile ("kitforge").getChildFile ("kit.json").existsAsFile())
            return root.getChildFile ("kitforge").getChildFile ("kit.json");

        return root.getChildFile ("kit.json");
    }

    static InstalledLibrary fromInstallJson (const juce::File& installJsonFile);
    bool writeInstallJson (const juce::File& libraryRoot) const;
    juce::var toVar() const;
};
