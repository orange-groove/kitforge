#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"
#include "../Models/InstalledLibrary.h"
#include "../Models/SampleIndex.h"
#include "../Serialization/KitForgePackageWriter.h"
#include "../Serialization/KitForgePackageReader.h"
#include "../Importers/SFZImporter.h"
#include "../Importers/SFZScanner.h"
#include "../Importers/LooseSampleFolderImporter.h"

/** Converts external import sources into native `.kitforge` packages. */
class KitImportService
{
public:
    struct ImportResult
    {
        bool success = false;
        juce::String errorMessage;
        juce::File packageFile;
        juce::File installPath;
        InstalledLibrary installed;
    };

    explicit KitImportService (SampleIndex& sampleIndexIn);

    ImportResult importSfzFile (const juce::File& sfzFile, SFZImporter& importer);
    ImportResult importLooseFolder (const juce::File& folder, LooseSampleFolderImporter& importer);
    ImportResult installKitforgeFile (const juce::File& kitforgeFile);
    ImportResult exportKitToFile (const KitModel& kit, const juce::File& outputFile,
                                  const juce::String& kitName, bool selfContained = true);

private:
    SampleIndex& sampleIndex;

    ImportResult packageAndInstall (const KitModel& kit, const juce::String& kitName, const juce::String& sourceLabel);
    static juce::String slugify (const juce::String& text);
};
