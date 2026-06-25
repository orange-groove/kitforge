#include "KitForgeSettings.h"
#include "KitForgePaths.h"

KitForgeSettings& KitForgeSettings::get()
{
    static KitForgeSettings instance;
    return instance;
}

KitForgeSettings::KitForgeSettings()
{
    load();
}

juce::File KitForgeSettings::getSettingsFile() const
{
    return KitForgePaths::getKitForgeRoot().getChildFile ("settings.json");
}

juce::String KitForgeSettings::getOpenAiApiKey() const
{
    return openAiApiKey;
}

void KitForgeSettings::setOpenAiApiKey (const juce::String& key)
{
    openAiApiKey = key.trim();
}

juce::String KitForgeSettings::getOpenAiModel() const
{
    return openAiModel.isNotEmpty() ? openAiModel : juce::String ("gpt-4o-mini");
}

void KitForgeSettings::setOpenAiModel (const juce::String& model)
{
    openAiModel = model.trim().isNotEmpty() ? model.trim() : juce::String ("gpt-4o-mini");
}

void KitForgeSettings::load()
{
    const auto file = getSettingsFile();

    if (! file.existsAsFile())
        return;

    juce::var parsed;

    if (juce::JSON::parse (file.loadFileAsString(), parsed).failed())
        return;

    if (auto* root = parsed.getDynamicObject())
    {
        openAiApiKey = root->getProperty ("openAiApiKey").toString();
        openAiModel = root->getProperty ("openAiModel").toString();

        if (openAiModel.isEmpty())
            openAiModel = "gpt-4o-mini";
    }
}

void KitForgeSettings::save()
{
    KitForgePaths::ensureDirectoryStructure();

    auto* root = new juce::DynamicObject();
    root->setProperty ("openAiApiKey", openAiApiKey);
    root->setProperty ("openAiModel", getOpenAiModel());

    getSettingsFile().replaceWithText (juce::JSON::toString (juce::var (root), true));
}
