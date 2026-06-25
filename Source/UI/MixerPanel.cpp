#include "MixerPanel.h"
#include "../PluginProcessor.h"
#include "../Models/DrumPieceTypes.h"
#include <cmath>

namespace
{
    class MixerDocumentWindow final : public juce::DocumentWindow
    {
    public:
        MixerDocumentWindow (MixerPanel* content, std::function<void()> onCloseIn)
            : DocumentWindow ("Kit Mixer",
                              juce::Colour (0xff1e1e1e),
                              DocumentWindow::allButtons),
              closeCallback (std::move (onCloseIn))
        {
            setUsingNativeTitleBar (true);
            setContentNonOwned (content, true);
            setResizable (true, true);
            setResizeLimits (320, 280, 1600, 600);
            centreWithSize (720, 380);
        }

        void closeButtonPressed() override
        {
            if (closeCallback)
                closeCallback();

            delete this;
        }

    private:
        std::function<void()> closeCallback;
    };

    juce::DocumentWindow* activeMixerWindow = nullptr;
}

float MixerPanel::MixerStrip::semitonesToRatio (float semitones)
{
    return std::pow (2.0f, semitones / 12.0f);
}

float MixerPanel::MixerStrip::ratioToSemitones (float ratio)
{
    return 12.0f * std::log2 (juce::jmax (ratio, 0.001f));
}

MixerPanel::MixerStrip::MixerStrip (KitForgeAudioProcessor& processorIn, const juce::String& pieceIdIn)
    : processor (processorIn),
      pieceId (pieceIdIn)
{
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    addAndMakeVisible (nameLabel);

    volumeSlider.setSliderStyle (juce::Slider::LinearVertical);
    volumeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setRange (0.0, 2.0, 0.01);
    volumeSlider.setDoubleClickReturnValue (true, 1.0);
    volumeSlider.onValueChange = [this]
    {
        const juce::ScopedLock lock (processor.getModelLock());

        if (auto* piece = processor.getKitModel().findPieceById (pieceId))
        {
            piece->volume = (float) volumeSlider.getValue();
            processor.getKitModel().notifyChanged();
        }

        updateVolumeLabel();
    };
    addAndMakeVisible (volumeSlider);

    volumeValueLabel.setJustificationType (juce::Justification::centred);
    volumeValueLabel.setFont (juce::FontOptions (10.0f));
    volumeValueLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (volumeValueLabel);

    pitchKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    pitchKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    pitchKnob.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                   juce::MathConstants<float>::pi * 2.8f,
                                   true);
    pitchKnob.setRange (-24.0, 24.0, 0.1);
    pitchKnob.setDoubleClickReturnValue (true, 0.0);
    pitchKnob.onValueChange = [this]
    {
        const juce::ScopedLock lock (processor.getModelLock());

        if (auto* piece = processor.getKitModel().findPieceById (pieceId))
        {
            piece->pitch = semitonesToRatio ((float) pitchKnob.getValue());
            processor.getKitModel().notifyChanged();
        }

        updatePitchLabel();
    };
    addAndMakeVisible (pitchKnob);

    pitchValueLabel.setJustificationType (juce::Justification::centred);
    pitchValueLabel.setFont (juce::FontOptions (10.0f));
    pitchValueLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (pitchValueLabel);

    syncFromModel();
}

void MixerPanel::MixerStrip::syncFromModel()
{
    const juce::ScopedLock lock (processor.getModelLock());

    if (const auto* piece = processor.getKitModel().findPieceById (pieceId))
    {
        nameLabel.setText (piece->name, juce::dontSendNotification);
        volumeSlider.setValue (piece->volume, juce::dontSendNotification);
        pitchKnob.setValue (ratioToSemitones (piece->pitch), juce::dontSendNotification);
        updateVolumeLabel();
        updatePitchLabel();
    }
}

void MixerPanel::MixerStrip::updateVolumeLabel()
{
    const int pct = juce::roundToInt (volumeSlider.getValue() * 100.0);
    volumeValueLabel.setText (juce::String (pct) + "%", juce::dontSendNotification);
}

void MixerPanel::MixerStrip::updatePitchLabel()
{
    const float st = (float) pitchKnob.getValue();
    const juce::String sign = st > 0.05f ? "+" : juce::String();
    pitchValueLabel.setText (sign + juce::String (st, 1) + " st", juce::dontSendNotification);
}

void MixerPanel::MixerStrip::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff252525));
    g.fillRect (getLocalBounds());

    g.setColour (juce::Colour (0xff333333));
    g.drawVerticalLine (getWidth() - 1, 0.0f, (float) getHeight());
}

void MixerPanel::MixerStrip::resized()
{
    auto area = getLocalBounds().reduced (4, 6);
    nameLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (4);

    volumeValueLabel.setBounds (area.removeFromBottom (16));
    area.removeFromBottom (4);
    pitchValueLabel.setBounds (area.removeFromBottom (16));
    area.removeFromBottom (4);

    const int knobSize = juce::jmin (area.getWidth(), 52);
    pitchKnob.setBounds (area.removeFromBottom (knobSize).withSizeKeepingCentre (knobSize, knobSize));
    area.removeFromBottom (6);

    volumeSlider.setBounds (area);
}

MixerPanel::MixerPanel (KitForgeAudioProcessor& processorIn)
    : processor (processorIn)
{
    viewport.setViewedComponent (&stripContainer, false);
    viewport.setScrollBarsShown (false, true);
    addAndMakeVisible (viewport);

    rebuildStrips();
}

void MixerPanel::showWindow (juce::Component* parent, KitForgeAudioProcessor& processor)
{
    juce::ignoreUnused (parent);

    if (activeMixerWindow != nullptr)
    {
        activeMixerWindow->setVisible (true);
        activeMixerWindow->toFront (true);

        if (auto* panel = dynamic_cast<MixerPanel*> (activeMixerWindow->getContentComponent()))
            panel->refreshFromModel();

        return;
    }

    auto* panel = new MixerPanel (processor);

    activeMixerWindow = new MixerDocumentWindow (panel, []
    {
        activeMixerWindow = nullptr;
    });

    activeMixerWindow->setVisible (true);
}

void MixerPanel::refreshFromModel()
{
    juce::MessageManager::callAsync ([this]
    {
        if (isShowing())
            rebuildStrips();
    });
}

void MixerPanel::rebuildStrips()
{
    juce::StringArray existingIds;

    for (auto* strip : strips)
        existingIds.add (strip->pieceId);

    juce::StringArray modelIds;

    {
        const juce::ScopedLock lock (processor.getModelLock());

        for (const auto& piece : processor.getKitModel().getPieces())
            modelIds.add (piece.id);
    }

    if (existingIds == modelIds)
    {
        for (auto* strip : strips)
            strip->syncFromModel();

        return;
    }

    strips.clear();
    stripContainer.removeAllChildren();

    {
        const juce::ScopedLock lock (processor.getModelLock());

        for (const auto& piece : processor.getKitModel().getPieces())
        {
            auto* strip = strips.add (new MixerStrip (processor, piece.id));
            stripContainer.addAndMakeVisible (strip);
        }
    }

    layoutStrips();
}

void MixerPanel::layoutStrips()
{
    const int stripHeight = juce::jmax (240, viewport.getHeight());
    const int totalWidth = (int) strips.size() * kStripWidth;

    stripContainer.setSize (juce::jmax (totalWidth, viewport.getWidth()), stripHeight);

    int x = 0;

    for (auto* strip : strips)
    {
        strip->setBounds (x, 0, kStripWidth, stripHeight);
        x += kStripWidth;
    }
}

void MixerPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
}

void MixerPanel::resized()
{
    viewport.setBounds (getLocalBounds());
    layoutStrips();
}
