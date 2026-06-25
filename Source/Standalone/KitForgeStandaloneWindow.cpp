#include "KitForgeStandaloneWindow.h"

KitForgeStandaloneWindow::KitForgeStandaloneWindow (const juce::String& title,
                                                    juce::Colour backgroundColour,
                                                    std::unique_ptr<juce::StandalonePluginHolder> pluginHolderIn)
    : StandaloneFilterWindow (title, backgroundColour, std::move (pluginHolderIn))
{
   #if ! JUCE_IOS && ! JUCE_ANDROID
    configureNativeChrome();
   #endif
}

void KitForgeStandaloneWindow::configureNativeChrome()
{
    setUsingNativeTitleBar (true);
    setTitleBarButtonsRequired (juce::DocumentWindow::allButtons, false);

    for (int i = 0; i < getNumChildComponents(); ++i)
        if (auto* button = dynamic_cast<juce::TextButton*> (getChildComponent (i)))
            if (button->getButtonText().containsIgnoreCase ("Options"))
                button->setVisible (false);

    menuModel = std::make_unique<KitForgeStandaloneMenuModel>();
    setMenuBar (menuModel.get());
}
