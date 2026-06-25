#pragma once

#include <JuceHeader.h>
#include "../Models/DrumPieceTypes.h"

/** Parsed metadata from a Kontakt-style sample filename. */
struct KontaktParsedSample
{
    juce::File file;
    juce::String pieceKey;
    juce::String pieceName;
    DrumPieceType pieceType = DrumPieceType::accessory;
    juce::String articulationName;
    int midiNote = 36;
    juce::String chokeGroupId;
    int layerNumber = 0;
    int minVelocity = 1;
    int maxVelocity = 127;
    int roundRobinIndex = 0;
    float confidence = 0.0f;
    bool skipped = false;
};

struct KontaktParseOptions
{
    bool preferSnaresOnOnly = true;
    bool consolidateHiHatClosed = true;
    bool consolidateHiHatOpen = true;
    bool rideCrashAsCrashPiece = true;
};

/** Parses Kontakt drum library filenames: "{instrument} - {part} - {N}.wav". */
class KontaktSampleNameParser
{
public:
    explicit KontaktSampleNameParser (KontaktParseOptions options = {});

    std::optional<KontaktParsedSample> parseFile (const juce::File& file) const;

    /** Assign velocity ranges and round-robin indices for a layer group. */
    static void assignVelocityLayers (std::vector<KontaktParsedSample>& group);

private:
    struct InstrumentDef
    {
        juce::String pieceKey;
        juce::String pieceName;
        DrumPieceType type;
    };

    struct ArticulationDef
    {
        juce::String name;
        int midiNote;
        juce::String chokeGroupId;
    };

    static std::optional<std::tuple<juce::String, juce::String, int>> parseBaseName (const juce::String& baseName);
    static bool shouldSkip (const juce::String& baseName);
    static std::optional<InstrumentDef> matchInstrument (const juce::String& instrument);
    static ArticulationDef matchArticulation (const juce::String& instrument, const juce::String& part,
                                              DrumPieceType type, const KontaktParseOptions& options);

    KontaktParseOptions options;
};
