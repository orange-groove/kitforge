#pragma once

#include <JuceHeader.h>

class KitForgeAudioProcessor;

/** Bottom overlay prompt for AI-assisted kit building. */
class AIBuilderPanel final : public juce::Component
{
public:
    explicit AIBuilderPanel (KitForgeAudioProcessor& processorIn,
                             std::function<juce::Point<float>()> canvasSizeProviderIn);

    void paint (juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onKitApplied;
    std::function<void()> onDismiss;

private:
    KitForgeAudioProcessor& processorRef;
    std::function<juce::Point<float>()> canvasSizeProvider;
    juce::TextEditor promptBox;
    juce::TextButton buildButton { "Build" };
    juce::TextButton closeButton { "Close" };
    juce::Label statusLabel;

    uint64_t pendingRequestId = 0;

    void handleBuild();
};
