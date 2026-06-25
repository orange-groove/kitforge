#pragma once

#include "DownloadManager.h"
#include "../Models/KitModel.h"
#include "DrumLibraryCatalogService.h"
#include "../Importers/SFZImporter.h"
#include "../Importers/SFZScanner.h"
#include "../Importers/KitForgePackImporter.h"
#include "../Models/OnlineDrumLibrary.h"
#include "../Importers/ImportResult.h"
#include <functional>

enum class DrumLibraryInstallStage
{
    idle,
    downloading,
    extracting,
    scanning,
    importing,
    awaitingPreview,
    installing,
    finished,
    failed,
    cancelled
};

struct DrumLibraryInstallProgress
{
    DrumLibraryInstallStage stage = DrumLibraryInstallStage::idle;
    double progress = 0.0;
    juce::String message;
    juce::String libraryId;
};

struct DrumLibraryInstallResult
{
    bool success = false;
    juce::String errorMessage;
    InstalledLibrary library;
    KitModel kit;
    bool hasKitModel = false;
};

/** Async download, extract, SFZ scan, and install orchestration (never runs on audio thread). */
class DrumLibraryDownloadManager
{
public:
    using ProgressCallback = std::function<void(const DrumLibraryInstallProgress&)>;
    using PreviewCallback = std::function<void(ImportResult importResult,
                                               juce::Array<SFZCandidate> sfzCandidates,
                                               const OnlineDrumLibrary& catalogEntry,
                                               std::function<void(ImportResult confirmed)> onConfirmed)>;
    using CompletionCallback = std::function<void(const DrumLibraryInstallResult&)>;

    DrumLibraryDownloadManager (DownloadManager& downloadsIn,
                                SFZImporter& sfzImporterIn,
                                KitForgePackImporter& packImporterIn,
                                DrumLibraryCatalogService& catalogIn);

    bool isBusy() const { return busy; }
    void cancel();
    /** Call when user dismisses import preview without installing. */
    void cancelPendingPreview() { busy = false; }

    /** Full SFZ flow: download -> extract -> scan -> preview callback -> install. */
    void installLibraryAsync (const OnlineDrumLibrary& library,
                              PreviewCallback onPreview,
                              ProgressCallback onProgress,
                              CompletionCallback onComplete);

private:
    DownloadManager& downloads;
    SFZImporter& sfzImporter;
    KitForgePackImporter& packImporter;
    DrumLibraryCatalogService& catalog;
    bool busy = false;
    bool cancelled = false;

    OnlineDrumLibrary currentLibrary;
    ProgressCallback progressCallback;
    PreviewCallback previewCallback;
    CompletionCallback completionCallback;

    juce::File libraryRoot;
    juce::File archiveFile;
    juce::File sourceFolder;

    void report (DrumLibraryInstallStage stage, double progress, const juce::String& message);
    void fail (const juce::String& error);
    void finishWithKit (const DrumLibraryInstallResult& result);

    void beginDownload();
    void onDownloadFinished (const DownloadProgress& dl);
    void extractOnBackground();
    void scanAndImportOnBackground();
    ImportResult importSfzFile (const juce::File& sfzFile) const;
    void installKitForgePackOnBackground();
    void finalizeInstallOnBackground (ImportResult importResult, const juce::File& sfzUsed);
};
