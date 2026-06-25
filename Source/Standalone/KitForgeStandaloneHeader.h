#pragma once

#include <JuceHeader.h>

class KitForgeStandaloneHeader final : public juce::Component,
                                       private juce::Button::Listener
{
public:
    KitForgeStandaloneHeader();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label titleLabel { {}, "KitForge" };
    juce::TextButton settingsButton { "Settings" };
    juce::TextButton optionsButton { "Options" };

    void buttonClicked (juce::Button* button) override;
    void showOptionsMenu();
    void openSettings();
};
