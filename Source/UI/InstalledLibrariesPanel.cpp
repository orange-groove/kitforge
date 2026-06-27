#include "InstalledLibrariesPanel.h"
#include "../PluginProcessor.h"
#include "../Serialization/KitForgePackageReader.h"
#include "../Core/KitImportService.h"
#include "../Core/KitForgePaths.h"
#include "LibraryImportFlow.h"

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
    addAndMakeVisible (importSfzButton);
    addAndMakeVisible (importKitforgeButton);
    addAndMakeVisible (libraryList);

    rescanButton.onClick = [this]
    {
        processorRef.getServices().getSampleIndex().scanKitsOnDisk();
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

                                  juce::Thread::launch ([this, folder]
                                  {
                                      auto result = processorRef.getServices().getKitImportService().importLooseFolder (
                                          folder, processorRef.getServices().getLooseFolderImporter());

                                      juce::MessageManager::callAsync ([this, result]
                                      {
                                          if (! result.success)
                                          {
                                              juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                                                      "Import Failed",
                                                                                      result.errorMessage);
                                              return;
                                          }

                                          refresh();
                                      });
                                  });
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

                                  juce::Thread::launch ([this, file]
                                  {
                                      auto result = processorRef.getServices().getKitImportService().importSfzFile (
                                          file, processorRef.getServices().getSFZImporter());

                                      juce::MessageManager::callAsync ([this, result]
                                      {
                                          if (! result.success)
                                          {
                                              juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                                                      "Import Failed",
                                                                                      result.errorMessage);
                                              return;
                                          }

                                          refresh();
                                      });
                                  });
                              });
    };

    importKitforgeButton.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Install KitForge Package", juce::File(), "*.kitforge");

        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this, chooser] (const juce::FileChooser& fc)
                              {
                                  const auto file = fc.getResult();

                                  if (! file.existsAsFile())
                                      return;

                                  juce::Thread::launch ([this, file]
                                  {
                                      auto result = processorRef.getServices().getKitImportService().installKitforgeFile (file);

                                      juce::MessageManager::callAsync ([this, result]
                                      {
                                          if (! result.success)
                                          {
                                              juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                                                      "Install Failed",
                                                                                      result.errorMessage);
                                              return;
                                          }

                                          refresh();
                                      });
                                  });
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

    const auto loaded = KitForgePackageReader::loadInstalledKit (lib->getRoot());

    if (! loaded.success)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                "Map Samples",
                                                loaded.errorMessage);
        return;
    }

    auto libraryCatalog = loaded.kit;
    libraryCatalog.resolveSamplePaths (lib->getRoot());

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

    processorRef.getServices().getSampleIndex().scanKitsOnDisk();
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
    importSfzButton.setBounds (buttonRow.removeFromLeft (110));
    buttonRow.removeFromLeft (6);
    importKitforgeButton.setBounds (buttonRow.removeFromLeft (130));

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
    juce::String line = lib.name + "  v" + lib.version;

    if (lib.author.isNotEmpty())
        line += "  · " + lib.author;

    if (lib.sampleCount > 0)
        line += "  · " + juce::String (lib.sampleCount) + " samples";

    g.drawText (line, 8, 0, w - 16, h, juce::Justification::centredLeft, true);
}

void InstalledLibrariesPanel::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    selectedRow = row;
    refresh();
}
