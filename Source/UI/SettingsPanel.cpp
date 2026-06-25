#include "SettingsPanel.h"
#include "../PluginProcessor.h"
#include "../Core/KitForgePaths.h"
#include "../Core/KitForgeSettings.h"

namespace
{
    class SettingsDocumentWindow final : public juce::DocumentWindow
    {
    public:
        SettingsDocumentWindow (SettingsPanel* content, std::function<void()> onCloseIn)
            : DocumentWindow ("KitForge Settings",
                              juce::Colour (0xff1e1e1e),
                              DocumentWindow::allButtons),
              closeCallback (std::move (onCloseIn))
        {
            setUsingNativeTitleBar (true);
            setContentNonOwned (content, true);
            setResizable (true, true);
            setResizeLimits (420, 360, 720, 800);
            centreWithSize (480, 420);
        }

        void closeButtonPressed() override
        {
            if (closeCallback)
                closeCallback();

            delete this;
        }

    private:
        std::function<void()> closeCallback;
    };

    juce::DocumentWindow* activeSettingsWindow = nullptr;
}

void SettingsPanel::showWindow (juce::Component* parent, KitForgeAudioProcessor& processor)
{
    juce::ignoreUnused (parent);

    if (activeSettingsWindow != nullptr)
    {
        activeSettingsWindow->setVisible (true);
        activeSettingsWindow->toFront (true);
        return;
    }

    auto* panel = new SettingsPanel (processor);

    activeSettingsWindow = new SettingsDocumentWindow (panel, []
    {
        activeSettingsWindow = nullptr;
    });

    activeSettingsWindow->setVisible (true);
}

SettingsPanel::SettingsPanel (KitForgeAudioProcessor& processorIn)
    : processorRef (processorIn)
{
    addAndMakeVisible (titleLabel);
    titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));

    addAndMakeVisible (libraryPathLabel);
    libraryPathLabel.setText ("Library: " + KitForgePaths::getLibrariesRoot().getFullPathName(), juce::dontSendNotification);

    addAndMakeVisible (catalogUrlLabel);
    catalogUrlLabel.setText ("Catalog manifest URL:", juce::dontSendNotification);

    addAndMakeVisible (catalogUrlEditor);
    catalogUrlEditor.setText (processorRef.getServices().getCatalog().getManifestUrl());

    addAndMakeVisible (openLibraryFolderButton);
    openLibraryFolderButton.onClick = []
    {
        KitForgePaths::ensureDirectoryStructure();
        KitForgePaths::getLibrariesRoot().revealToUser();
    };

    addAndMakeVisible (aiSectionLabel);
    aiSectionLabel.setFont (juce::FontOptions (15.0f, juce::Font::bold));

    addAndMakeVisible (apiKeyLabel);
    addAndMakeVisible (apiKeyEditor);
    apiKeyEditor.setPasswordCharacter (0x2022);

    addAndMakeVisible (modelLabel);
    addAndMakeVisible (modelEditor);

    addAndMakeVisible (saveAiSettingsButton);
    saveAiSettingsButton.onClick = [this] { saveAiSettings(); };

    addAndMakeVisible (aiHintLabel);
    aiHintLabel.setText ("ChatGPT structures kit layout and MIDI mapping from your prompt. "
                          "Samples are matched locally if available.",
                          juce::dontSendNotification);
    aiHintLabel.setColour (juce::Label::textColourId, juce::Colours::grey);

    loadAiSettings();
}

void SettingsPanel::loadAiSettings()
{
    const auto& settings = KitForgeSettings::get();
    apiKeyEditor.setText (settings.getOpenAiApiKey(), juce::dontSendNotification);
    modelEditor.setText (settings.getOpenAiModel(), juce::dontSendNotification);
}

void SettingsPanel::saveAiSettings()
{
    auto& settings = KitForgeSettings::get();
    settings.setOpenAiApiKey (apiKeyEditor.getText());
    settings.setOpenAiModel (modelEditor.getText());
    settings.save();
}

void SettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
}

void SettingsPanel::resized()
{
    auto area = getLocalBounds().reduced (16);
    titleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (16);
    libraryPathLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (16);
    catalogUrlLabel.setBounds (area.removeFromTop (22));
    catalogUrlEditor.setBounds (area.removeFromTop (28));
    area.removeFromTop (16);
    openLibraryFolderButton.setBounds (area.removeFromTop (32).removeFromLeft (180));
    area.removeFromTop (24);
    aiSectionLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (8);
    aiHintLabel.setBounds (area.removeFromTop (36));
    area.removeFromTop (12);
    apiKeyLabel.setBounds (area.removeFromTop (22));
    apiKeyEditor.setBounds (area.removeFromTop (28));
    area.removeFromTop (12);
    modelLabel.setBounds (area.removeFromTop (22));
    modelEditor.setBounds (area.removeFromTop (28));
    area.removeFromTop (12);
    saveAiSettingsButton.setBounds (area.removeFromTop (32).removeFromLeft (140));
}
