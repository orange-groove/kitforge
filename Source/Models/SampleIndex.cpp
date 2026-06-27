#include "SampleIndex.h"
#include "InstalledLibrary.h"
#include "../Core/KitForgePaths.h"
#include "../Serialization/ManifestSerializer.h"
#include "../Serialization/KitMigrationService.h"

void SampleIndex::clear()
{
    libraries.clear();
    kits.clear();
    samples.clear();
    notifyChanged();
}

void SampleIndex::addLibrary (InstalledLibrary library)
{
    libraries.push_back (std::move (library));
    notifyChanged();
}

void SampleIndex::addKit (InstalledKit kit)
{
    kits.push_back (std::move (kit));
    notifyChanged();
}

void SampleIndex::addSample (SampleMetadata sample)
{
    samples.push_back (std::move (sample));
    notifyChanged();
}

void SampleIndex::scanKitsOnDisk()
{
    clear();
    KitForgePaths::ensureDirectoryStructure();

    const auto root = KitForgePaths::getKitsRoot();

    for (const auto& entry : root.findChildFiles (juce::File::findDirectories, false))
    {
        const auto manifestFile = entry.getChildFile ("manifest.json");

        if (! manifestFile.existsAsFile())
            continue;

        KitManifest manifest;
        juce::String manifestError;

        if (! ManifestSerializer::readFromFile (manifestFile, manifest, manifestError))
            continue;

        const auto migration = KitMigrationService::validateManifest (manifest);

        if (! migration.ok)
            continue;

        const auto kitJson = entry.getChildFile (manifest.kitFile.isNotEmpty() ? manifest.kitFile : "kit.json");

        if (! kitJson.existsAsFile())
            continue;

        auto lib = InstalledLibrary::fromManifest (manifest, entry,
                                                   manifest.sampleCount,
                                                   manifest.pieceCount);
        lib.sizeBytes = manifest.installSizeBytes;
        addLibrary (lib);

        InstalledKit installedKit;
        installedKit.id = lib.id;
        installedKit.name = lib.name;
        installedKit.source = "kitforge";
        installedKit.libraryId = lib.id;
        installedKit.kitJsonPath = kitJson.getFullPathName();
        installedKit.artworkPath = lib.thumbnailPath;
        installedKit.licensePath = lib.licensePath;
        installedKit.creditsPath = lib.creditsPath;
        installedKit.tags = lib.tags;
        installedKit.installedAtMs = lib.installedAtMs;
        addKit (std::move (installedKit));
    }
}

void SampleIndex::scanLibrariesOnDisk()
{
    scanKitsOnDisk();
}

std::vector<SampleMetadata> SampleIndex::searchByTags (const juce::StringArray& tags,
                                                       DrumPieceType type,
                                                       int maxResults) const
{
    std::vector<std::pair<int, SampleMetadata>> ranked;

    for (const auto& sample : samples)
    {
        if (type != DrumPieceType::accessory && sample.instrumentType != type)
            continue;

        const int score = sample.matchScoreForTags (tags);

        if (score > 0)
            ranked.emplace_back (score, sample);
    }

    std::sort (ranked.begin(), ranked.end(),
               [] (const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<SampleMetadata> results;

    for (int i = 0; i < (int) ranked.size() && i < maxResults; ++i)
        results.push_back (ranked[(size_t) i].second);

    return results;
}

std::vector<SampleMetadata> SampleIndex::searchByInstrumentType (DrumPieceType type, int maxResults) const
{
    std::vector<SampleMetadata> results;

    for (const auto& sample : samples)
    {
        if (sample.instrumentType == type)
        {
            results.push_back (sample);

            if ((int) results.size() >= maxResults)
                break;
        }
    }

    return results;
}

bool SampleIndex::saveCache (const juce::File& file) const
{
    juce::ignoreUnused (file);
    return false;
}

bool SampleIndex::loadCache (const juce::File& file)
{
    juce::ignoreUnused (file);
    return false;
}

void SampleIndex::addListener (std::function<void()> listener)
{
    listeners.push_back (std::move (listener));
}

void SampleIndex::notifyChanged()
{
    for (const auto& listener : listeners)
        if (listener)
            listener();
}
