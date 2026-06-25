#include "KitForgePackImporter.h"
#include "../Serialization/KitSerializer.h"
#include "../Catalog/DownloadManager.h"
#include "../Core/DemoKitSampleBindings.h"

KitForgePackImportResult KitForgePackImporter::importPack (const juce::File& packFile,
                                                            const juce::File& destinationRoot) const
{
    KitForgePackImportResult result;

    if (! packFile.existsAsFile() && ! packFile.isDirectory())
    {
        result.errorMessage = "Pack file not found.";
        return result;
    }

    if (packFile.isDirectory())
        return importFolder (packFile);

    destinationRoot.createDirectory();

    juce::ZipFile zip (packFile);
    const auto unzip = zip.uncompressTo (destinationRoot, true);

    if (unzip.failed())
    {
        result.errorMessage = unzip.getErrorMessage();
        return result;
    }

    return importFolder (destinationRoot);
}

KitForgePackImportResult KitForgePackImporter::importFolder (const juce::File& packFolder) const
{
    KitForgePackImportResult result;

    const auto kitJson = packFolder.getChildFile ("kit.json");

    if (! kitJson.existsAsFile())
    {
        result.errorMessage = "Missing kit.json in pack folder.";
        return result;
    }

    juce::var parsed;

    if (juce::JSON::parse (kitJson.loadFileAsString(), parsed).failed())
    {
        result.errorMessage = "Failed to parse kit.json.";
        return result;
    }

    KitSerializer::kitFromVar (result.kit, parsed);
    result.kit.resolveSamplePaths (packFolder);
    DemoKitSampleBindings::bindSamplePathsFromFolder (result.kit, packFolder);
    result.kit.resolveSamplePaths (packFolder);
    result.extractedRoot = packFolder;
    result.success = true;

    if (auto* meta = parsed.getDynamicObject())
    {
        result.metadata.name = meta->getProperty ("kitName").toString();
        result.metadata.version = meta->getProperty ("version").toString();
    }

    const auto licenseFile = packFolder.getChildFile ("license.txt");

    if (licenseFile.existsAsFile())
        result.metadata.licenseText = licenseFile.loadFileAsString();

    const auto creditsFile = packFolder.getChildFile ("credits.txt");

    if (creditsFile.existsAsFile())
        result.metadata.creditsText = creditsFile.loadFileAsString();

    return result;
}

bool KitForgePackExporter::exportPack (const KitModel& kit,
                                        const KitForgePackMetadata& metadata,
                                        const juce::File& outputPackFile) const
{
    const auto stagingDir = outputPackFile.getSiblingFile (outputPackFile.getFileNameWithoutExtension() + "_staging");
    stagingDir.deleteRecursively();
    stagingDir.createDirectory();

    const auto kitJson = stagingDir.getChildFile ("kit.json");
    KitSerializer::saveKitToFile (kit, kitJson);

    stagingDir.getChildFile ("samples").createDirectory();
    stagingDir.getChildFile ("artwork").createDirectory();

    if (metadata.licenseText.isNotEmpty())
        stagingDir.getChildFile ("license.txt").replaceWithText (metadata.licenseText);

    if (metadata.creditsText.isNotEmpty())
        stagingDir.getChildFile ("credits.txt").replaceWithText (metadata.creditsText);

    juce::String zipError;
    DownloadManager dm;

    if (! dm.createZipFromFolder (stagingDir, outputPackFile, zipError))
        return false;

    stagingDir.deleteRecursively();
    return outputPackFile.existsAsFile();
}
