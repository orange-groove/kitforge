#pragma once

#include <JuceHeader.h>

class KitForgeAudioProcessor;

/** Paths, catalog URL, OpenAI key, and library settings. */
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
    juce::Label libraryPathLabel;
    juce::Label catalogUrlLabel;
    juce::TextEditor catalogUrlEditor;
    juce::TextButton openLibraryFolderButton { "Open Library Folder" };

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
