#pragma once

#include <JuceHeader.h>
#include "InstalledLibrariesPanel.h"

class KitForgeAudioProcessor;

/** Installed `.kitforge` kits browser (native fallback UI). */
class LibraryBrowserPanel final : public juce::Component
{
public:
    explicit LibraryBrowserPanel (KitForgeAudioProcessor& processorIn);

    void paint (juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onKitInstalled;

private:
    InstalledLibrariesPanel installedPanel;
};
