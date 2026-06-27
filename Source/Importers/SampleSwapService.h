#pragma once

#include "../Models/KitModel.h"
#include "../Models/SampleSet.h"

/**
    Replaces sample content on a kit with a `SampleSet` drawn from another
    installed library, preserving the target's identity/layout/mapping.

    Three granularities:
      - articulation : replace one articulation's layers
      - layer        : replace one velocity layer's round robins
      - piece        : replace every articulation on the piece
*/
class SampleSwapService
{
public:
    enum class Mode { articulation, piece, layer };

    struct Target
    {
        juce::String pieceId;
        juce::String articulationId;
        juce::String layerId;
        Mode mode = Mode::articulation;
    };

    struct Options
    {
        bool useSourceName = false;           // adopt the source set's display name
        bool useSourceMidi = false;           // adopt the source mapping (rootMidiNote)
        bool useSourceVelocityRanges = false; // layer mode: adopt source min/max velocity
    };

    struct Result
    {
        bool success = false;
        juce::String errorMessage;
        int replacedCount = 0;
    };

    static Result swap (KitModel& kit,
                        const SampleSet& set,
                        const Target& target,
                        const Options& options);

private:
    static std::vector<SampleLayer> cloneLayers (const std::vector<SampleLayer>& src);
};
