#include "KitForgeStandaloneMenuModel.h"
#include "KitForgeStandaloneOptions.h"

juce::StringArray KitForgeStandaloneMenuModel::getMenuBarNames()
{
    return { "KitForge", "Options" };
}

juce::PopupMenu KitForgeStandaloneMenuModel::getMenuForIndex (int topLevelMenuIndex, const juce::String&)
{
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 0)
    {
        menu.addItem (100, "About KitForge");
        menu.addSeparator();
        menu.addItem (101, "Quit KitForge");
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
        if (menuItemID == 100)
        {
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                                    "KitForge",
                                                    "KitForge v" + juce::String (JucePlugin_VersionString));
        }
        else if (menuItemID == 101)
        {
            if (auto* app = juce::JUCEApplicationBase::getInstance())
                app->systemRequestedQuit();
        }

        return;
    }

    if (topLevelMenuIndex == 1)
        KitForgeStandaloneOptions::handleMenuResult (menuItemID);
}
