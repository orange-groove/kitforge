#pragma once

#include <JuceHeader.h>
#include "SampleIndex.h"
#include "SampleSet.h"
#include "ResolvedSample.h"
#include "KitModel.h"
#include <optional>
#include <vector>

/** Query for searching the global sample-set index. Empty/zero fields mean "any". */
struct SampleSetQuery
{
    juce::String instrumentType;   // e.g. "kick" (empty = any)
    juce::String articulation;     // e.g. "center" (empty = any)
    juce::StringArray tags;
    juce::String libraryId;        // restrict to one library (empty = any)
    juce::String text;             // free-text fuzzy match (empty = any)
    int minVelocityLayers = 0;
    int minRoundRobins = 0;
    int maxResults = 200;
};

/**
    Builds and serves a global, cross-library index of swappable `SampleSet`s by
    reading every installed `.kitforge` kit's `kit.json`. Used by the sample-swap
    UI to search compatible sets and to resolve `SampleRef`s into absolute paths.

    Backed by the existing `SampleIndex` (which owns the on-disk library list).
*/
class SampleIndexService
{
public:
    explicit SampleIndexService (SampleIndex& indexIn) : index (indexIn) {}

    /** Rescans installed libraries and rebuilds the in-memory SampleSet list. */
    void rebuildIndex();

    /** Rebuilds only if the installed-library set changed since the last build. */
    void rebuildIfStale();

    const std::vector<SampleSet>& getSampleSets() const { return sampleSets; }

    std::vector<SampleSet> searchSampleSets (const SampleSetQuery& query) const;

    std::vector<SampleSet> getCompatibleSampleSets (const juce::String& instrumentType,
                                                    const juce::String& articulation) const;

    std::optional<SampleSet> findSampleSetById (const juce::String& id) const;

    std::optional<ResolvedSample> resolveSampleRef (const SampleRef& ref) const;

    bool isLibraryInstalled (const juce::String& libraryId) const;
    juce::String getLibraryLicense (const juce::String& libraryId) const;
    juce::String getLibraryName (const juce::String& libraryId) const;

    /** Fills empty/missing `filePath`s on a kit from each sample's `SampleRef`. */
    int resolveKitSampleRefs (KitModel& kit) const;

    int getLibraryCount() const { return (int) index.getLibraries().size(); }
    int getSampleSetCount() const { return (int) sampleSets.size(); }
    int getMissingSampleCount() const { return missingSampleCount; }

private:
    SampleIndex& index;
    std::vector<SampleSet> sampleSets;
    int missingSampleCount = 0;
    juce::String lastSignature;
    mutable juce::CriticalSection lock; // guards sampleSets across background threads

    juce::String computeSignature() const;
};
