#include "DrumLibraryDownloadManager.h"
#include "DrumLibraryInstallHelper.h"
#include "../Core/KitForgePaths.h"
#include "../Importers/ImportWarning.h"

namespace
{
    juce::String archiveExtensionFromUrl (const juce::String& url)
    {
        const juce::URL parsed (url);
        const auto path = parsed.getSubPath().trim().toLowerCase();

        if (path.endsWith (".tar.bz2")) return ".tar.bz2";
        if (path.endsWith (".tar.gz"))  return ".tar.gz";
        if (path.endsWith (".kitforgepack")) return ".kitforgepack";
        if (path.endsWith (".zip")) return ".zip";

        return ".zip";
    }

    int64 minimumExpectedArchiveBytes (const OnlineDrumLibrary& library)
    {
        if (library.sizeMb > 1.0)
            return (int64) (library.sizeMb * 0.25 * 1024.0 * 1024.0);

        return 65536;
    }
}

DrumLibraryDownloadManager::DrumLibraryDownloadManager (DownloadManager& downloadsIn,
                                                          SFZImporter& sfzImporterIn,
                                                          KitForgePackImporter& packImporterIn,
                                                          DrumLibraryCatalogService& catalogIn)
    : downloads (downloadsIn),
      sfzImporter (sfzImporterIn),
      packImporter (packImporterIn),
      catalog (catalogIn)
{
}

void DrumLibraryDownloadManager::cancel()
{
    cancelled = true;
    downloads.cancelAll();
    busy = false;
}

void DrumLibraryDownloadManager::report (DrumLibraryInstallStage stage, double progress, const juce::String& message)
{
    if (! progressCallback)
        return;

    DrumLibraryInstallProgress p;
    p.stage = stage;
    p.progress = progress;
    p.message = message;
    p.libraryId = currentLibrary.id;
    progressCallback (p);
}

void DrumLibraryDownloadManager::fail (const juce::String& error)
{
    busy = false;
    report (DrumLibraryInstallStage::failed, 0.0, error);

    DrumLibraryInstallResult result;
    result.errorMessage = error;

    if (completionCallback)
        completionCallback (result);
}

void DrumLibraryDownloadManager::finishWithKit (const DrumLibraryInstallResult& result)
{
    busy = false;
    report (DrumLibraryInstallStage::finished, 1.0, "Installed " + currentLibrary.name);

    if (completionCallback)
        completionCallback (result);
}

void DrumLibraryDownloadManager::installLibraryAsync (const OnlineDrumLibrary& library,
                                                        PreviewCallback onPreview,
                                                        ProgressCallback onProgress,
                                                        CompletionCallback onComplete)
{
    if (busy)
    {
        DrumLibraryInstallResult result;
        result.errorMessage = "Another library install is already in progress.";
        if (onComplete)
            onComplete (result);
        return;
    }

    busy = true;
    cancelled = false;
    currentLibrary = library;
    previewCallback = std::move (onPreview);
    progressCallback = std::move (onProgress);
    completionCallback = std::move (onComplete);

    libraryRoot = DrumLibraryInstallHelper::getLibraryRoot (library.id);
    sourceFolder = libraryRoot.getChildFile ("source");

    const auto downloadUrl = catalog.resolveDownloadUrl (library);
    archiveFile = KitForgePaths::getDownloadsCacheRoot()
                      .getChildFile (library.id + archiveExtensionFromUrl (downloadUrl));

    KitForgePaths::ensureDirectoryStructure();

    if (library.format.equalsIgnoreCase ("kitforgepack"))
    {
        beginDownload();
        return;
    }

    if (library.format.equalsIgnoreCase ("sfz"))
    {
        const auto kitJson = libraryRoot.getChildFile ("kitforge").getChildFile ("kit.json");

        if (sourceFolder.isDirectory() && ! kitJson.existsAsFile())
        {
            const auto candidates = SFZScanner::scanFolder (sourceFolder);

            if (candidates.size() > 0)
            {
                report (DrumLibraryInstallStage::scanning, 0.4,
                        "Finishing install from downloaded files...");
                scanAndImportOnBackground();
                return;
            }
        }

        beginDownload();
        return;
    }

    fail ("Unsupported library format: " + library.format);
}

void DrumLibraryDownloadManager::beginDownload()
{
    const auto url = catalog.resolveDownloadUrl (currentLibrary);

    if (url.trim().isEmpty())
    {
        fail ("Download URL not configured yet for \"" + currentLibrary.name + "\".\n"
              "TODO: Add a verified downloadUrl to catalog_manifest.json.");
        return;
    }

    report (DrumLibraryInstallStage::downloading, 0.0, "Downloading " + currentLibrary.name + "...");

    downloads.downloadFileAsync (url, archiveFile,
                                 [this] (const DownloadProgress& dl) { onDownloadFinished (dl); });
}

void DrumLibraryDownloadManager::onDownloadFinished (const DownloadProgress& dl)
{
    if (cancelled)
    {
        busy = false;
        report (DrumLibraryInstallStage::cancelled, 0.0, "Cancelled.");
        return;
    }

    if (! dl.finished)
    {
        report (DrumLibraryInstallStage::downloading, dl.progress, "Downloading...");
        return;
    }

    if (dl.failed || ! archiveFile.existsAsFile())
    {
        fail (dl.errorMessage.isNotEmpty() ? dl.errorMessage : "Download failed or archive is incomplete.");
        return;
    }

    const auto minBytes = minimumExpectedArchiveBytes (currentLibrary);

    if (archiveFile.getSize() < minBytes)
    {
        fail ("Download appears incomplete ("
              + juce::String (archiveFile.getSize()) + " bytes received, expected at least "
              + juce::String (minBytes) + ").");
        return;
    }

    extractOnBackground();
}

void DrumLibraryDownloadManager::extractOnBackground()
{
    report (DrumLibraryInstallStage::extracting, 0.0, "Extracting...");

    juce::Thread::launch ([this]
    {
        if (cancelled)
            return;

        juce::String extractError;

        if (sourceFolder.exists())
            sourceFolder.deleteRecursively();

        sourceFolder.createDirectory();

        const bool extracted = downloads.extractArchive (archiveFile, sourceFolder, extractError);

        juce::MessageManager::callAsync ([this, extracted, extractError]
        {
            if (cancelled)
                return;

            if (! extracted)
            {
                fail (extractError.isNotEmpty() ? extractError : "Extraction failed.");
                return;
            }

            if (currentLibrary.format.equalsIgnoreCase ("kitforgepack"))
                installKitForgePackOnBackground();
            else
                scanAndImportOnBackground();
        });
    });
}

void DrumLibraryDownloadManager::installKitForgePackOnBackground()
{
    report (DrumLibraryInstallStage::installing, 0.8, "Installing kitforge pack...");

    juce::Thread::launch ([this]
    {
        const auto packResult = packImporter.importFolder (sourceFolder);

        juce::MessageManager::callAsync ([this, packResult]() mutable
        {
            if (cancelled)
                return;

            if (! packResult.success)
            {
                fail (packResult.errorMessage);
                return;
            }

            ImportResult importResult;
            importResult.success = true;
            importResult.kitName = currentLibrary.name;
            importResult.kit = std::move (packResult.kit);
            importResult.stats = ImportResult::computeStats (importResult.kit, 0);
            importResult.importFormat = "kitforgepack";
            importResult.sourceRoot = sourceFolder;

            report (DrumLibraryInstallStage::awaitingPreview, 0.75, "Ready for import preview.");

            if (! previewCallback)
            {
                finalizeInstallOnBackground (std::move (importResult), {});
                return;
            }

            previewCallback (std::move (importResult),
                             {},
                             currentLibrary,
                             [this] (ImportResult confirmed) mutable
                             {
                                 finalizeInstallOnBackground (std::move (confirmed), {});
                             });
        });
    });
}

void DrumLibraryDownloadManager::scanAndImportOnBackground()
{
    report (DrumLibraryInstallStage::scanning, 0.5, "Scanning for SFZ files...");

    juce::Thread::launch ([this]
    {
        const auto candidates = SFZScanner::scanFolder (sourceFolder);

        juce::MessageManager::callAsync ([this, candidates]
        {
            if (cancelled)
                return;

            if (candidates.isEmpty())
            {
                fail ("No .sfz files found in downloaded archive.");
                return;
            }

            const auto best = SFZScanner::pickBest (candidates);
            auto importResult = importSfzFile (best);
            importResult.selectedSfzFile = best;

            for (const auto& c : candidates)
                importResult.sfzCandidates.add (c.file);

            if (! importResult.success)
            {
                fail (importResult.errorMessage);
                return;
            }

            if (candidates.size() > 1)
            {
                importResult.warnings.push_back (ImportWarning::make (ImportWarningType::general,
                                                                      juce::String (candidates.size())
                                                                          + " SFZ files found — select one below if needed."));
            }

            if (currentLibrary.license.containsIgnoreCase ("CC-BY-SA")
                || currentLibrary.license.containsIgnoreCase ("attribution"))
            {
                importResult.warnings.push_back (ImportWarning::make (ImportWarningType::general,
                                                                      "License requires attribution / share-alike: "
                                                                          + currentLibrary.license));
            }

            report (DrumLibraryInstallStage::awaitingPreview, 0.75, "Ready for import preview.");

            if (! previewCallback)
            {
                finalizeInstallOnBackground (std::move (importResult), best);
                return;
            }

            previewCallback (std::move (importResult),
                             candidates,
                             currentLibrary,
                             [this] (ImportResult confirmed) mutable
                             {
                                 const auto sfz = confirmed.selectedSfzFile.existsAsFile()
                                                    ? confirmed.selectedSfzFile
                                                    : juce::File();
                                 finalizeInstallOnBackground (std::move (confirmed), sfz);
                             });
        });
    });
}

ImportResult DrumLibraryDownloadManager::importSfzFile (const juce::File& sfzFile) const
{
    SFZImportOptions options;
    options.sfzFilePath = sfzFile.getFullPathName();
    options.sampleRootPath = sfzFile.getParentDirectory().getFullPathName();
    options.kitName = currentLibrary.name;

    auto result = sfzImporter.importFile (options);
    result.importFormat = "sfz";
    result.sourceRoot = sourceFolder;
    return result;
}

void DrumLibraryDownloadManager::finalizeInstallOnBackground (ImportResult importResult, const juce::File& sfzUsed)
{
    report (DrumLibraryInstallStage::installing, 0.9, "Writing KitForge library...");

    juce::Thread::launch ([this, importResult = std::move (importResult), sfzUsed]() mutable
    {
        const auto installResult = DrumLibraryInstallHelper::installConvertedKit (currentLibrary,
                                                                                   importResult,
                                                                                   sfzUsed,
                                                                                   sourceFolder);

        juce::MessageManager::callAsync ([this, installResult, importResult]() mutable
        {
            if (cancelled)
                return;

            if (! installResult.success)
            {
                fail (installResult.errorMessage);
                return;
            }

            DrumLibraryInstallResult result;
            result.success = true;
            result.library = installResult.library;
            result.kit = std::move (importResult.kit);
            result.hasKitModel = true;
            finishWithKit (result);
        });
    });
}
