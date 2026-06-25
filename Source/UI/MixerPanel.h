#pragma once

#include <JuceHeader.h>

class KitForgeAudioProcessor;

/** Per-piece volume and pitch controls for the current kit. */
class MixerPanel final : public juce::Component
{
public:
    explicit MixerPanel (KitForgeAudioProcessor& processorIn);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void refreshFromModel();

    /** Opens a non-modal mixer window (raises existing window if already open). */
    static void showWindow (juce::Component* parent, KitForgeAudioProcessor& processor);

private:
    class MixerStrip final : public juce::Component
    {
    public:
        MixerStrip (KitForgeAudioProcessor& processorIn, const juce::String& pieceId);

        void syncFromModel();
        void paint (juce::Graphics& g) override;
        void resized() override;

        juce::String pieceId;

    private:
        KitForgeAudioProcessor& processor;

        juce::Label nameLabel;
        juce::Slider volumeSlider;
        juce::Label volumeValueLabel;
        juce::Slider pitchKnob;
        juce::Label pitchValueLabel;

        void updateVolumeLabel();
        void updatePitchLabel();
        static float semitonesToRatio (float semitones);
        static float ratioToSemitones (float ratio);
    };

    KitForgeAudioProcessor& processor;

    juce::Viewport viewport;
    juce::Component stripContainer;
    juce::OwnedArray<MixerStrip> strips;

    static constexpr int kStripWidth = 72;

    void rebuildStrips();
    void layoutStrips();
};
