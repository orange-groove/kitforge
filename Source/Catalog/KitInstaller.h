#pragma once

#include "CatalogTypes.h"
#include "DownloadManager.h"
#include "../Importers/KitForgePackImporter.h"
#include "../Importers/SFZImporter.h"
#include "../Models/KitModel.h"
#include "../Models/SampleIndex.h"
#include <functional>

enum class KitInstallStage
{
    idle,
    downloading,
    extracting,
    importing,
    finished,
    failed
};

struct KitInstallProgress
{
    KitInstallStage stage = KitInstallStage::idle;
    double progress = 0.0;
    juce::String message;
};

struct KitInstallResult
{
    bool success = false;
    juce::String kitId;
    juce::String errorMessage;
    juce::File installPath;
    KitModel kit;
    bool hasKitModel = false;
};

/** Downloads, extracts, and registers a catalog kit. */
class KitInstaller
{
public:
    KitInstaller (DownloadManager& downloadsIn,
                  KitForgePackImporter& packImporterIn,
                  SFZImporter& sfzImporterIn,
                  SampleIndex& sampleIndexIn);

    using ProgressCallback = std::function<void(const KitInstallProgress&)>;
    using CompletionCallback = std::function<void(const KitInstallResult&)>;

    bool isBusy() const { return busy; }

    void installKitAsync (const CatalogKitEntry& kit,
                          ProgressCallback onProgress,
                          CompletionCallback onComplete);

    void cancel();

private:
    DownloadManager& downloads;
    KitForgePackImporter& packImporter;
    SFZImporter& sfzImporter;
    SampleIndex& sampleIndex;

    bool busy = false;

    static juce::String archiveExtensionForFormat (const juce::String& format);
    void finalizeInstallOnBackground (const CatalogKitEntry& kit,
                                      const juce::File& archiveFile,
                                      ProgressCallback onProgress,
                                      CompletionCallback onComplete);
    void registerInstalledLibrary (const CatalogKitEntry& kit, const juce::File& installPath);
};
