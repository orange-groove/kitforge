#include <JuceHeader.h>

#if JucePlugin_Build_Standalone

 #include "KitForgeStandaloneWindow.h"
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace
{
    class KitForgeStandaloneApp final : public juce::JUCEApplication
    {
    public:
        KitForgeStandaloneApp()
        {
            juce::PropertiesFile::Options options;
            options.applicationName     = juce::CharPointer_UTF8 (JucePlugin_Name);
            options.filenameSuffix      = ".settings";
            options.osxLibrarySubFolder = "Application Support";
           #if JUCE_LINUX || JUCE_BSD
            options.folderName          = "~/.config";
           #else
            options.folderName          = "";
           #endif

            appProperties.setStorageParameters (options);
        }

        const juce::String getApplicationName() override             { return juce::CharPointer_UTF8 (JucePlugin_Name); }
        const juce::String getApplicationVersion() override          { return JucePlugin_VersionString; }
        bool moreThanOneInstanceAllowed() override                   { return true; }
        void anotherInstanceStarted (const juce::String&) override   {}

        std::unique_ptr<juce::StandalonePluginHolder> createPluginHolder()
        {
           #ifdef JucePlugin_PreferredChannelConfigurations
            constexpr juce::StandalonePluginHolder::PluginInOuts channels[] { JucePlugin_PreferredChannelConfigurations };
            const juce::Array<juce::StandalonePluginHolder::PluginInOuts> channelConfig (channels,
                                                                                          juce::numElementsInArray (channels));
           #else
            const juce::Array<juce::StandalonePluginHolder::PluginInOuts> channelConfig;
           #endif

            return std::make_unique<juce::StandalonePluginHolder> (appProperties.getUserSettings(),
                                                                   false,
                                                                   juce::String{},
                                                                   nullptr,
                                                                   channelConfig,
                                                                   false);
        }

        void initialise (const juce::String&) override
        {
            if (juce::Desktop::getInstance().getDisplays().displays.isEmpty())
            {
                jassertfalse;
                return;
            }

            mainWindow = std::make_unique<KitForgeStandaloneWindow> (
                getApplicationName(),
                juce::LookAndFeel::getDefaultLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId),
                createPluginHolder());

            mainWindow->setVisible (true);
        }

        void shutdown() override
        {
            mainWindow = nullptr;
            appProperties.saveIfNeeded();
        }

        void systemRequestedQuit() override
        {
            if (mainWindow != nullptr)
                mainWindow->getPluginHolder()->savePluginState();

            if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
            {
                juce::Timer::callAfterDelay (100, []
                {
                    if (auto* app = juce::JUCEApplicationBase::getInstance())
                        app->systemRequestedQuit();
                });
            }
            else
            {
                quit();
            }
        }

    private:
        juce::ApplicationProperties appProperties;
        std::unique_ptr<KitForgeStandaloneWindow> mainWindow;
    };
}

JUCE_CREATE_APPLICATION_DEFINE (KitForgeStandaloneApp)

#endif
