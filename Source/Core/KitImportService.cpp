#include "KitImportService.h"
#include "../Core/KitForgePaths.h"
#include "../Importers/KitImportLayoutEnforcer.h"

KitImportService::KitImportService (SampleIndex& sampleIndexIn)
    : sampleIndex (sampleIndexIn)
{
}

juce::String KitImportService::slugify (const juce::String& text)
{
    juce::String slug = text.trim().toLowerCase();

    for (int i = 0; i < slug.length(); ++i)
    {
        const juce::juce_wchar c = slug[i];

        if (! juce::CharacterFunctions::isLetterOrDigit (c))
            slug = slug.replaceCharacter (c, '-');
    }

    while (slug.contains ("--"))
        slug = slug.replace ("--", "-");

    return slug.trimCharactersAtStart ("-").trimCharactersAtEnd ("-");
}

KitImportService::ImportResult KitImportService::packageAndInstall (const KitModel& kit,
                                                                     const juce::String& kitName,
                                                                     const juce::String& sourceLabel)
{
    ImportResult result;
    juce::ignoreUnused (sourceLabel);

    const auto packageId = slugify (kitName);
    const auto installPath = KitForgePaths::getKitInstallPath (packageId);

    if (installPath.exists())
        installPath.deleteRecursively();

    KitForgePackageWriter writer;
    KitForgePackageWriter::WriteOptions options;
    options.packageId = packageId;
    options.kitName = kitName;
    options.creditsText = "Imported into KitForge from " + sourceLabel + ".\n";

    const auto writeResult = writer.writeFolder (kit, installPath, options);

    if (! writeResult.success)
    {
        installPath.deleteRecursively();
        result.errorMessage = writeResult.errorMessage;
        return result;
    }

    KitForgePackageReader reader;
    const auto installResult = reader.installPackageFolder (installPath);

    if (! installResult.success)
    {
        installPath.deleteRecursively();
        result.errorMessage = installResult.errorMessage;
        return result;
    }

    sampleIndex.scanKitsOnDisk();
    result.success = true;
    result.installPath = installResult.installPath;
    result.installed = installResult.installed;
    return result;
}

KitImportService::ImportResult KitImportService::importSfzFile (const juce::File& sfzFile, SFZImporter& importer)
{
    ImportResult result;

    SFZImportOptions options;
    options.sfzFilePath = sfzFile.getFullPathName();
    options.sampleRootPath = sfzFile.getParentDirectory().getFullPathName();
    options.kitName = sfzFile.getFileNameWithoutExtension();

    const auto importResult = importer.importFile (options);

    if (! importResult.success)
    {
        result.errorMessage = importResult.errorMessage;
        return result;
    }

    return packageAndInstall (importResult.kit, options.kitName, "SFZ");
}

KitImportService::ImportResult KitImportService::importLooseFolder (const juce::File& folder, LooseSampleFolderImporter& importer)
{
    ImportResult result;

    const auto importResult = importer.scanFolder (folder);

    if (! importResult.success)
    {
        result.errorMessage = importResult.errorMessage;
        return result;
    }

    auto kit = importResult.kit;
    applyImportKitLayout (kit, 980.0f, 680.0f);
    const auto kitName = folder.getFileName().isNotEmpty() ? folder.getFileName() : juce::String ("Custom Kit");
    return packageAndInstall (kit, kitName, "WAV folder");
}

KitImportService::ImportResult KitImportService::installKitforgeFile (const juce::File& kitforgeFile)
{
    ImportResult result;

    KitForgePackageReader reader;
    const auto installResult = reader.installPackageFile (kitforgeFile);

    if (! installResult.success)
    {
        result.errorMessage = installResult.errorMessage;
        return result;
    }

    sampleIndex.scanKitsOnDisk();
    result.success = true;
    result.packageFile = kitforgeFile;
    result.installPath = installResult.installPath;
    result.installed = installResult.installed;
    return result;
}

KitImportService::ImportResult KitImportService::exportKitToFile (const KitModel& kit,
                                                                   const juce::File& outputFile,
                                                                   const juce::String& kitName)
{
    ImportResult result;

    KitForgePackageWriter writer;
    KitForgePackageWriter::WriteOptions options;
    options.packageId = slugify (kitName);
    options.kitName = kitName;

    const auto writeResult = writer.writePackage (kit, outputFile, options);

    if (! writeResult.success)
    {
        result.errorMessage = writeResult.errorMessage;
        return result;
    }

    result.success = true;
    result.packageFile = outputFile;
    return result;
}
