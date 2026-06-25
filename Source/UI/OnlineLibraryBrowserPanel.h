#pragma once

#include <JuceHeader.h>
#include "../Models/OnlineDrumLibrary.h"
#include "../Importers/SFZScanner.h"
#include "DownloadProgressComponent.h"

class KitForgeAudioProcessor;
struct ImportResult;
struct OnlineDrumLibrary;

/** Browse online SFZ drum libraries, download, and install. */
class OnlineLibraryBrowserPanel final : public juce::Component,
                                        public juce::ListBoxModel
{
public:
    explicit OnlineLibraryBrowserPanel (KitForgeAudioProcessor& processorIn);

    void refresh();
    void paint (juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    std::function<void()> onLibraryInstalled;

private:
    KitForgeAudioProcessor& processorRef;
    juce::Label titleLabel { {}, "SFZ Drum Libraries" };
    juce::TextEditor searchBox;
    juce::TextButton refreshButton { "Refresh" };
    juce::TextButton installButton { "Download / Install" };
    juce::TextButton removeButton { "Remove" };
    juce::TextButton openSourceButton { "Open Source" };
    juce::TextButton openKitForgeButton { "Open KitForge" };
    juce::ListBox libraryList { "OnlineLibraryList", this };
    juce::Label detailLabel;
    DownloadProgressComponent progressComponent;

    juce::Array<OnlineDrumLibrary> visibleLibraries;
    int selectedRow = -1;

    void updateVisibleLibraries();
    void updateDetailText();
    void updateButtonStates();
    const OnlineDrumLibrary* getSelectedLibrary() const;
    void installSelectedLibrary();
    void removeSelectedLibrary();
    void openSelectedSourceFolder();
    void openSelectedKitForgeFolder();
    void showImportPreviewForInstall (ImportResult result,
                                      juce::Array<SFZCandidate> sfzCandidates,
                                      const OnlineDrumLibrary& catalogEntry,
                                      std::function<void(ImportResult)> onConfirmed);
};
