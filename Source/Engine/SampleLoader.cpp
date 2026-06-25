#include "SampleLoader.h"

SampleLoader::SampleLoader()
{
    formatManager.registerBasicFormats();
}

void SampleLoader::prepare (double sampleRate)
{
    targetSampleRate = sampleRate;
}

const LoadedSample* SampleLoader::load (const juce::String& filePath)
{
    if (filePath.isEmpty())
        return nullptr;

    const auto key = filePath.toStdString();
    const auto it = cache.find (key);

    if (it != cache.end())
        return it->second.get();

    const juce::File file (filePath);

    if (! file.existsAsFile())
        return nullptr;

    auto loaded = decodeFile (file);

    if (loaded == nullptr)
        return nullptr;

    const auto* ptr = loaded.get();
    cache[key] = std::move (loaded);
    return ptr;
}

void SampleLoader::unloadAll()
{
    cache.clear();
}

void SampleLoader::pruneUnused (const juce::StringArray& referencedPaths)
{
    for (auto it = cache.begin(); it != cache.end();)
    {
        if (! referencedPaths.contains (it->second->filePath))
            it = cache.erase (it);
        else
            ++it;
    }
}

std::unique_ptr<LoadedSample> SampleLoader::decodeFile (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr)
        return nullptr;

    auto loaded = std::make_unique<LoadedSample>();
    loaded->filePath = file.getFullPathName();
    loaded->sampleRate = reader->sampleRate;
    loaded->lengthInSamples = (int) reader->lengthInSamples;

    loaded->buffer.setSize ((int) reader->numChannels, loaded->lengthInSamples);
    reader->read (&loaded->buffer, 0, loaded->lengthInSamples, 0, true, true);

    return loaded;
}
