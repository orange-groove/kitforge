#pragma once

#include <JuceHeader.h>
#include "../Importers/ImportResult.h"
#include "../Models/SampleIndex.h"
#include "../Serialization/KitForgePackWriter.h"
#include "../Core/KitForgePaths.h"

/** Installs a scanned import result as a native KitForge library folder. */
class ImportInstallHelper
{
public:
    struct InstallResult
    {
        bool success = false;
        juce::String errorMessage;
        juce::File installPath;
    };

    static juce::String makeLibraryFolderName (const juce::String& kitName)
    {
        const auto slug = kitName.toLowerCase()
                               .replaceCharacter (' ', '-')
                               .replaceCharacter ('_', '-');

        return slug + ".kitforge";
    }

    static InstallResult installToLibraries (ImportResult importResult, SampleIndex& sampleIndex)
    {
        InstallResult result;

        const auto installPath = KitForgePaths::getLibraryInstallPath (makeLibraryFolderName (importResult.kitName));

        KitForgePackWriter writer;
        KitForgePackWriter::WriteOptions options;
        options.kitName = importResult.kitName;

        const auto writeResult = writer.writeFolder (importResult.kit, installPath, options);

        if (! writeResult.success)
        {
            result.errorMessage = writeResult.errorMessage;
            return result;
        }

        importResult.kit.resolveSamplePaths (installPath);
        sampleIndex.scanLibrariesOnDisk();

        result.success = true;
        result.installPath = installPath;
        return result;
    }
};
