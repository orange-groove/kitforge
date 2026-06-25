#pragma once

#include <JuceHeader.h>

struct SFZCandidate
{
    juce::File file;
    int score = 0;
    juce::String relativePath;
};

/** Recursively scans folders for SFZ files and ranks drum-kit candidates. */
class SFZScanner
{
public:
    static juce::Array<SFZCandidate> scanFolder (const juce::File& root);
    static juce::File pickBest (const juce::Array<SFZCandidate>& candidates);

private:
    static int scoreFileName (const juce::String& fileName);
};
