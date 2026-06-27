#pragma once

#include <JuceHeader.h>

class KitForgeAudioProcessor;

/** Paths and OpenAI settings. */
class SettingsPanel final : public juce::Component
{
public:
    explicit SettingsPanel (KitForgeAudioProcessor& processorIn);

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** Opens a non-modal settings window (raises existing window if already open). */
    static void showWindow (juce::Component* parent, KitForgeAudioProcessor& processor);

private:
    KitForgeAudioProcessor& processorRef;
    juce::Label titleLabel { {}, "Settings" };
    juce::Label kitsPathLabel;
    juce::TextButton openKitsFolderButton { "Open Kits Folder" };

    juce::Label aiSectionLabel { {}, "AI Kit Builder (OpenAI)" };
    juce::Label apiKeyLabel { {}, "API key:" };
    juce::TextEditor apiKeyEditor;
    juce::Label modelLabel { {}, "Model:" };
    juce::TextEditor modelEditor;
    juce::TextButton saveAiSettingsButton { "Save AI Settings" };
    juce::Label aiHintLabel;

    void loadAiSettings();
    void saveAiSettings();
};
