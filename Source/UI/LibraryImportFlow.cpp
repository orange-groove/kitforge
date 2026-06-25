#include "LibraryImportFlow.h"
#include "SampleMappingPanel.h"
#include "../PluginProcessor.h"

void LibraryImportFlow::showMappingForInstalledLibrary (juce::Component* parent,
                                                         KitForgeAudioProcessor& processor,
                                                         const juce::String& libraryName,
                                                         KitModel libraryCatalog,
                                                         std::function<void()> onComplete)
{
    SampleMappingPanel::showDialog (parent,
                                    processor,
                                    libraryName,
                                    std::move (libraryCatalog),
                                    std::move (onComplete));
}

void LibraryImportFlow::showPreviewAndMap (juce::Component* parent,
                                            KitForgeAudioProcessor& processor,
                                            ImportResult scanResult,
                                            InstallFn installFn,
                                            std::function<void()> onLibraryInstalled,
                                            ImportPreviewPanel::CancelledCallback onCancel,
                                            ImportPreviewPanel::SfzReimportFunction reimportFn)
{
    if (! scanResult.success)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                "Import Failed",
                                                scanResult.errorMessage);
        return;
    }

    ImportPreviewPanel::showDialog (parent,
                                    std::move (scanResult),
                                    [&processor, parent, installFn, onLibraryInstalled] (ImportResult confirmed)
                                    {
                                        const auto installResult = installFn (confirmed);

                                        if (! installResult.success)
                                        {
                                            juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                                                    "Install Failed",
                                                                                    installResult.errorMessage);
                                            return;
                                        }

                                        confirmed.kit.resolveSamplePaths (installResult.installPath);

                                        showMappingForInstalledLibrary (parent,
                                                                        processor,
                                                                        confirmed.kitName,
                                                                        std::move (confirmed.kit),
                                                                        onLibraryInstalled);
                                    },
                                    std::move (onCancel),
                                    std::move (reimportFn));
}
