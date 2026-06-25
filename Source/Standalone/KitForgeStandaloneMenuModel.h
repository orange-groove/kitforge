#pragma once

#include <JuceHeader.h>

class KitForgeStandaloneMenuModel final : public juce::MenuBarModel
{
public:
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex (int topLevelMenuIndex, const juce::String&) override;
    void menuItemSelected (int menuItemID, int topLevelMenuIndex) override;
};
