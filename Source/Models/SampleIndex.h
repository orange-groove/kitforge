#pragma once

#include "InstalledKit.h"
#include "InstalledLibrary.h"
#include "KitModel.h"
#include "SampleMetadata.h"
#include <functional>
#include <vector>

/** Indexes installed libraries/kits and supports tag-based sample search for AI builder. */
class SampleIndex
{
public:
    void clear();

    void addLibrary (InstalledLibrary library);
    void addKit (InstalledKit kit);
    void addSample (SampleMetadata sample);

    const std::vector<InstalledLibrary>& getLibraries() const { return libraries; }
    const std::vector<InstalledKit>& getKits() const { return kits; }
    const std::vector<SampleMetadata>& getSamples() const { return samples; }

    /** Scan ~/Documents/KitForge/Kits and rebuild the index. */
    void scanKitsOnDisk();

    /** @deprecated Use scanKitsOnDisk(). */
    void scanLibrariesOnDisk();

    std::vector<SampleMetadata> searchByTags (const juce::StringArray& tags,
                                              DrumPieceType type = DrumPieceType::accessory,
                                              int maxResults = 32) const;

    std::vector<SampleMetadata> searchByInstrumentType (DrumPieceType type, int maxResults = 32) const;

    bool saveCache (const juce::File& file) const;
    bool loadCache (const juce::File& file);

    void addListener (std::function<void()> listener);
    void notifyChanged();

private:
    std::vector<InstalledLibrary> libraries;
    std::vector<InstalledKit> kits;
    std::vector<SampleMetadata> samples;
    std::vector<std::function<void()>> listeners;
};
