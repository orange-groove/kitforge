#pragma once

#include <JuceHeader.h>
#include "OnlineLibraryBrowserPanel.h"
#include "InstalledLibrariesPanel.h"

class KitForgeAudioProcessor;

/** Tab 2 — online SFZ catalog + installed libraries. */
class LibraryBrowserPanel final : public juce::Component
{
public:
    explicit LibraryBrowserPanel (KitForgeAudioProcessor& processorIn);

    void paint (juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onKitInstalled;

private:
    OnlineLibraryBrowserPanel onlinePanel;
    InstalledLibrariesPanel installedPanel;
};
