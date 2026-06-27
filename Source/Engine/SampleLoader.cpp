#include "SampleLoader.h"
#include <atomic>
#include <thread>

SampleLoader::SampleLoader()
{
    formatManager.registerBasicFormats();
}

void SampleLoader::prepare (double sampleRate)
{
    targetSampleRate = sampleRate;
}

const LoadedSample* SampleLoader::getCached (const juce::String& filePath) const
{
    if (filePath.isEmpty())
        return nullptr;

    const auto it = cache.find (filePath.toStdString());

    if (it != cache.end())
        return it->second.get();

    return nullptr;
}

const LoadedSample* SampleLoader::load (const juce::String& filePath)
{
    if (filePath.isEmpty())
        return nullptr;

    if (const auto* cached = getCached (filePath))
        return cached;

    const auto key = filePath.toStdString();

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

void SampleLoader::loadMany (const juce::StringArray& filePaths)
{
    std::vector<juce::String> todo;
    todo.reserve ((size_t) filePaths.size());

    for (const auto& path : filePaths)
    {
        if (path.isEmpty())
            continue;

        if (cache.find (path.toStdString()) == cache.end())
            todo.push_back (path);
    }

    if (todo.empty())
        return;

    // Decode in parallel; cache map is touched only on this thread afterwards.
    std::vector<std::unique_ptr<LoadedSample>> decoded (todo.size());
    std::atomic<size_t> next { 0 };

    const auto worker = [&]()
    {
        for (;;)
        {
            const size_t i = next.fetch_add (1);

            if (i >= todo.size())
                return;

            const juce::File file (todo[i]);

            if (file.existsAsFile())
                decoded[i] = decodeFile (file);
        }
    };

    const int hw = (int) std::thread::hardware_concurrency();
    const int numThreads = juce::jlimit (1, 8, hw > 0 ? hw : 4);

    std::vector<std::thread> threads;
    threads.reserve ((size_t) numThreads - 1);

    for (int t = 0; t < numThreads - 1; ++t)
        threads.emplace_back (worker);

    worker();

    for (auto& th : threads)
        th.join();

    for (size_t i = 0; i < todo.size(); ++i)
        if (decoded[i] != nullptr)
            cache[todo[i].toStdString()] = std::move (decoded[i]);
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
