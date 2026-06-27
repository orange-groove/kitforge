#include "SampleIndexService.h"
#include "DrumPieceTypes.h"
#include "../Serialization/KitForgePackageReader.h"

namespace
{
    juce::String stableHash (const juce::String& s)
    {
        return juce::String::toHexString (s.hashCode64());
    }

    juce::String slug (juce::String text)
    {
        text = text.trim().toLowerCase();
        juce::String out;

        for (auto c : text)
        {
            if (juce::CharacterFunctions::isLetterOrDigit (c))
                out << c;
            else if (c == ' ' || c == '-' || c == '_')
                out << '-';
        }

        while (out.contains ("--"))
            out = out.replace ("--", "-");

        return out.trimCharactersAtStart ("-").trimCharactersAtEnd ("-");
    }

    juce::String relativePathOf (const juce::File& file, const juce::File& root)
    {
        if (! root.exists())
            return file.getFullPathName();

        auto rel = file.getRelativePathFrom (root);
        return rel.replaceCharacter ('\\', '/');
    }

    int countRoundRobins (const std::vector<SampleLayer>& layers)
    {
        int maxRr = 0;

        for (const auto& layer : layers)
            maxRr = juce::jmax (maxRr, (int) layer.roundRobins.samples.size());

        return maxRr;
    }

    int countSamples (const std::vector<SampleLayer>& layers)
    {
        int total = 0;

        for (const auto& layer : layers)
            total += (int) layer.roundRobins.samples.size();

        return total;
    }
}

juce::String SampleIndexService::computeSignature() const
{
    juce::String sig;

    for (const auto& lib : index.getLibraries())
        sig << lib.id << ':' << juce::String (lib.installedAtMs) << ':'
            << juce::String (lib.sampleCount) << ';';

    return sig;
}

void SampleIndexService::rebuildIfStale()
{
    const juce::ScopedLock sl (lock);

    if (computeSignature() != lastSignature || sampleSets.empty())
        rebuildIndex();
}

void SampleIndexService::rebuildIndex()
{
    const juce::ScopedLock sl (lock);
    sampleSets.clear();
    missingSampleCount = 0;

    for (const auto& lib : index.getLibraries())
    {
        const auto root = lib.getRoot();

        if (! root.isDirectory())
            continue;

        auto loaded = KitForgePackageReader::loadInstalledKit (root);

        if (! loaded.success)
            continue;

        loaded.kit.resolveSamplePaths (root);

        for (const auto& piece : loaded.kit.getPieces())
        {
            for (const auto& art : piece.articulations)
            {
                if (art.layers.empty())
                    continue;

                if (countSamples (art.layers) == 0)
                    continue;

                SampleSet set;
                set.libraryId = lib.id;
                set.instrumentType = piece.type;
                set.articulation = art.name.isNotEmpty() ? art.name : juce::String ("Default");
                set.displayName = (piece.name.isNotEmpty() ? piece.name
                                                           : defaultPieceDisplayName (piece.type))
                                + " " + set.articulation;
                set.sourceKitName = lib.name.isNotEmpty() ? lib.name : loaded.kit.kitName;
                set.sourcePackPath = lib.installedPath;

                // Deterministic id: library + instrument + articulation + source articulation id.
                set.id = "ss-" + stableHash (lib.id + "|" + drumPieceTypeToString (piece.type)
                                             + "|" + slug (set.articulation) + "|" + art.id);

                set.tags = lib.tags;
                set.tags.addIfNotAlreadyThere (drumPieceTypeToString (piece.type));
                set.tags.addIfNotAlreadyThere (slug (set.articulation));

                // Deep-copy layers and attach a resolvable SampleRef to every sample.
                bool allPresent = true;

                for (const auto& srcLayer : art.layers)
                {
                    SampleLayer layer;
                    layer.id = SampleLayer::makeId();
                    layer.minVelocity = srcLayer.minVelocity;
                    layer.maxVelocity = srcLayer.maxVelocity;

                    for (const auto& srcSample : srcLayer.roundRobins.samples)
                    {
                        DrumSample sample = srcSample;
                        sample.id = DrumSample::makeId();

                        const juce::File abs (srcSample.filePath);
                        const auto rel = relativePathOf (abs, root);

                        if (! abs.existsAsFile())
                        {
                            allPresent = false;
                            ++missingSampleCount;
                        }

                        SampleRef ref;
                        ref.libraryId = lib.id;
                        ref.sampleSetId = set.id;
                        ref.sampleId = "smp-" + stableHash (lib.id + "|" + rel);
                        ref.relativePath = rel;
                        ref.fallbackPath = abs.getFullPathName();
                        ref.sourceType = SampleRef::SourceType::installedLibrary;
                        sample.sampleRef = ref;

                        layer.roundRobins.addSample (std::move (sample));
                    }

                    set.layers.push_back (std::move (layer));
                }

                set.sampleCount = countSamples (set.layers);
                set.velocityLayerCount = (int) set.layers.size();
                set.roundRobinCount = countRoundRobins (set.layers);
                set.allSamplesPresent = allPresent;

                for (const auto& layer : set.layers)
                    if (! layer.roundRobins.samples.empty())
                    {
                        set.previewSampleRef = layer.roundRobins.samples.front().sampleRef;
                        break;
                    }

                sampleSets.push_back (std::move (set));
            }
        }
    }

    lastSignature = computeSignature();
}

std::optional<SampleSet> SampleIndexService::findSampleSetById (const juce::String& id) const
{
    const juce::ScopedLock sl (lock);

    for (const auto& set : sampleSets)
        if (set.id == id)
            return set;

    return std::nullopt;
}

std::vector<SampleSet> SampleIndexService::searchSampleSets (const SampleSetQuery& query) const
{
    const juce::ScopedLock sl (lock);

    const auto wantType = query.instrumentType.trim().toLowerCase();
    const auto wantArt  = query.articulation.trim().toLowerCase();
    const auto wantText = query.text.trim().toLowerCase();

    std::vector<std::pair<int, const SampleSet*>> ranked;

    for (const auto& set : sampleSets)
    {
        if (query.libraryId.isNotEmpty() && set.libraryId != query.libraryId)
            continue;

        if (query.minVelocityLayers > 0 && set.velocityLayerCount < query.minVelocityLayers)
            continue;

        if (query.minRoundRobins > 0 && set.roundRobinCount < query.minRoundRobins)
            continue;

        const auto typeStr = drumPieceTypeToString (set.instrumentType).toLowerCase();
        const auto artStr  = set.articulation.toLowerCase();

        // Hard instrument-type filter (when requested) keeps the swap list compatible.
        if (wantType.isNotEmpty() && typeStr != wantType)
            continue;

        int score = 10; // baseline for a passing set

        if (wantType.isNotEmpty() && typeStr == wantType)
            score += 100;

        if (wantArt.isNotEmpty())
        {
            if (artStr == wantArt)
                score += 80;
            else if (artStr.contains (wantArt) || wantArt.contains (artStr))
                score += 30;
        }

        for (const auto& tag : query.tags)
            if (set.tags.contains (tag, true))
                score += 15;

        if (wantText.isNotEmpty())
        {
            const auto hay = (set.displayName + " " + set.sourceKitName + " "
                              + set.tags.joinIntoString (" ")).toLowerCase();

            if (hay.contains (wantText))
                score += 40;
            else
            {
                // Require at least one token to match when free-text is supplied.
                bool anyToken = false;

                for (const auto& token : juce::StringArray::fromTokens (wantText, " ", ""))
                    if (token.isNotEmpty() && hay.contains (token))
                    {
                        anyToken = true;
                        score += 10;
                    }

                if (! anyToken)
                    continue;
            }
        }

        // More velocity layers / round robins are richer; nudge them up.
        score += juce::jmin (set.velocityLayerCount, 8) * 2;
        score += juce::jmin (set.roundRobinCount, 8);

        // Missing samples rank last.
        if (! set.allSamplesPresent)
            score -= 500;

        ranked.emplace_back (score, &set);
    }

    std::stable_sort (ranked.begin(), ranked.end(),
                      [] (const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<SampleSet> results;
    const int limit = query.maxResults > 0 ? query.maxResults : (int) ranked.size();

    for (int i = 0; i < (int) ranked.size() && i < limit; ++i)
        results.push_back (*ranked[(size_t) i].second);

    return results;
}

std::vector<SampleSet> SampleIndexService::getCompatibleSampleSets (const juce::String& instrumentType,
                                                                    const juce::String& articulation) const
{
    SampleSetQuery query;
    query.instrumentType = instrumentType;
    query.articulation = articulation;
    return searchSampleSets (query);
}

std::optional<ResolvedSample> SampleIndexService::resolveSampleRef (const SampleRef& ref) const
{
    if (! ref.hasRef())
        return std::nullopt;

    ResolvedSample resolved;
    resolved.sampleRef = ref;

    auto tryFile = [&resolved] (const juce::File& f) -> bool
    {
        if (f.existsAsFile())
        {
            resolved.absoluteFilePath = f.getFullPathName();
            resolved.exists = true;
            return true;
        }

        return false;
    };

    // 1) Installed library: relative path inside the library root.
    if (ref.sourceType == SampleRef::SourceType::installedLibrary || ref.libraryId.isNotEmpty())
    {
        for (const auto& lib : index.getLibraries())
        {
            if (lib.id != ref.libraryId)
                continue;

            const auto root = lib.getRoot();

            if (ref.relativePath.isNotEmpty() && tryFile (root.getChildFile (ref.relativePath)))
                return resolved;

            break;
        }
    }

    // 2) Absolute / embedded fallback.
    if (ref.fallbackPath.isNotEmpty() && tryFile (juce::File (ref.fallbackPath)))
        return resolved;

    if (ref.relativePath.isNotEmpty() && tryFile (juce::File (ref.relativePath)))
        return resolved;

    resolved.exists = false;
    resolved.errorMessage = ref.libraryId.isNotEmpty()
                                ? "Sample not found in library \"" + ref.libraryId + "\": " + ref.relativePath
                                : "Sample not found: " + ref.relativePath;
    return resolved;
}

bool SampleIndexService::isLibraryInstalled (const juce::String& libraryId) const
{
    for (const auto& lib : index.getLibraries())
        if (lib.id == libraryId)
            return true;

    return false;
}

juce::String SampleIndexService::getLibraryLicense (const juce::String& libraryId) const
{
    for (const auto& lib : index.getLibraries())
        if (lib.id == libraryId)
            return lib.license;

    return {};
}

juce::String SampleIndexService::getLibraryName (const juce::String& libraryId) const
{
    for (const auto& lib : index.getLibraries())
        if (lib.id == libraryId)
            return lib.name;

    return {};
}

int SampleIndexService::resolveKitSampleRefs (KitModel& kit) const
{
    int resolvedCount = 0;

    for (auto& piece : kit.getPiecesMutable())
    {
        for (auto& art : piece.articulations)
        {
            for (auto& layer : art.layers)
            {
                for (auto& sample : layer.roundRobins.samples)
                {
                    if (! sample.sampleRef.hasRef())
                        continue;

                    const juce::File current (sample.filePath);

                    if (current.existsAsFile())
                        continue;

                    if (auto resolved = resolveSampleRef (sample.sampleRef))
                    {
                        if (resolved->exists)
                        {
                            sample.filePath = resolved->absoluteFilePath;
                            ++resolvedCount;
                        }
                    }
                }
            }
        }
    }

    return resolvedCount;
}
