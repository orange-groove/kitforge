#pragma once

#include "ImportResult.h"
#include "../Models/SampleMetadata.h"
#include "../Models/KitModel.h"

/** Builds KitModel from parsed sample metadata (shared by loose WAV and SFZ importers). */
class KitModelBuilder
{
public:
    static KitModel buildFromMetadata (const juce::String& kitName,
                                       const std::vector<SampleMetadata>& samples,
                                       std::vector<ImportWarning>& warnings);

private:
    struct PieceKey
    {
        DrumPieceType type = DrumPieceType::accessory;
        int index = 0;

        bool operator== (const PieceKey& other) const
        {
            return type == other.type && index == other.index;
        }
    };

    struct LayerKey
    {
        int minVelocity = 1;
        int maxVelocity = 127;

        bool operator== (const LayerKey& other) const
        {
            return minVelocity == other.minVelocity && maxVelocity == other.maxVelocity;
        }
    };

    static juce::String pieceKeyToString (const PieceKey& key);
    static juce::String pieceDisplayName (const PieceKey& key);
    static juce::String articulationKey (const SampleMetadata& meta);
    static void applyPieceDefaults (DrumPiece& piece, const PieceKey& key);
    static void addSampleToModel (KitModel& model,
                                  juce::HashMap<juce::String, DrumPiece*>& piecesByKey,
                                  const SampleMetadata& meta,
                                  std::vector<ImportWarning>& warnings);
};
