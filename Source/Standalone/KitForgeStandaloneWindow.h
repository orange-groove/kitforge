#pragma once

#include <JuceHeader.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include "KitForgeStandaloneMenuModel.h"

class KitForgeStandaloneWindow final : public juce::StandaloneFilterWindow
{
public:
    KitForgeStandaloneWindow (const juce::String& title,
                              juce::Colour backgroundColour,
                              std::unique_ptr<juce::StandalonePluginHolder> pluginHolderIn);

private:
    void configureNativeChrome();

    std::unique_ptr<KitForgeStandaloneMenuModel> menuModel;
};
