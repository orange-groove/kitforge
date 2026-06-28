#include "PluginEditor.h"

#if JUCE_WEB_BROWSER
namespace
{
    juce::File getUiDistDirectory()
    {
        const juce::Array<juce::File> candidates =
        {
            juce::File::getCurrentWorkingDirectory().getChildFile ("ui/dist"),
            juce::File::getSpecialLocation (juce::File::currentApplicationFile)
                .getParentDirectory().getParentDirectory().getChildFile ("Resources/ui/dist"),
            juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                .getParentDirectory().getParentDirectory().getChildFile ("Resources/ui/dist"),
        };

        for (const auto& dir : candidates)
            if (dir.getChildFile ("index.html").existsAsFile())
                return dir;

        return {};
    }
}
#endif

KitForgeAudioProcessorEditor::KitForgeAudioProcessorEditor (KitForgeAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
#if JUCE_WEB_BROWSER
    bridge = std::make_unique<WebViewBridge> (processorRef);

    // The UI hosts its own resize grip (the native WebView covers the host/JUCE resize
    // corner in some hosts, e.g. FL Studio attached mode), so resize the editor on request.
    bridge->onResizeEditorRequested = [this] (int width, int height)
    {
        if (auto* c = getConstrainer())
        {
            width  = juce::jlimit (c->getMinimumWidth(),  c->getMaximumWidth(),  width);
            height = juce::jlimit (c->getMinimumHeight(), c->getMaximumHeight(), height);
        }

        setSize (width, height);
    };

    webView = std::make_unique<juce::WebBrowserComponent> (bridge->buildBrowserOptions());
    bridge->attachToBrowser (*webView);
    addAndMakeVisible (*webView);

    fallbackLabel.setJustificationType (juce::Justification::centred);
    fallbackLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addChildComponent (fallbackLabel);

    loadWebViewUrl();
#else
    fallbackLabel.setText ("KitForge WebView UI is disabled.\nRebuild with JUCE_WEB_BROWSER=1.",
                           juce::dontSendNotification);
    fallbackLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (fallbackLabel);
#endif

    setResizable (true, true);
    setResizeLimits (720, 520, 2560, 1600);
    setSize (980, 680);
}

KitForgeAudioProcessorEditor::~KitForgeAudioProcessorEditor() = default;

#if JUCE_WEB_BROWSER
void KitForgeAudioProcessorEditor::loadWebViewUrl()
{
   #if KITFORGE_USE_DEV_SERVER
    webView->goToURL ("http://localhost:5173");
   #else
    if (getUiDistDirectory().getChildFile ("index.html").existsAsFile())
    {
        webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    }
    else
    {
        showFallback ("React UI not found.\n\nBuild it with:\n  cd ui && npm install && npm run build\n\n"
                      "Or run the dev server with KITFORGE_USE_DEV_SERVER=1:\n  cd ui && npm run dev");
    }
   #endif
}

void KitForgeAudioProcessorEditor::showFallback (const juce::String& reason)
{
    webViewLoadFailed = true;
    webView->setVisible (false);
    fallbackLabel.setText (reason, juce::dontSendNotification);
    fallbackLabel.setVisible (true);
}
#endif

void KitForgeAudioProcessorEditor::saveKit()
{
#if JUCE_WEB_BROWSER
    if (bridge != nullptr)
        bridge->requestSaveKit();
#endif
}

void KitForgeAudioProcessorEditor::saveKitAs()
{
#if JUCE_WEB_BROWSER
    if (bridge != nullptr)
        bridge->requestSaveKitAs();
#endif
}

void KitForgeAudioProcessorEditor::loadKit()
{
#if JUCE_WEB_BROWSER
    if (bridge != nullptr)
        bridge->requestLoadKit();
#endif
}

void KitForgeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));
}

void KitForgeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

#if JUCE_WEB_BROWSER
    bridge->setCanvasSize ((float) area.getWidth(), (float) area.getHeight());

    if (webViewLoadFailed)
        fallbackLabel.setBounds (area);
    else
        webView->setBounds (area);
#else
    fallbackLabel.setBounds (area);
#endif
}
