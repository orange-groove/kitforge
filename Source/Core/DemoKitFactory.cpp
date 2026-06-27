#include "DemoKitFactory.h"
#include "DemoKitSampleBindings.h"
#include "../Models/KitModel.h"
#include "../Serialization/KitForgePackageWriter.h"
#include "../Serialization/KitForgePackageReader.h"

namespace
{
    constexpr const char* kDemoPackageId = "demo-rock-kit";
    constexpr const char* kDemoPackVersion = "1.0.0";

    bool installedKitIsValid (const juce::File& installPath)
    {
        if (! installPath.isDirectory())
            return false;

        const auto manifestFile = installPath.getChildFile ("manifest.json");
        const auto kitJson = installPath.getChildFile ("kit.json");

        if (! manifestFile.existsAsFile() || ! kitJson.existsAsFile())
            return false;

        const auto loaded = KitForgePackageReader::loadInstalledKit (installPath);

        if (! loaded.success)
            return false;

        auto kit = loaded.kit;
        kit.resolveSamplePaths (installPath);

        for (const auto& piece : kit.getPieces())
        {
            for (const auto& art : piece.articulations)
            {
                for (const auto& layer : art.layers)
                {
                    for (const auto& sample : layer.roundRobins.samples)
                    {
                        if (sample.filePath.isNotEmpty() && juce::File (sample.filePath).existsAsFile())
                            return true;
                    }
                }
            }
        }

        return false;
    }
}

juce::File DemoKitFactory::ensureDemoPackExists()
{
    KitForgePaths::ensureDirectoryStructure();

    const auto packFile = KitForgePaths::getPackCacheRoot().getChildFile ("demo-rock-kit.kitforge");

    if (packFile.existsAsFile())
        return packFile;

    KitModel model;
    model.kitName = "Demo Rock Kit";
    model.createDefaultKit (800.0f, 600.0f);

    const auto staging = packFile.getSiblingFile ("demo-rock-kit_staging");
    staging.deleteRecursively();
    staging.createDirectory();

    if (! DemoKitSampleBindings::writeBundledSamplesToFolder (staging))
    {
        staging.deleteRecursively();
        return {};
    }

    DemoKitSampleBindings::bindSamplePathsFromFolder (model, staging);

    KitForgePackageWriter writer;
    KitForgePackageWriter::WriteOptions options;
    options.packageId = kDemoPackageId;
    options.kitName = model.kitName;
    options.version = kDemoPackVersion;
    options.author = "KitForge";
    options.licenseType = "custom";
    options.licenseText =
        "Demo Rock Kit for KitForge.\n"
        "Sample licenses vary (CC0 / CC BY). See credits.txt.\n";
    options.creditsText =
        "Demo drum one-shots: fugue-state-audio distkit series.\n"
        "From stargate-sample-pack (CC0).\n"
        "https://github.com/stargatedaw/stargate-sample-pack\n";
    options.tags = { "demo", "rock", "acoustic" };
    options.generatePlaceholderLicense = false;

    const auto writeResult = writer.writePackage (model, packFile, options);
    staging.deleteRecursively();

    if (! writeResult.success)
        return {};

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
    const auto installPath = KitForgePaths::getKitInstallPath (kDemoPackageId);

    if (installedKitIsValid (installPath))
        return false;

    const auto packFile = ensureDemoPackExists();

    if (! packFile.existsAsFile())
        return false;

    KitForgePackageReader reader;
    const auto installResult = reader.installPackageFile (packFile);
    return installResult.success;
}
