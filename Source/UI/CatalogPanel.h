#pragma once

#include <JuceHeader.h>
#include "../Catalog/CatalogTypes.h"

class KitForgeAudioProcessor;

/** Tab 2 — browse online catalog and installed libraries. */
class CatalogPanel final : public juce::Component,
                           public juce::ListBoxModel
{
public:
    explicit CatalogPanel (KitForgeAudioProcessor& processorIn);

    void refresh();
    void paint (juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

    std::function<void()> onKitInstalled;

private:
    KitForgeAudioProcessor& processorRef;
    juce::Label titleLabel { {}, "Online Catalog" };
    juce::TextEditor searchBox;
    juce::TextButton refreshButton { "Refresh" };
    juce::TextButton installButton { "Install Kit" };
    juce::TextButton removeButton { "Remove" };
    juce::ListBox kitList { "CatalogList", this };
    juce::Label statusLabel;
    juce::ProgressBar progressBar { installProgress };
    juce::Array<CatalogKitEntry> visibleKits;

    double installProgress = 0.0;
    int selectedRow = -1;

    void updateVisibleKits();
    void updateButtonStates();
    void installSelectedKit();
    void removeSelectedKit();
    const CatalogKitEntry* getSelectedKit() const;
};
