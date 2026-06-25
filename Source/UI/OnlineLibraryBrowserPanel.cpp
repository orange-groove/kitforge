#include "OnlineLibraryBrowserPanel.h"
#include "../PluginProcessor.h"
#include "../Core/KitForgeServices.h"
#include "../Catalog/DrumLibraryCatalogService.h"
#include "../Catalog/DrumLibraryDownloadManager.h"
#include "ImportPreviewPanel.h"
#include "LibraryImportFlow.h"
#include "../Importers/SFZImporter.h"
#include "../Importers/ImportResult.h"

OnlineLibraryBrowserPanel::OnlineLibraryBrowserPanel (KitForgeAudioProcessor& processorIn)
    : processorRef (processorIn)
{
    addAndMakeVisible (titleLabel);
    titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));

    searchBox.setTextToShowWhenEmpty ("Search libraries...", juce::Colours::grey);
    searchBox.onTextChange = [this] { updateVisibleLibraries(); };
    addAndMakeVisible (searchBox);

    addAndMakeVisible (refreshButton);
    addAndMakeVisible (installButton);
    addAndMakeVisible (removeButton);
    addAndMakeVisible (openSourceButton);
    addAndMakeVisible (openKitForgeButton);
    addAndMakeVisible (libraryList);
    addAndMakeVisible (detailLabel);
    addAndMakeVisible (progressComponent);

    detailLabel.setFont (juce::FontOptions (13.0f));
    detailLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);

    refreshButton.onClick = [this]
    {
        auto& catalog = processorRef.getServices().getDrumLibraryCatalog();
        progressComponent.setVisibleProgress (true);
        progressComponent.setProgress (0.0, "Refreshing catalog...");

        catalog.refreshManifest ([this] (bool success, const DrumLibraryCatalogManifest&)
        {
            progressComponent.reset();
            juce::ignoreUnused (success);
            refresh();
        });
    };

    installButton.onClick = [this] { installSelectedLibrary(); };
    removeButton.onClick = [this] { removeSelectedLibrary(); };
    openSourceButton.onClick = [this] { openSelectedSourceFolder(); };
    openKitForgeButton.onClick = [this] { openSelectedKitForgeFolder(); };

    refresh();
}

void OnlineLibraryBrowserPanel::refresh()
{
    updateVisibleLibraries();
    updateDetailText();
    updateButtonStates();
}

void OnlineLibraryBrowserPanel::updateVisibleLibraries()
{
    visibleLibraries = processorRef.getServices().getDrumLibraryCatalog().search (searchBox.getText());
    libraryList.updateContent();
    selectedRow = juce::jmin (selectedRow, visibleLibraries.size() - 1);
}

void OnlineLibraryBrowserPanel::updateDetailText()
{
    if (const auto* lib = getSelectedLibrary())
    {
        const bool installed = processorRef.getServices().getDrumLibraryCatalog().isLibraryInstalled (lib->id);
        juce::String text;
        text << lib->description << "\n\n";
        text << "License: " << lib->license << "\n";
        text << "Size: ~" << juce::String (lib->sizeMb, 1) << " MB\n";
        text << "Tags: " << lib->tags.joinIntoString (", ") << "\n";
        text << "Status: " << (installed ? "Installed" : "Not installed");

        if (! lib->hasDownloadUrl()
            && processorRef.getServices().getDrumLibraryCatalog().resolveDownloadUrl (*lib).isEmpty())
            text << "\n\nDownload URL not configured yet (see catalog_manifest.json).";

        detailLabel.setText (text, juce::dontSendNotification);
    }
    else
    {
        detailLabel.setText ("Select a library to view details.", juce::dontSendNotification);
    }
}

void OnlineLibraryBrowserPanel::updateButtonStates()
{
    const auto* lib = getSelectedLibrary();
    const bool hasSelection = lib != nullptr;
    const bool installed = hasSelection
                             && processorRef.getServices().getDrumLibraryCatalog().isLibraryInstalled (lib->id);
    const bool hasUrl = hasSelection
                          && processorRef.getServices().getDrumLibraryCatalog().resolveDownloadUrl (*lib).isNotEmpty();

    installButton.setEnabled (hasSelection && hasUrl && ! processorRef.getServices().getDrumLibraryDownloadManager().isBusy());
    removeButton.setEnabled (installed);
    openSourceButton.setEnabled (installed);
    openKitForgeButton.setEnabled (installed);
}

const OnlineDrumLibrary* OnlineLibraryBrowserPanel::getSelectedLibrary() const
{
    if (! juce::isPositiveAndBelow (selectedRow, visibleLibraries.size()))
        return nullptr;

    return &visibleLibraries.getReference (selectedRow);
}

void OnlineLibraryBrowserPanel::showImportPreviewForInstall (ImportResult result,
                                                              juce::Array<SFZCandidate> sfzCandidates,
                                                              const OnlineDrumLibrary& catalogEntry,
                                                              std::function<void(ImportResult)> onConfirmed)
{
    for (const auto& c : sfzCandidates)
        result.sfzCandidates.addIfNotAlreadyThere (c.file);

    if (result.selectedSfzFile.existsAsFile())
        result.sfzCandidates.addIfNotAlreadyThere (result.selectedSfzFile);

    auto reimportFn = [this, catalogEntry] (const juce::File& sfzFile) -> ImportResult
    {
        SFZImportOptions options;
        options.sfzFilePath = sfzFile.getFullPathName();
        options.sampleRootPath = sfzFile.getParentDirectory().getFullPathName();
        options.kitName = catalogEntry.name;
        auto imported = processorRef.getServices().getSFZImporter().importFile (options);
        imported.importFormat = "sfz";
        imported.selectedSfzFile = sfzFile;
        return imported;
    };

    ImportPreviewPanel::showDialog (this,
                                    std::move (result),
                                    [onConfirmed = std::move (onConfirmed)] (ImportResult confirmed)
                                    {
                                        if (onConfirmed)
                                            onConfirmed (std::move (confirmed));
                                    },
                                    [this]
                                    {
                                        processorRef.getServices().getDrumLibraryDownloadManager().cancelPendingPreview();
                                        updateButtonStates();
                                    },
                                    reimportFn);
}

void OnlineLibraryBrowserPanel::installSelectedLibrary()
{
    const auto* lib = getSelectedLibrary();

    if (lib == nullptr)
        return;

    auto& dm = processorRef.getServices().getDrumLibraryDownloadManager();
    progressComponent.setVisibleProgress (true);
    installButton.setEnabled (false);

    dm.installLibraryAsync (*lib,
                            [this] (ImportResult result,
                                    juce::Array<SFZCandidate> sfzCandidates,
                                    const OnlineDrumLibrary& catalogEntry,
                                    std::function<void(ImportResult)> onConfirmed)
                            {
                                progressComponent.reset();
                                showImportPreviewForInstall (std::move (result),
                                                               std::move (sfzCandidates),
                                                               catalogEntry,
                                                               std::move (onConfirmed));
                            },
                            [this] (const DrumLibraryInstallProgress& p)
                            {
                                progressComponent.setVisibleProgress (p.stage != DrumLibraryInstallStage::idle
                                                                      && p.stage != DrumLibraryInstallStage::finished
                                                                      && p.stage != DrumLibraryInstallStage::failed
                                                                      && p.stage != DrumLibraryInstallStage::cancelled
                                                                      && p.stage != DrumLibraryInstallStage::awaitingPreview);
                                progressComponent.setProgress (p.progress, p.message);
                            },
                            [this] (const DrumLibraryInstallResult& result)
                            {
                                progressComponent.reset();
                                updateButtonStates();

                                if (! result.success)
                                {
                                    if (result.errorMessage.isNotEmpty())
                                    {
                                        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                                                "Install Failed",
                                                                                result.errorMessage);
                                    }

                                    return;
                                }

                                processorRef.getServices().getSampleIndex().scanLibrariesOnDisk();

                                if (result.hasKitModel)
                                {
                                    LibraryImportFlow::showMappingForInstalledLibrary (
                                        this,
                                        processorRef,
                                        result.library.name.isNotEmpty() ? result.library.name : result.kit.kitName,
                                        std::move (result.kit),
                                        [this]
                                        {
                                            refresh();

                                            if (onLibraryInstalled)
                                                onLibraryInstalled();
                                        });
                                    return;
                                }

                                refresh();

                                if (onLibraryInstalled)
                                    onLibraryInstalled();
                            });
}

void OnlineLibraryBrowserPanel::removeSelectedLibrary()
{
    const auto* lib = getSelectedLibrary();

    if (lib == nullptr)
        return;

    processorRef.getServices().getDrumLibraryCatalog().removeInstalledLibrary (lib->id);
    processorRef.getServices().getSampleIndex().scanLibrariesOnDisk();
    refresh();

    if (onLibraryInstalled)
        onLibraryInstalled();
}

void OnlineLibraryBrowserPanel::openSelectedSourceFolder()
{
    const auto* lib = getSelectedLibrary();

    if (lib == nullptr)
        return;

    const auto installed = processorRef.getServices().getDrumLibraryCatalog().getInstalledLibrary (lib->id);
    const juce::File folder (installed.sourcePath);

    if (folder.isDirectory())
        folder.revealToUser();
}

void OnlineLibraryBrowserPanel::openSelectedKitForgeFolder()
{
    const auto* lib = getSelectedLibrary();

    if (lib == nullptr)
        return;

    const auto installed = processorRef.getServices().getDrumLibraryCatalog().getInstalledLibrary (lib->id);
    const juce::File folder (installed.kitForgePath);

    if (folder.isDirectory())
        folder.revealToUser();
}

void OnlineLibraryBrowserPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
}

void OnlineLibraryBrowserPanel::resized()
{
    auto area = getLocalBounds().reduced (12);
    titleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (8);

    auto topRow = area.removeFromTop (28);
    searchBox.setBounds (topRow.removeFromLeft (area.getWidth() / 2));
    topRow.removeFromLeft (8);
    refreshButton.setBounds (topRow.removeFromLeft (90));

    area.removeFromTop (8);
    auto buttonRow = area.removeFromTop (28);
    installButton.setBounds (buttonRow.removeFromLeft (140));
    buttonRow.removeFromLeft (8);
    removeButton.setBounds (buttonRow.removeFromLeft (90));
    buttonRow.removeFromLeft (8);
    openSourceButton.setBounds (buttonRow.removeFromLeft (110));
    buttonRow.removeFromLeft (8);
    openKitForgeButton.setBounds (buttonRow.removeFromLeft (120));

    area.removeFromTop (8);
    progressComponent.setBounds (area.removeFromTop (28));
    area.removeFromTop (4);

    detailLabel.setBounds (area.removeFromBottom (80));
    area.removeFromBottom (8);
    libraryList.setBounds (area);
}

int OnlineLibraryBrowserPanel::getNumRows()
{
    return visibleLibraries.size();
}

void OnlineLibraryBrowserPanel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (! juce::isPositiveAndBelow (row, visibleLibraries.size()))
        return;

    const auto& lib = visibleLibraries.getReference (row);
    const bool installed = processorRef.getServices().getDrumLibraryCatalog().isLibraryInstalled (lib.id);

    if (selected)
        g.fillAll (juce::Colour (0xff3d5a80));

    g.setColour (juce::Colours::white);
    juce::String line = lib.name;

    if (lib.recommended)
        line += "  ★";

    line += installed ? "  [installed]" : "  [available]";
    line += "  ~" + juce::String (lib.sizeMb, 0) + " MB";

    g.drawText (line, 8, 0, w - 16, h, juce::Justification::centredLeft, true);
}

void OnlineLibraryBrowserPanel::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    selectedRow = row;
    updateDetailText();
    updateButtonStates();
}
