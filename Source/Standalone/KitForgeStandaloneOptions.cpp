#include "KitForgeStandaloneOptions.h"

#include "../PluginProcessor.h"
#include "../UI/SettingsPanel.h"
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace
{
    juce::StandaloneFilterWindow* findStandaloneWindow()
    {
        for (int i = 0; i < juce::TopLevelWindow::getNumTopLevelWindows(); ++i)
            if (auto* window = dynamic_cast<juce::StandaloneFilterWindow*> (juce::TopLevelWindow::getTopLevelWindow (i)))
                return window;

        return nullptr;
    }

    void openSettingsWindow()
    {
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
            if (auto* processor = dynamic_cast<KitForgeAudioProcessor*> (holder->processor.get()))
                SettingsPanel::showWindow (nullptr, *processor);
    }
}

juce::PopupMenu KitForgeStandaloneOptions::buildMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Audio/MIDI Settings...");
    menu.addSeparator();
    menu.addItem (2, "KitForge Settings...");
    menu.addSeparator();
    menu.addItem (3, "Save Plugin State...");
    menu.addItem (4, "Load Plugin State...");
    menu.addSeparator();
    menu.addItem (5, "Reset to Default State");

   #if ! JUCE_MAC
    menu.addSeparator();
    menu.addItem (6, "Quit KitForge");
   #endif

    return menu;
}

void KitForgeStandaloneOptions::handleMenuResult (int result)
{
    auto* holder = juce::StandalonePluginHolder::getInstance();

    if (holder == nullptr)
        return;

    switch (result)
    {
        case 1:
            holder->showAudioSettingsDialog();
            break;

        case 2:
            openSettingsWindow();
            break;

        case 3:
            holder->askUserToSaveState();
            break;

        case 4:
            holder->askUserToLoadState();
            break;

        case 5:
            if (auto* window = findStandaloneWindow())
                window->resetToDefaultState();
            break;

        case 6:
            if (auto* app = juce::JUCEApplicationBase::getInstance())
                app->systemRequestedQuit();
            break;

        default:
            break;
    }
}
