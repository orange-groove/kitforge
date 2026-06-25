#include "LibraryBrowserPanel.h"
#include "../PluginProcessor.h"

LibraryBrowserPanel::LibraryBrowserPanel (KitForgeAudioProcessor& processorIn)
    : onlinePanel (processorIn),
      installedPanel (processorIn)
{
    onlinePanel.onLibraryInstalled = [this]
    {
        installedPanel.refresh();

        if (onKitInstalled)
            onKitInstalled();
    };

    addAndMakeVisible (onlinePanel);
    addAndMakeVisible (installedPanel);
}

void LibraryBrowserPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
}

void LibraryBrowserPanel::resized()
{
    auto area = getLocalBounds();
    onlinePanel.setBounds (area.removeFromTop (area.getHeight() * 2 / 3));
    installedPanel.setBounds (area);
}
