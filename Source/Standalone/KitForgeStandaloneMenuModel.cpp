#include "KitForgeStandaloneMenuModel.h"
#include "KitForgeStandaloneOptions.h"

#include "../PluginProcessor.h"
#include "../PluginEditor.h"
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace
{
    KitForgeAudioProcessorEditor* findKitForgeEditor()
    {
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
            if (auto* processor = dynamic_cast<KitForgeAudioProcessor*> (holder->processor.get()))
                return dynamic_cast<KitForgeAudioProcessorEditor*> (processor->getActiveEditor());

        return nullptr;
    }

    enum FileMenuIds
    {
        fileSaveKit   = 200,
        fileSaveKitAs = 201,
        fileLoadKit   = 202,
    };
}

juce::StringArray KitForgeStandaloneMenuModel::getMenuBarNames()
{
    return { "File", "Options" };
}

juce::PopupMenu KitForgeStandaloneMenuModel::getMenuForIndex (int topLevelMenuIndex, const juce::String&)
{
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 0)
    {
        menu.addItem (fileSaveKit,   "Save");
        menu.addItem (fileSaveKitAs, "Save As...");
        menu.addSeparator();
        menu.addItem (fileLoadKit,   "Load...");
    }
    else if (topLevelMenuIndex == 1)
    {
        menu = KitForgeStandaloneOptions::buildMenu();
    }

    return menu;
}

void KitForgeStandaloneMenuModel::menuItemSelected (int menuItemID, int topLevelMenuIndex)
{
    if (topLevelMenuIndex == 0)
    {
        auto* editor = findKitForgeEditor();
        if (editor == nullptr)
            return;

        if (menuItemID == fileSaveKit)        editor->saveKit();
        else if (menuItemID == fileSaveKitAs) editor->saveKitAs();
        else if (menuItemID == fileLoadKit)   editor->loadKit();

        return;
    }

    if (topLevelMenuIndex == 1)
        KitForgeStandaloneOptions::handleMenuResult (menuItemID);
}
