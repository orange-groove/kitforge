#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"
#include "../Importers/KitForgePackImporter.h"

/** Writes a KitModel to native KitForge folder layout (kit.json + samples/ + artwork/). */
class KitForgePackWriter
{
public:
    struct WriteOptions
    {
        juce::String kitName;
        juce::String creditsText;
        juce::String licenseText;
        bool generatePlaceholderLicense = true;
        /** When true, sample paths in the kit are written as-is (no copy into samples/). */
        bool referenceSamplesInPlace = false;
    };

    struct WriteResult
    {
        bool success = false;
        juce::String errorMessage;
        juce::File outputFolder;
    };

    WriteResult writeFolder (const KitModel& kit,
                             const juce::File& outputFolder,
                             const WriteOptions& options) const;

    /** Also creates a .kitforgepack zip alongside the folder. */
    WriteResult writeFolderAndPack (const KitModel& kit,
                                    const juce::File& outputFolder,
                                    const juce::File& packZipFile,
                                    const WriteOptions& options) const;
};
