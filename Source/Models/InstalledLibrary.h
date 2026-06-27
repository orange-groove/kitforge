#pragma once

#include <JuceHeader.h>
#include "KitManifest.h"

/** Metadata for an installed `.kitforge` kit on disk. */
struct InstalledLibrary
{
    juce::String id;
    juce::String name;
    juce::String format { "kitforge" };
    juce::String license;
    juce::String version;
    juce::String author;
    juce::String installedPath;
    juce::String kitForgePath;
    juce::String thumbnailPath;
    juce::String previewAudioPath;
    juce::String licensePath;
    juce::String creditsPath;
    juce::StringArray tags;
    int64_t installedAtMs = 0;
    int64_t sizeBytes = 0;
    int sampleCount = 0;
    int pieceCount = 0;

    bool isValid() const { return id.isNotEmpty() && getRoot().exists(); }

    juce::File getRoot() const
    {
        if (installedPath.isNotEmpty())
            return juce::File (installedPath);

        return {};
    }

    juce::File getKitJsonFile() const
    {
        const auto root = getRoot();
        return root.getChildFile ("kit.json");
    }

    juce::File getManifestFile() const
    {
        return getRoot().getChildFile ("manifest.json");
    }

    static InstalledLibrary fromManifest (const KitManifest& manifest,
                                          const juce::File& installFolder,
                                          int sampleCountIn,
                                          int pieceCountIn);

    juce::var toVar() const;
};
