#pragma once

#include <JuceHeader.h>

/** Metadata for a native `.kitforge` drum kit package (manifest.json). */
struct KitManifest
{
    static constexpr int kSupportedFormatVersion = 1;

    juce::String format { "kitforge" };
    int formatVersion = kSupportedFormatVersion;
    juce::String packageId;
    juce::String name;
    juce::String version { "1.0.0" };
    juce::String author { "KitForge" };
    juce::String createdAt;
    juce::String updatedAt;
    juce::String license { "custom" };
    juce::String creditsFile { "credits.txt" };
    juce::String licenseFile { "license.txt" };
    juce::String kitFile { "kit.json" };
    juce::String thumbnail { "artwork/thumbnail.png" };
    juce::String previewAudio { "preview/preview.wav" };
    juce::StringArray tags;
    int sampleCount = 0;
    int pieceCount = 0;
    int64 installSizeBytes = 0;

    bool isValid() const
    {
        return format == "kitforge" && packageId.isNotEmpty() && name.isNotEmpty();
    }
};
