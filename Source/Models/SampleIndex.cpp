#include "SampleIndex.h"
#include "InstalledLibrary.h"
#include "../Core/KitForgePaths.h"
#include "../Serialization/KitSerializer.h"

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

void SampleIndex::scanLibrariesOnDisk()
{
    clear();
    KitForgePaths::ensureDirectoryStructure();

    const auto root = KitForgePaths::getLibrariesRoot();

    for (const auto& entry : root.findChildFiles (juce::File::findDirectories, false))
    {
        InstalledLibrary lib;
        const auto installJson = entry.getChildFile ("install.json");

        if (installJson.existsAsFile())
        {
            lib = InstalledLibrary::fromInstallJson (installJson);
            lib.sizeBytes = entry.getSize();
        }
        else
        {
            lib.id = entry.getFileName();
            lib.name = entry.getFileName();
            lib.installedPath = entry.getFullPathName();
            lib.rootPath = lib.installedPath;
            lib.installedAtMs = entry.getLastModificationTime().toMilliseconds();
            lib.sizeBytes = entry.getSize();

            const auto kitForgeSub = entry.getChildFile ("kitforge");

            if (kitForgeSub.isDirectory())
            {
                lib.kitForgePath = kitForgeSub.getFullPathName();
                lib.format = "sfz";
            }
            else if (entry.getChildFile ("kit.json").existsAsFile())
            {
                lib.kitForgePath = lib.installedPath;
                lib.format = "kitforgepack";
            }
            else
            {
                lib.format = "unknown";
            }
        }

        if (lib.name.isEmpty())
            lib.name = lib.id;

        addLibrary (lib);

        const auto kitJson = lib.getKitJsonFile();

        if (kitJson.existsAsFile())
        {
            InstalledKit kit;
            kit.id = lib.id;
            kit.name = lib.name;
            kit.source = installJson.existsAsFile() ? "catalog" : "import";
            kit.libraryId = lib.id;
            kit.kitJsonPath = kitJson.getFullPathName();
            kit.artworkPath = juce::File (lib.kitForgePath).getChildFile ("artwork").getFullPathName();
            kit.licensePath = lib.licensePath.isNotEmpty()
                                ? lib.licensePath
                                : juce::File (lib.kitForgePath).getChildFile ("license.txt").getFullPathName();
            kit.creditsPath = lib.creditsPath.isNotEmpty()
                                ? lib.creditsPath
                                : juce::File (lib.kitForgePath).getChildFile ("credits.txt").getFullPathName();
            kit.tags = lib.tags;
            kit.installedAtMs = lib.installedAtMs;
            addKit (std::move (kit));
        }
    }
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
            ranked.push_back ({ score, sample });
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
    auto* root = new juce::DynamicObject();
    root->setProperty ("version", 1);
    root->setProperty ("sampleCount", (int) samples.size());
    return file.replaceWithText (juce::JSON::toString (juce::var (root), true));
}

bool SampleIndex::loadCache (const juce::File& file)
{
    juce::var parsed;

    if (! file.existsAsFile() || juce::JSON::parse (file.loadFileAsString(), parsed).failed())
        return false;

    juce::ignoreUnused (parsed);
    return true;
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
