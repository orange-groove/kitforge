#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class KitForgeAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit KitForgeAudioProcessorEditor (KitForgeAudioProcessor&);
    ~KitForgeAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    KitForgeAudioProcessor& processorRef;

    juce::TabbedComponent mainTabs { juce::TabbedButtonBar::TabsAtTop };

    std::unique_ptr<class KitBuilderPanel> kitBuilder;
    std::unique_ptr<class LibraryBrowserPanel> libraryBrowser;
    std::unique_ptr<class KitForgeStandaloneHeader> standaloneHeader;

    bool isStandaloneApp = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KitForgeAudioProcessorEditor)
};
