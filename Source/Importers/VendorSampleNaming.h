#pragma once

#include <JuceHeader.h>
#include "../Models/SampleMetadata.h"
#include <vector>

/** Parses Native Instruments / Kontakt-style filenames and resolves layer indices
    into velocity ranges and round-robin slots.

    Common pattern: "{Instrument} - {Part} - {Layer}.wav"
      Layer 1–9  → velocity layers, OR round-robin within 1–90 when layers ≥ 10 exist
      Layer ≥ 10 → narrow high-velocity layer centered on the layer number */
namespace VendorSampleNaming
{
    /** When true, meta.layerIndex and (usually) piece/articulation are set. */
    bool tryApply (const juce::File& file, SampleMetadata& meta);

    /** Groups samples by piece+articulation and assigns min/max velocity + roundRobinIndex. */
    void resolveLayerAssignments (std::vector<SampleMetadata>& samples);
}
