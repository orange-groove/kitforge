#pragma once

#include "../Models/KitModel.h"

/** Maps parsed library samples onto an existing kit layout (preserves piece IDs and positions). */
class SampleLibraryMapper
{
public:
    struct SourceEntry
    {
        juce::String key;
        juce::String pieceName;
        juce::String articulationName;
        DrumPieceType type = DrumPieceType::accessory;
        int midiNote = 36;
        int sampleCount = 0;
        juce::String sourcePieceId;
        juce::String sourceArticulationId;
    };

    struct TargetSlot
    {
        juce::String key;
        juce::String label;
        juce::String pieceId;
        juce::String articulationId;
    };

    struct Mapping
    {
        juce::String sourceKey;
        juce::String targetKey;
        bool enabled = true;
    };

    static juce::String makeKey (const juce::String& pieceId, const juce::String& articulationId);
    static juce::Array<SourceEntry> listLibrarySources (const KitModel& libraryCatalog);
    static juce::Array<TargetSlot> listTargetSlots (const KitModel& targetKit);
    static juce::Array<Mapping> suggestMappings (const KitModel& targetKit, const KitModel& libraryCatalog);

    /** Copies sample layers from library articulations onto matched target articulations. */
    static int applyMappings (KitModel& targetKit,
                              const KitModel& libraryCatalog,
                              const juce::Array<Mapping>& mappings);
};
