#include "KitForgeStandaloneHeader.h"
#include "KitForgeStandaloneOptions.h"
#include "../PluginProcessor.h"
#include "../UI/SettingsPanel.h"
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

KitForgeStandaloneHeader::KitForgeStandaloneHeader()
{
    titleLabel.setFont (juce::FontOptions { 15.0f, juce::Font::bold });
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (titleLabel);

    settingsButton.addListener (this);
    addAndMakeVisible (settingsButton);

    optionsButton.addListener (this);
    optionsButton.setTriggeredOnMouseDown (true);
    addAndMakeVisible (optionsButton);
}

void KitForgeStandaloneHeader::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRect (bounds);

    g.setColour (juce::Colour (0xff333333));
    g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());
}

void KitForgeStandaloneHeader::resized()
{
    auto area = getLocalBounds().reduced (10, 6);
    optionsButton.setBounds (area.removeFromRight (88));
    area.removeFromRight (6);
    settingsButton.setBounds (area.removeFromRight (88));
    titleLabel.setBounds (area);
}

void KitForgeStandaloneHeader::buttonClicked (juce::Button* button)
{
    if (button == &settingsButton)
        openSettings();
    else if (button == &optionsButton)
        showOptionsMenu();
}

void KitForgeStandaloneHeader::openSettings()
{
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
        if (auto* processor = dynamic_cast<KitForgeAudioProcessor*> (holder->processor.get()))
            SettingsPanel::showWindow (this, *processor);
}

void KitForgeStandaloneHeader::showOptionsMenu()
{
    auto menu = KitForgeStandaloneOptions::buildMenu();

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (optionsButton),
                        [] (int result)
                        {
                            if (result != 0)
                                KitForgeStandaloneOptions::handleMenuResult (result);
                        });
}
