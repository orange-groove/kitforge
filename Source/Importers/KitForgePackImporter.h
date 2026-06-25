#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"

struct KitForgePackMetadata
{
    juce::String name;
    juce::String author;
    juce::String version;
    juce::String description;
    juce::String licenseText;
    juce::String creditsText;
    juce::StringArray tags;
};

struct KitForgePackImportResult
{
    bool success = false;
    juce::String errorMessage;
    KitModel kit;
    KitForgePackMetadata metadata;
    juce::File extractedRoot;
};

/** Imports native .kitforgepack archives (zip containing kit.json + samples/ + artwork/). */
class KitForgePackImporter
{
public:
    KitForgePackImportResult importPack (const juce::File& packFile,
                                         const juce::File& destinationRoot) const;

    /** Import an already-extracted pack folder. */
    KitForgePackImportResult importFolder (const juce::File& packFolder) const;
};

/** Exports a KitModel to .kitforgepack format. */
class KitForgePackExporter
{
public:
    bool exportPack (const KitModel& kit,
                     const KitForgePackMetadata& metadata,
                     const juce::File& outputPackFile) const;
};
