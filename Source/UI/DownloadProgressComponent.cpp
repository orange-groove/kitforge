#include "DownloadProgressComponent.h"

DownloadProgressComponent::DownloadProgressComponent()
{
    addChildComponent (statusLabel);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addChildComponent (progressBar);
    setVisible (false);
}

void DownloadProgressComponent::setProgress (double value, const juce::String& statusText)
{
    progressValue = juce::jlimit (0.0, 1.0, value);
    statusLabel.setText (statusText, juce::dontSendNotification);
    progressBar.setVisible (showing);
    statusLabel.setVisible (showing);
    repaint();
}

void DownloadProgressComponent::setVisibleProgress (bool shouldShow)
{
    showing = shouldShow;
    setVisible (shouldShow);
    progressBar.setVisible (shouldShow);
    statusLabel.setVisible (shouldShow);

    if (shouldShow)
        startTimerHz (15);
    else
        stopTimer();
}

void DownloadProgressComponent::reset()
{
    progressValue = 0.0;
    statusLabel.setText ({}, juce::dontSendNotification);
    setVisibleProgress (false);
}

void DownloadProgressComponent::paint (juce::Graphics& g)
{
    if (showing)
        g.fillAll (juce::Colour (0xff252525));
}

void DownloadProgressComponent::resized()
{
    auto area = getLocalBounds().reduced (4);
    statusLabel.setBounds (area.removeFromTop (20));
    progressBar.setBounds (area.removeFromTop (18));
}

void DownloadProgressComponent::timerCallback()
{
    progressBar.repaint();
}
