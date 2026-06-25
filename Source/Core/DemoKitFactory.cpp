#include "DemoKitFactory.h"
#include "DemoKitSampleBindings.h"
#include "../Models/KitModel.h"
#include "../Serialization/KitSerializer.h"

namespace
{
    constexpr const char* kDemoPackVersion = "3.0.2";

    bool zipFolder (const juce::File& sourceFolder, const juce::File& zipFile)
    {
        juce::ZipFile::Builder builder;

        for (const auto& file : sourceFolder.findChildFiles (juce::File::findFiles, true))
        {
            const auto relativePath = file.getRelativePathFrom (sourceFolder);
            builder.addFile (file, 9, relativePath);
        }

        if (zipFile.existsAsFile())
            zipFile.deleteFile();

        juce::FileOutputStream stream (zipFile);

        if (stream.failedToOpen())
            return false;

        return builder.writeToStream (stream, nullptr);
    }

    bool packIsCurrentVersion (const juce::File& packFile)
    {
        juce::ZipFile zip (packFile);
        const int versionIdx = zip.getIndexOfFileName ("demo-pack-version.txt");

        if (versionIdx < 0)
            return false;

        if (auto versionStream = std::unique_ptr<juce::InputStream> (zip.createStreamForEntry (versionIdx)))
        {
            if (versionStream->readString().trim() != kDemoPackVersion)
                return false;
        }
        else
        {
            return false;
        }

        const int kitIdx = zip.getIndexOfFileName ("kit.json");

        if (kitIdx < 0)
            return false;

        if (auto kitStream = std::unique_ptr<juce::InputStream> (zip.createStreamForEntry (kitIdx)))
        {
            juce::var parsed;

            if (juce::JSON::parse (kitStream->readString(), parsed).failed())
                return false;

            KitModel kit;
            KitSerializer::kitFromVar (kit, parsed);
            return DemoKitSampleBindings::kitHasAssignedSamples (kit);
        }

        return false;
    }

    bool extractPackToFolder (const juce::File& packFile, const juce::File& destination, juce::String& error)
    {
        if (! packFile.existsAsFile())
        {
            error = "Pack file not found.";
            return false;
        }

        if (destination.exists())
            destination.deleteRecursively();

        destination.createDirectory();

        juce::ZipFile zip (packFile);
        const auto result = zip.uncompressTo (destination, true);

        if (result.failed())
        {
            error = result.getErrorMessage();
            return false;
        }

        return true;
    }

    bool installedKitNeedsRepair (const juce::File& installPath)
    {
        if (! installPath.isDirectory())
            return false;

        const auto version = installPath.getChildFile (".kitforge-version").loadFileAsString().trim();

        if (version != kDemoPackVersion)
            return true;

        const auto kitJson = installPath.getChildFile ("kit.json");

        if (! kitJson.existsAsFile())
            return true;

        juce::var parsed;

        if (juce::JSON::parse (kitJson.loadFileAsString(), parsed).failed())
            return true;

        KitModel kit;
        KitSerializer::kitFromVar (kit, parsed);

        if (! DemoKitSampleBindings::kitHasAssignedSamples (kit))
            return true;

        kit.resolveSamplePaths (installPath);
        DemoKitSampleBindings::bindSamplePathsFromFolder (kit, installPath);

        for (const auto& piece : kit.getPieces())
        {
            for (const auto& art : piece.articulations)
            {
                for (const auto& layer : art.layers)
                {
                    for (const auto& sample : layer.roundRobins.samples)
                    {
                        if (sample.filePath.isNotEmpty() && juce::File (sample.filePath).existsAsFile())
                            return false;
                    }
                }
            }
        }

        return true;
    }

    bool repairDemoInstallInPlace (const juce::File& installPath)
    {
        const auto kitJson = installPath.getChildFile ("kit.json");

        if (! kitJson.existsAsFile())
            return false;

        const auto samplesDir = installPath.getChildFile ("samples");

        if (! samplesDir.isDirectory()
            || samplesDir.findChildFiles (juce::File::findFiles, false, "*.wav").isEmpty())
            return false;

        juce::var parsed;

        if (juce::JSON::parse (kitJson.loadFileAsString(), parsed).failed())
            return false;

        KitModel model;
        KitSerializer::kitFromVar (model, parsed);
        DemoKitSampleBindings::bindSamplePathsFromFolder (model, installPath);
        model.resolveSamplePaths (installPath);

        if (! DemoKitSampleBindings::kitHasAssignedSamples (model))
            return false;

        if (! KitSerializer::saveKitToFile (model, kitJson))
            return false;

        installPath.getChildFile (".kitforge-version").replaceWithText (kDemoPackVersion);
        return true;
    }
}

juce::File DemoKitFactory::ensureDemoPackExists()
{
    KitForgePaths::ensureDirectoryStructure();

    const auto packFile = KitForgePaths::getPackCacheRoot().getChildFile ("demo-rock-kit.kitforgepack");

    if (packFile.existsAsFile() && packIsCurrentVersion (packFile))
        return packFile;

    if (packFile.existsAsFile())
        packFile.deleteFile();

    const auto staging = packFile.getSiblingFile ("demo-rock-kit_staging");
    staging.deleteRecursively();
    staging.createDirectory();
    staging.getChildFile ("artwork").createDirectory();

    KitModel model;
    model.kitName = "Demo Rock Kit";
    model.createDefaultKit (800.0f, 600.0f);

    if (! DemoKitSampleBindings::writeBundledSamplesToFolder (staging))
    {
        staging.deleteRecursively();
        return {};
    }

    DemoKitSampleBindings::bindSamplePathsFromFolder (model, staging);

    KitSerializer::saveKitToFile (model, staging.getChildFile ("kit.json"));
    staging.getChildFile ("demo-pack-version.txt").replaceWithText (kDemoPackVersion);
    staging.getChildFile ("license.txt").replaceWithText (
        "Demo Rock Kit for KitForge offline catalog testing.\n"
        "Sample licenses vary (CC0 / CC BY). See credits.txt.\n");
    staging.getChildFile ("credits.txt").replaceWithText (
        "Demo drum one-shots: fugue-state-audio distkit series.\n"
        "From stargate-sample-pack (CC0).\n"
        "https://github.com/stargatedaw/stargate-sample-pack\n");

    if (! zipFolder (staging, packFile))
    {
        staging.deleteRecursively();
        return {};
    }

    staging.deleteRecursively();
    return packFile;
}

juce::String DemoKitFactory::getDemoPackDownloadUrl()
{
    const auto pack = ensureDemoPackExists();

    if (! pack.existsAsFile())
        return {};

    return juce::URL (pack).toString (true);
}

bool DemoKitFactory::repairInstalledDemoKitIfNeeded()
{
    const auto installPath = KitForgePaths::getLibraryInstallPath ("demo-rock-kit");

    if (! installedKitNeedsRepair (installPath))
        return false;

    const auto packFile = ensureDemoPackExists();

    if (packFile.existsAsFile())
    {
        juce::String error;

        if (extractPackToFolder (packFile, installPath, error))
        {
            installPath.getChildFile (".kitforge-version").replaceWithText (kDemoPackVersion);
            return true;
        }
    }

    return repairDemoInstallInPlace (installPath);
}
