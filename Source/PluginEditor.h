#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#if JUCE_WEB_BROWSER
 #include "UI/WebViewBridge.h"
#endif

class KitForgeAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit KitForgeAudioProcessorEditor (KitForgeAudioProcessor&);
    ~KitForgeAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    KitForgeAudioProcessor& processorRef;

#if JUCE_WEB_BROWSER
    std::unique_ptr<WebViewBridge> bridge;
    std::unique_ptr<juce::WebBrowserComponent> webView;
    juce::Label fallbackLabel;
    bool webViewLoadFailed = false;

    void loadWebViewUrl();
    void showFallback (const juce::String& reason);
#else
    juce::Label fallbackLabel;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KitForgeAudioProcessorEditor)
};
