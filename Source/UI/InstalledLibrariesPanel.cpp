#include "InstalledLibrariesPanel.h"
#include "../PluginProcessor.h"
#include "../Importers/SFZImporter.h"
#include "../Importers/LooseSampleFolderImporter.h"
#include "../Importers/KontaktImporter.h"
#include "../Importers/ImportInstallHelper.h"
#include "../Importers/KitForgePackImporter.h"
#include "../Models/InstalledLibrary.h"
#include "LibraryImportFlow.h"

namespace
{
    void showImportPreview (InstalledLibrariesPanel* panel,
                            KitForgeAudioProcessor& processor,
                            ImportResult result)
    {
        LibraryImportFlow::showPreviewAndMap (panel,
                                              processor,
                                              std::move (result),
                                              [&processor] (ImportResult& confirmed)
                                              {
                                                  return ImportInstallHelper::installToLibraries (
                                                      confirmed,
                                                      processor.getServices().getSampleIndex());
                                              },
                                              [panel] { panel->refresh(); });
    }
}

InstalledLibrariesPanel::InstalledLibrariesPanel (KitForgeAudioProcessor& processorIn)
    : processorRef (processorIn)
{
    addAndMakeVisible (titleLabel);
    titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    addAndMakeVisible (rescanButton);
    addAndMakeVisible (mapToKitButton);
    addAndMakeVisible (removeButton);
    addAndMakeVisible (revealButton);
    addAndMakeVisible (importFolderButton);
    addAndMakeVisible (importKontaktButton);
    addAndMakeVisible (importSfzButton);
    addAndMakeVisible (libraryList);

    rescanButton.onClick = [this]
    {
        processorRef.getServices().getSampleIndex().scanLibrariesOnDisk();
        refresh();
    };

    mapToKitButton.onClick = [this] { mapSelectedLibraryToKit(); };
    removeButton.onClick = [this] { removeSelectedLibrary(); };
    revealButton.onClick = [this] { revealSelectedLibrary(); };

    importFolderButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Import Sample Folder", juce::File());

        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [this, chooser] (const juce::FileChooser& fc)
                              {
                                  const auto folder = fc.getResult();

                                  if (! folder.isDirectory())
                                      return;

                                  LooseSampleFolderImporter importer;
                                  showImportPreview (this, processorRef, importer.scanFolder (folder));
                              });
    };

    importKontaktButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Import Kontakt Library", juce::File());

        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [this, chooser] (const juce::FileChooser& fc)
                              {
                                  const auto folder = fc.getResult();

                                  if (! folder.isDirectory())
                                      return;

                                  showImportPreview (this,
                                                     processorRef,
                                                     processorRef.getServices().getKontaktImporter().importLibraryFolder (folder));
                              });
    };

    importSfzButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Import SFZ", juce::File(), "*.sfz");

        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this, chooser] (const juce::FileChooser& fc)
                              {
                                  const auto file = fc.getResult();

                                  if (! file.existsAsFile())
                                      return;

                                  SFZImportOptions options;
                                  options.sfzFilePath = file.getFullPathName();
                                  options.sampleRootPath = file.getParentDirectory().getFullPathName();
                                  options.kitName = file.getFileNameWithoutExtension();

                                  showImportPreview (this,
                                                     processorRef,
                                                     processorRef.getServices().getSFZImporter().importFile (options));
                              });
    };

    refresh();
}

const InstalledLibrary* InstalledLibrariesPanel::getSelectedLibrary() const
{
    const auto& libs = processorRef.getServices().getSampleIndex().getLibraries();

    if (! juce::isPositiveAndBelow (selectedRow, (int) libs.size()))
        return nullptr;

    return &libs[(size_t) selectedRow];
}

void InstalledLibrariesPanel::mapSelectedLibraryToKit()
{
    const auto* lib = getSelectedLibrary();

    if (lib == nullptr)
        return;

    const auto kitRoot = lib->getKitJsonFile().getParentDirectory();

    if (! lib->getKitJsonFile().existsAsFile())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                "Map Samples",
                                                "No sample catalog found for this library.");
        return;
    }

    KitForgePackImporter importer;
    const auto result = importer.importFolder (kitRoot);

    if (! result.success)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                "Map Samples",
                                                result.errorMessage);
        return;
    }

    auto libraryCatalog = result.kit;
    libraryCatalog.resolveSamplePaths (kitRoot);

    LibraryImportFlow::showMappingForInstalledLibrary (this,
                                                       processorRef,
                                                       lib->name,
                                                       std::move (libraryCatalog));
}

void InstalledLibrariesPanel::removeSelectedLibrary()
{
    const auto* lib = getSelectedLibrary();

    if (lib == nullptr)
        return;

    const auto root = lib->getRoot();

    if (root.exists())
        root.deleteRecursively();

    processorRef.getServices().getSampleIndex().scanLibrariesOnDisk();
    selectedRow = -1;
    refresh();
}

void InstalledLibrariesPanel::revealSelectedLibrary()
{
    const auto* lib = getSelectedLibrary();

    if (lib == nullptr)
        return;

    lib->getRoot().revealToUser();
}

void InstalledLibrariesPanel::refresh()
{
    libraryList.updateContent();
    libraryList.repaint();

    const bool hasSelection = getSelectedLibrary() != nullptr;
    mapToKitButton.setEnabled (hasSelection);
    removeButton.setEnabled (hasSelection);
    revealButton.setEnabled (hasSelection);
}

void InstalledLibrariesPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
}

void InstalledLibrariesPanel::resized()
{
    auto area = getLocalBounds().reduced (12);
    titleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (8);

    auto buttonRow = area.removeFromTop (28);
    rescanButton.setBounds (buttonRow.removeFromLeft (80));
    buttonRow.removeFromLeft (6);
    mapToKitButton.setBounds (buttonRow.removeFromLeft (100));
    buttonRow.removeFromLeft (6);
    removeButton.setBounds (buttonRow.removeFromLeft (80));
    buttonRow.removeFromLeft (6);
    revealButton.setBounds (buttonRow.removeFromLeft (80));
    buttonRow.removeFromLeft (6);
    importFolderButton.setBounds (buttonRow.removeFromLeft (120));
    buttonRow.removeFromLeft (6);
    importKontaktButton.setBounds (buttonRow.removeFromLeft (130));
    buttonRow.removeFromLeft (6);
    importSfzButton.setBounds (buttonRow.removeFromLeft (110));

    area.removeFromTop (8);
    libraryList.setBounds (area);
}

int InstalledLibrariesPanel::getNumRows()
{
    return (int) processorRef.getServices().getSampleIndex().getLibraries().size();
}

void InstalledLibrariesPanel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    const auto& libs = processorRef.getServices().getSampleIndex().getLibraries();

    if (! juce::isPositiveAndBelow (row, (int) libs.size()))
        return;

    const auto& lib = libs[(size_t) row];

    if (selected)
        g.fillAll (juce::Colour (0xff3d5a80));

    g.setColour (juce::Colours::white);
    juce::String line = lib.name + "  [" + lib.format + "]";

    if (lib.pieceCount > 0)
        line += "  " + juce::String (lib.pieceCount) + " pieces";

    g.drawText (line, 8, 0, w - 16, h, juce::Justification::centredLeft, true);
}

void InstalledLibrariesPanel::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    selectedRow = row;
    refresh();
}
