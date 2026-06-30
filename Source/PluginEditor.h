#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#if JUCE_WEB_BROWSER
 #include "UI/WebViewBridge.h"
#endif

class KitForgeAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit KitForgeAudioProcessorEditor (KitForgeAudioProcessor&);
    ~KitForgeAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** Invoked by the standalone File menu. No-ops when the WebView UI is unavailable. */
    void saveKit();
    void saveKitAs();
    void loadKit();

private:
    KitForgeAudioProcessor& processorRef;

    void timerCallback() override;

#if JUCE_WEB_BROWSER
    std::unique_ptr<WebViewBridge> bridge;
    std::unique_ptr<juce::WebBrowserComponent> webView;
    juce::Label fallbackLabel;
    bool webViewLoadFailed = false;

    /** Last-seen hit counts (per MIDI note) so we only flash newly played notes. */
    std::array<juce::uint32, DrumSamplerEngine::kNumMidiNotes> lastHitCounters {};

    void loadWebViewUrl();
    void showFallback (const juce::String& reason);
    void pollEngineHits();
#else
    juce::Label fallbackLabel;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KitForgeAudioProcessorEditor)
};
