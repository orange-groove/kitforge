#include "KitInstaller.h"
#include "../Core/KitForgePaths.h"
#include "../Serialization/KitForgePackWriter.h"

namespace
{
    void report (const KitInstaller::ProgressCallback& cb, KitInstallStage stage, double progress, const juce::String& message)
    {
        if (cb)
        {
            KitInstallProgress p;
            p.stage = stage;
            p.progress = progress;
            p.message = message;
            cb (p);
        }
    }

    juce::File findFirstSfzFile (const juce::File& root)
    {
        for (const auto& file : root.findChildFiles (juce::File::findFiles, true, "*.sfz"))
            return file;

        return {};
    }
}

KitInstaller::KitInstaller (DownloadManager& downloadsIn,
                              KitForgePackImporter& packImporterIn,
                              SFZImporter& sfzImporterIn,
                              SampleIndex& sampleIndexIn)
    : downloads (downloadsIn),
      packImporter (packImporterIn),
      sfzImporter (sfzImporterIn),
      sampleIndex (sampleIndexIn)
{
}

juce::String KitInstaller::archiveExtensionForFormat (const juce::String& format)
{
    if (format.equalsIgnoreCase ("kitforgepack"))
        return ".kitforgepack";

    if (format.equalsIgnoreCase ("sfz"))
        return ".zip";

    return ".zip";
}

void KitInstaller::installKitAsync (const CatalogKitEntry& kit,
                                      ProgressCallback onProgress,
                                      CompletionCallback onComplete)
{
    if (busy)
    {
        KitInstallResult result;
        result.success = false;
        result.errorMessage = "Another install is already in progress.";
        if (onComplete)
            onComplete (result);
        return;
    }

    if (kit.downloadUrl.isEmpty())
    {
        KitInstallResult result;
        result.kitId = kit.id;
        result.success = false;
        result.errorMessage = "Kit has no download URL.";
        if (onComplete)
            onComplete (result);
        return;
    }

    busy = true;
    KitForgePaths::ensureDirectoryStructure();

    const auto extension = archiveExtensionForFormat (kit.format);
    const auto archiveFile = KitForgePaths::getDownloadArchivePath (kit.id, extension);

    report (onProgress, KitInstallStage::downloading, 0.0, "Downloading " + kit.name + "...");

    downloads.downloadFileAsync (kit.downloadUrl, archiveFile,
                                 [this, kit, archiveFile, onProgress, onComplete] (const DownloadProgress& dl)
                                 {
                                     if (! dl.finished)
                                     {
                                         report (onProgress, KitInstallStage::downloading, dl.progress, "Downloading...");
                                         return;
                                     }

                                     if (dl.failed)
                                     {
                                         busy = false;
                                         KitInstallResult result;
                                         result.kitId = kit.id;
                                         result.errorMessage = dl.errorMessage;
                                         if (onComplete)
                                             onComplete (result);
                                         return;
                                     }

                                     juce::Thread::launch ([this, kit, archiveFile, onProgress, onComplete]
                                     {
                                         finalizeInstallOnBackground (kit, archiveFile, onProgress, onComplete);
                                     });
                                 });
}

void KitInstaller::finalizeInstallOnBackground (const CatalogKitEntry& kit,
                                                 const juce::File& archiveFile,
                                                 ProgressCallback onProgress,
                                                 CompletionCallback onComplete)
{
    report (onProgress, KitInstallStage::extracting, 0.0, "Extracting...");

    const auto installPath = KitForgePaths::getLibraryInstallPath (kit.id);
    juce::String extractError;

    if (! downloads.extractArchive (archiveFile, installPath, extractError))
    {
        juce::MessageManager::callAsync ([this, kit, extractError, onComplete]
        {
            busy = false;
            KitInstallResult result;
            result.kitId = kit.id;
            result.errorMessage = extractError;
            if (onComplete)
                onComplete (result);
        });
        return;
    }

    report (onProgress, KitInstallStage::importing, 0.5, "Importing kit...");

    KitInstallResult result;
    result.kitId = kit.id;
    result.installPath = installPath;

    if (kit.format.equalsIgnoreCase ("sfz"))
    {
        const auto sfzFile = findFirstSfzFile (installPath);

        if (! sfzFile.existsAsFile())
        {
            result.errorMessage = "No .sfz file found in downloaded archive.";
        }
        else
        {
            SFZImportOptions options;
            options.sfzFilePath = sfzFile.getFullPathName();
            options.sampleRootPath = sfzFile.getParentDirectory().getFullPathName();
            options.kitName = kit.name;

            const auto importResult = sfzImporter.importFile (options);

            if (importResult.success)
            {
                KitForgePackWriter writer;
                KitForgePackWriter::WriteOptions writeOptions;
                writeOptions.kitName = kit.name;

                const auto writeResult = writer.writeFolder (importResult.kit, installPath, writeOptions);

                if (writeResult.success)
                {
                    result.kit = std::move (importResult.kit);
                    result.kit.resolveSamplePaths (installPath);
                    result.hasKitModel = true;
                    result.success = true;
                }
                else
                {
                    result.errorMessage = writeResult.errorMessage;
                }
            }
            else
            {
                result.errorMessage = importResult.errorMessage;
            }
        }
    }
    else
    {
        const auto packResult = packImporter.importFolder (installPath);

        if (packResult.success)
        {
            result.kit = std::move (packResult.kit);
            result.hasKitModel = true;
            result.success = true;
        }
        else
        {
            result.errorMessage = packResult.errorMessage;
        }
    }

    if (result.success)
        registerInstalledLibrary (kit, installPath);

    juce::MessageManager::callAsync ([this, result, onProgress, onComplete]() mutable
    {
        busy = false;

        if (result.success)
            report (onProgress, KitInstallStage::finished, 1.0, "Installed " + result.kit.kitName);
        else
            report (onProgress, KitInstallStage::failed, 0.0, result.errorMessage);

        if (onComplete)
            onComplete (result);
    });
}

void KitInstaller::registerInstalledLibrary (const CatalogKitEntry& kit, const juce::File& installPath)
{
    InstalledLibrary lib;
    lib.id = kit.id;
    lib.name = kit.name;
    lib.format = kit.format;
    lib.rootPath = installPath.getFullPathName();
    lib.version = kit.version;
    lib.tags = kit.tags;
    lib.installedAtMs = juce::Time::getCurrentTime().toMilliseconds();
    lib.sizeBytes = installPath.getSize();

    sampleIndex.addLibrary (std::move (lib));

    InstalledKit installed;
    installed.id = kit.id;
    installed.name = kit.name;
    installed.source = "catalog";
    installed.libraryId = kit.id;
    installed.kitJsonPath = installPath.getChildFile ("kit.json").getFullPathName();
    installed.tags = kit.tags;
    installed.installedAtMs = lib.installedAtMs;
    sampleIndex.addKit (std::move (installed));

    installPath.getChildFile (".kitforge-version").replaceWithText (kit.version);
}

void KitInstaller::cancel()
{
    downloads.cancelAll();
    busy = false;
}
