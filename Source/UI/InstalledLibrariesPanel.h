#pragma once

#include <JuceHeader.h>
#include "../Models/InstalledLibrary.h"

class KitForgeAudioProcessor;

/** Installed libraries on disk — load, remove, reveal, manual import. */
class InstalledLibrariesPanel final : public juce::Component,
                                      public juce::ListBoxModel
{
public:
    explicit InstalledLibrariesPanel (KitForgeAudioProcessor& processorIn);

    void refresh();
    void paint (juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

private:
    KitForgeAudioProcessor& processorRef;
    juce::Label titleLabel { {}, "Installed Libraries" };
    juce::TextButton rescanButton { "Rescan" };
    juce::TextButton mapToKitButton { "Map to Kit" };
    juce::TextButton removeButton { "Remove" };
    juce::TextButton revealButton { "Reveal" };
    juce::TextButton importFolderButton { "Import Folder..." };
    juce::TextButton importKontaktButton { "Import Kontakt..." };
    juce::TextButton importSfzButton { "Import SFZ..." };
    juce::ListBox libraryList { "LibraryList", this };

    int selectedRow = -1;

    const InstalledLibrary* getSelectedLibrary() const;
    void mapSelectedLibraryToKit();
    void removeSelectedLibrary();
    void revealSelectedLibrary();
};
