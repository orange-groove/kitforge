#include "KitForgePackageReader.h"
#include "KitPackageJsonSerializer.h"
#include "ManifestSerializer.h"
#include "KitMigrationService.h"
#include "../Core/KitForgePaths.h"

namespace
{
    int countPiecesInKit (const KitModel& kit)
    {
        return (int) kit.getPieces().size();
    }

    InstalledLibrary installedFromManifest (const KitManifest& manifest, const juce::File& installFolder, const KitModel& kit)
    {
        InstalledLibrary lib;
        lib.id = manifest.packageId;
        lib.name = manifest.name;
        lib.format = "kitforge";
        lib.license = manifest.license;
        lib.version = manifest.version;
        lib.author = manifest.author;
        lib.installedPath = installFolder.getFullPathName();
        lib.kitForgePath = lib.installedPath;
        lib.tags = manifest.tags;
        lib.sampleCount = manifest.sampleCount > 0
                              ? manifest.sampleCount
                              : KitPackageJsonSerializer::countSamplesInKit (kit);
        lib.pieceCount = manifest.pieceCount > 0
                             ? manifest.pieceCount
                             : countPiecesInKit (kit);
        lib.sizeBytes = manifest.installSizeBytes;
        lib.installedAtMs = juce::Time::getCurrentTime().toMilliseconds();
        lib.thumbnailPath = installFolder.getChildFile (manifest.thumbnail).getFullPathName();
        lib.previewAudioPath = installFolder.getChildFile (manifest.previewAudio).getFullPathName();
        lib.licensePath = installFolder.getChildFile (manifest.licenseFile).getFullPathName();
        lib.creditsPath = installFolder.getChildFile (manifest.creditsFile).getFullPathName();
        return lib;
    }
}

KitPackageValidation KitForgePackageReader::validateFolder (const juce::File& packageFolder) const
{
    KitPackageValidation validation;

    KitManifest manifest;
    juce::String manifestError;

    if (! ManifestSerializer::readFromFile (packageFolder.getChildFile ("manifest.json"), manifest, manifestError))
    {
        validation.errors.add (manifestError);
        return validation;
    }

    const auto migration = KitMigrationService::validateManifest (manifest);

    if (! migration.ok)
    {
        validation.errors.add (migration.errorMessage);
        return validation;
    }

    const auto kitJson = packageFolder.getChildFile (manifest.kitFile.isNotEmpty() ? manifest.kitFile : "kit.json");

    if (! kitJson.existsAsFile())
    {
        validation.errors.add ("Missing kit.json");
        return validation;
    }

    KitModel kit;
    juce::String kitError;

    if (! KitPackageJsonSerializer::readKitFromFile (kitJson, kit, kitError))
    {
        validation.errors.add (kitError);
        return validation;
    }

    juce::StringArray pieceIds;
    juce::HashMap<juce::String, bool> seenPieceIds;

    for (const auto& piece : kit.getPieces())
    {
        if (piece.id.isEmpty())
            validation.errors.add ("Piece with empty id in kit.json");

        if (seenPieceIds.contains (piece.id))
            validation.errors.add ("Duplicate piece id: " + piece.id);
        else
            seenPieceIds.set (piece.id, true);

        pieceIds.add (piece.id);

        juce::HashMap<juce::String, bool> seenArtIds;

        for (const auto& art : piece.articulations)
        {
            if (art.id.isEmpty())
                validation.errors.add ("Empty articulation id on piece " + piece.name);

            if (seenArtIds.contains (art.id))
                validation.errors.add ("Duplicate articulation id on piece " + piece.name + ": " + art.id);
            else
                seenArtIds.set (art.id, true);

            for (const auto& layer : art.layers)
            {
                for (const auto& sample : layer.roundRobins.samples)
                {
                    if (sample.filePath.isEmpty())
                        continue;

                    const auto sampleFile = packageFolder.getChildFile (sample.filePath);

                    if (! sampleFile.existsAsFile())
                        validation.warnings.add ("Missing sample: " + sample.filePath);
                }
            }
        }
    }

    if (! packageFolder.getChildFile (manifest.licenseFile).existsAsFile())
        validation.warnings.add ("Missing license.txt");

    if (! packageFolder.getChildFile (manifest.creditsFile).existsAsFile())
        validation.warnings.add ("Missing credits.txt");

    if (! packageFolder.getChildFile (manifest.thumbnail).existsAsFile())
        validation.warnings.add ("Missing thumbnail");

    if (! packageFolder.getChildFile (manifest.previewAudio).existsAsFile())
        validation.warnings.add ("Missing preview audio");

    validation.valid = validation.errors.isEmpty();
    return validation;
}

KitForgePackageReader::ReadResult KitForgePackageReader::readFolder (const juce::File& packageFolder) const
{
    ReadResult result;
    result.packageRoot = packageFolder;
    result.validation = validateFolder (packageFolder);

    if (! result.validation.valid)
    {
        result.errorMessage = result.validation.errors.joinIntoString ("\n");
        return result;
    }

    KitManifest manifest;
    juce::String manifestError;
    ManifestSerializer::readFromFile (packageFolder.getChildFile ("manifest.json"), manifest, manifestError);
    result.manifest = manifest;

    juce::String kitError;

    if (! KitPackageJsonSerializer::readKitFromFile (packageFolder.getChildFile (manifest.kitFile), result.kit, kitError))
    {
        result.errorMessage = kitError;
        return result;
    }

    result.kit.resolveSamplePaths (packageFolder);
    result.kit.kitName = manifest.name.isNotEmpty() ? manifest.name : result.kit.kitName;
    result.success = true;
    return result;
}

KitForgePackageReader::ReadResult KitForgePackageReader::readPackageFile (const juce::File& packageFile) const
{
    ReadResult result;

    if (! packageFile.existsAsFile())
    {
        result.errorMessage = "Package file not found.";
        return result;
    }

    const auto staging = KitForgePaths::getImportStagingRoot().getChildFile (packageFile.getFileNameWithoutExtension() + "_extract");
    staging.deleteRecursively();
    staging.createDirectory();

    juce::ZipFile zip (packageFile);
    const auto unzip = zip.uncompressTo (staging, true);

    if (unzip.failed())
    {
        result.errorMessage = unzip.getErrorMessage();
        return result;
    }

    return readFolder (staging);
}

KitForgePackageReader::InstallResult KitForgePackageReader::installPackageFolder (const juce::File& packageFolder) const
{
    InstallResult result;

    const auto loaded = readFolder (packageFolder);

    if (! loaded.success)
    {
        result.errorMessage = loaded.errorMessage;
        return result;
    }

    result.installed = installedFromManifest (loaded.manifest, packageFolder, loaded.kit);
    result.installPath = packageFolder;
    result.success = true;
    return result;
}

KitForgePackageReader::InstallResult KitForgePackageReader::installPackageFile (const juce::File& packageFile) const
{
    InstallResult result;

    const auto readResult = readPackageFile (packageFile);

    if (! readResult.success)
    {
        result.errorMessage = readResult.errorMessage;
        return result;
    }

    const auto installPath = KitForgePaths::getKitInstallPath (readResult.manifest.packageId);

    if (installPath.exists())
        installPath.deleteRecursively();

    installPath.createDirectory();

    juce::ZipFile zip (packageFile);
    const auto unzip = zip.uncompressTo (installPath, true);

    if (unzip.failed())
    {
        result.errorMessage = unzip.getErrorMessage();
        return result;
    }

    const auto loaded = readFolder (installPath);

    if (! loaded.success)
    {
        result.errorMessage = loaded.errorMessage;
        installPath.deleteRecursively();
        return result;
    }

    result.installed = installedFromManifest (loaded.manifest, installPath, loaded.kit);
    result.installPath = installPath;
    result.success = true;
    return result;
}

KitForgePackageReader::ReadResult KitForgePackageReader::loadInstalledKit (const juce::File& installFolder)
{
    KitForgePackageReader reader;
    return reader.readFolder (installFolder);
}
