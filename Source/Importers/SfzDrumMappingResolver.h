#pragma once

#include <JuceHeader.h>
#include "../Models/SampleMetadata.h"

/** Maps SFZ mapping filenames and sample paths to drum piece metadata. */
struct SfzDrumMappingHint
{
    DrumPieceType type = DrumPieceType::accessory;
    int index = 0;
    juce::String articulation;
    float confidence = 0.0f;
};

class SfzDrumMappingResolver
{
public:
    /** Resolve from an included mapping file path (e.g. mappings/smdrums_sfz_kick.sfz). */
    static SfzDrumMappingHint fromMappingSource (const juce::String& mappingSource);

    /** Resolve from sample file path and filename tokens. */
    static SfzDrumMappingHint fromSamplePath (const juce::File& sampleFile);
};
