#pragma once

#include <JuceHeader.h>

/** Persistent user settings (OpenAI key, model, etc.). */
class KitForgeSettings
{
public:
    static KitForgeSettings& get();

    juce::String getOpenAiApiKey() const;
    void setOpenAiApiKey (const juce::String& key);

    juce::String getOpenAiModel() const;
    void setOpenAiModel (const juce::String& model);

    void load();
    void save();

private:
    KitForgeSettings();

    juce::File getSettingsFile() const;

    juce::String openAiApiKey;
    juce::String openAiModel { "gpt-4o-mini" };
};
