#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"
#include "../Models/KitManifest.h"

/** Writes native `.kitforge` ZIP packages from a KitModel. */
class KitForgePackageWriter
{
public:
    struct WriteOptions
    {
        juce::String packageId;
        juce::String kitName;
        juce::String author { "KitForge" };
        juce::String version { "1.0.0" };
        juce::String licenseType { "custom" };
        juce::String creditsText;
        juce::String licenseText;
        juce::StringArray tags;
        juce::File artworkFile;
        juce::File previewAudioFile;
        bool generatePlaceholderLicense = true;
    };

    struct WriteResult
    {
        bool success = false;
        juce::String errorMessage;
        juce::File outputFolder;
        juce::File packageFile;
        KitManifest manifest;
    };

    WriteResult writeFolder (const KitModel& kit,
                             const juce::File& outputFolder,
                             const WriteOptions& options) const;

    /** Writes folder layout then zips to `.kitforge`. */
    WriteResult writePackage (const KitModel& kit,
                              const juce::File& packageFile,
                              const WriteOptions& options) const;
};
