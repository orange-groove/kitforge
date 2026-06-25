#pragma once

#include <JuceHeader.h>

/** Copies bundled demo drum WAVs (embedded at build time) into a folder. */
namespace DemoSampleAssets
{
    bool writeBundledSample (const juce::File& destination, const juce::String& fileName);
}
