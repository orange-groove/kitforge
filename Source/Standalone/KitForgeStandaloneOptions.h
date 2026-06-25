#pragma once

#include <JuceHeader.h>

class KitForgeStandaloneOptions
{
public:
    static juce::PopupMenu buildMenu();
    static void handleMenuResult (int result);
};
