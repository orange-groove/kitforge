#pragma once

#include <JuceHeader.h>
#include "../Importers/ImportResult.h"
#include "../Importers/ImportInstallHelper.h"
#include "ImportPreviewPanel.h"

class KitForgeAudioProcessor;

/** Wires library scan → preview → install → sample mapping (keeps current kit layout). */
namespace LibraryImportFlow
{
    using InstallFn = std::function<ImportInstallHelper::InstallResult (ImportResult& confirmed)>;

    void showPreviewAndMap (juce::Component* parent,
                            KitForgeAudioProcessor& processor,
                            ImportResult scanResult,
                            InstallFn installFn,
                            std::function<void()> onLibraryInstalled = {},
                            ImportPreviewPanel::CancelledCallback onCancel = {},
                            ImportPreviewPanel::SfzReimportFunction reimportFn = {});

    void showMappingForInstalledLibrary (juce::Component* parent,
                                         KitForgeAudioProcessor& processor,
                                         const juce::String& libraryName,
                                         KitModel libraryCatalog,
                                         std::function<void()> onComplete = {});
}
