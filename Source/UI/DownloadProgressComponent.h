#pragma once

#include <JuceHeader.h>

/** Reusable download/install progress display. */
class DownloadProgressComponent final : public juce::Component,
                                        private juce::Timer
{
public:
    DownloadProgressComponent();

    void setProgress (double value, const juce::String& statusText);
    void setVisibleProgress (bool shouldShow);
    void reset();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label statusLabel;
    juce::ProgressBar progressBar { progressValue };
    double progressValue = 0.0;
    bool showing = false;

    void timerCallback() override;
};
