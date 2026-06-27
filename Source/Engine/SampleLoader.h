#pragma once

#include <JuceHeader.h>
#include <memory>
#include <unordered_map>

/** In-memory decoded audio ready for playback. */
struct LoadedSample
{
    juce::String filePath;
    juce::AudioBuffer<float> buffer;
    double sampleRate = 44100.0;
    int lengthInSamples = 0;
};

/** Loads and caches WAV files for the sampler engine. */
class SampleLoader
{
public:
    SampleLoader();

    void prepare (double sampleRate);
    const LoadedSample* load (const juce::String& filePath);

    /** Decodes any not-yet-cached paths in parallel. Much faster than calling
        load() in a loop for large kits (startup preload). */
    void loadMany (const juce::StringArray& filePaths);

    const LoadedSample* getCached (const juce::String& filePath) const;
    void unloadAll();
    void pruneUnused (const juce::StringArray& referencedPaths);

    double getTargetSampleRate() const { return targetSampleRate; }

private:
    juce::AudioFormatManager formatManager;
    std::unordered_map<std::string, std::unique_ptr<LoadedSample>> cache;
    double targetSampleRate = 44100.0;

    std::unique_ptr<LoadedSample> decodeFile (const juce::File& file);
};
