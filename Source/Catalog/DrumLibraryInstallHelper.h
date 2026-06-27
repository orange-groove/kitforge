#pragma once

#include <JuceHeader.h>
#include "../Importers/ImportResult.h"
#include "../Models/InstalledLibrary.h"
#include "../Models/OnlineDrumLibrary.h"
#include "../Serialization/KitForgePackWriter.h"

/** Writes installed library layout under ~/Documents/KitForge/Libraries/<id>/. */
class DrumLibraryInstallHelper
{
public:
    struct InstallResult
    {
        bool success = false;
        juce::String errorMessage;
        InstalledLibrary library;
    };

    /** Rewrites sample paths relative to kitForgeDir (e.g. ../source/kick.wav). */
    static void anchorKitSamplePaths (KitModel& kit, const juce::File& kitForgeDir);

    static juce::File getLibraryRoot (const juce::String& libraryId)
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("KitForge")
                   .getChildFile ("Libraries")
                   .getChildFile (libraryId);
    }

    static InstallResult installConvertedKit (const OnlineDrumLibrary& catalogEntry,
                                              ImportResult& importResult,
                                              const juce::File& sfzFileUsed,
                                              const juce::File& sourceFolder)
    {
        InstallResult result;

        const auto libraryRoot = getLibraryRoot (catalogEntry.id);
        libraryRoot.createDirectory();

        const auto kitForgeDir = libraryRoot.getChildFile ("kitforge");

        KitForgePackWriter writer;
        KitForgePackWriter::WriteOptions options;
        options.kitName = catalogEntry.name.isNotEmpty() ? catalogEntry.name : importResult.kitName;
        options.licenseText = "License: " + catalogEntry.license + "\n\n"
                            + "See homepage for full terms: " + catalogEntry.homepageUrl;
        options.creditsText = "Library: " + catalogEntry.name + "\n"
                            + "Imported via KitForge Online Catalog.\n"
                            + "Source format: " + catalogEntry.format + "\n";

        if (sfzFileUsed.existsAsFile())
            options.creditsText += "SFZ file: " + sfzFileUsed.getFileName() + "\n";

        anchorKitSamplePaths (importResult.kit, kitForgeDir);
        options.referenceSamplesInPlace = true;

        const auto writeResult = writer.writeFolder (importResult.kit, kitForgeDir, options);

        if (! writeResult.success)
        {
            result.errorMessage = writeResult.errorMessage;
            return result;
        }

        importResult.kit.resolveSamplePaths (kitForgeDir);

        InstalledLibrary lib;
        lib.id = catalogEntry.id;
        lib.name = catalogEntry.name;
        lib.format = catalogEntry.format;
        lib.license = catalogEntry.license;
        lib.tags = catalogEntry.tags;
        lib.installedPath = libraryRoot.getFullPathName();
        lib.rootPath = lib.installedPath;
        lib.sourcePath = sourceFolder.getFullPathName();
        lib.kitForgePath = kitForgeDir.getFullPathName();
        lib.sfzFileUsed = sfzFileUsed.getFullPathName();
        lib.installedAtMs = juce::Time::getCurrentTime().toMilliseconds();
        lib.sizeBytes = libraryRoot.getSize();
        lib.sampleCount = importResult.stats.wavFileCount;
        lib.pieceCount = importResult.stats.pieceCount;
        lib.licensePath = kitForgeDir.getChildFile ("license.txt").getFullPathName();
        lib.creditsPath = kitForgeDir.getChildFile ("credits.txt").getFullPathName();

        if (! lib.writeInstallJson (libraryRoot))
        {
            result.errorMessage = "Failed to write install.json.";
            return result;
        }

        result.success = true;
        result.library = std::move (lib);
        return result;
    }
};
