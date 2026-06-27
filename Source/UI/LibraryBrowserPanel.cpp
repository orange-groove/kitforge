#include "LibraryBrowserPanel.h"
#include "../PluginProcessor.h"

LibraryBrowserPanel::LibraryBrowserPanel (KitForgeAudioProcessor& processorIn)
    : installedPanel (processorIn)
{
    addAndMakeVisible (installedPanel);
}

void LibraryBrowserPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
}

void LibraryBrowserPanel::resized()
{
    installedPanel.setBounds (getLocalBounds());
}
