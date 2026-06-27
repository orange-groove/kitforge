#include "SfzDrumMappingResolver.h"
#include <initializer_list>

// Generic, library-agnostic drum classifier.
//
// Instead of hard-coding one library's file names, we scan the supplied text
// (an SFZ #include name, or a sample filename + its parent folder) for common
// drum vocabulary and synonyms. This is the same idea SampleNameParser uses,
// shared here so the SFZ importer can classify regions whose MIDI keymap is
// non-standard.
namespace
{
    juce::String normalise (const juce::String& raw)
    {
        // Lower-case and turn every separator into a space so we can match
        // whole-ish words ("... bd ...") as well as substrings ("crash16").
        auto s = raw.toLowerCase();

        for (auto sep : { '\\', '/', '_', '-', '.', '(', ')', '[', ']' })
            s = s.replaceCharacter (sep, ' ');

        return " " + s.trim() + " ";
    }

    bool any (const juce::String& hay, std::initializer_list<const char*> needles)
    {
        for (auto* n : needles)
            if (hay.contains (n))
                return true;

        return false;
    }

    SfzDrumMappingHint make (DrumPieceType type, const char* articulation, float confidence)
    {
        SfzDrumMappingHint hint;
        hint.type = type;
        hint.index = 0; // grouping/index is resolved downstream from the file stem.
        hint.articulation = articulation;
        hint.confidence = confidence;
        return hint;
    }

    SfzDrumMappingHint classify (const juce::String& text)
    {
        if (text.isEmpty())
            return {};

        const auto n = normalise (text);

        // Articulation-only words that strongly imply a specific instrument.
        if (any (n, { "sidestick", "side stick", "crossstick", "cross stick", " xstick", " x stick" }))
            return make (DrumPieceType::snare, "Sidestick", 0.95f);

        if (any (n, { "rimshot", "rim shot", " rimclick", "rim click" }))
            return make (DrumPieceType::snare, "Rimshot", 0.95f);

        if (any (n, { "kick", " kik", "bassdrum", "bass drum", " bd ", " bd0", " bd1" }))
            return make (DrumPieceType::kick, "Center", 0.95f);

        if (any (n, { "snare", " snr", " sd " }))
        {
            if (any (n, { " rim" }))
                return make (DrumPieceType::snare, "Rimshot", 0.9f);

            return make (DrumPieceType::snare, "Center", 0.9f);
        }

        if (any (n, { "hihat", "hi hat", "highhat", " hat", " hh" }))
        {
            if (any (n, { "open" }))         return make (DrumPieceType::hiHat, "Open", 0.9f);
            if (any (n, { "pedal", "foot", "chick" })) return make (DrumPieceType::hiHat, "Pedal", 0.9f);
            if (any (n, { "loose", "half" })) return make (DrumPieceType::hiHat, "Open", 0.9f);
            return make (DrumPieceType::hiHat, "Closed", 0.9f);
        }

        if (any (n, { "china", "chinese" }))
            return make (DrumPieceType::china, "Center", 0.9f);

        if (any (n, { "splash" }))
            return make (DrumPieceType::splash, "Center", 0.9f);

        if (any (n, { "ride", " rd " }))
        {
            if (any (n, { "bell" })) return make (DrumPieceType::ride, "Bell", 0.9f);
            if (any (n, { "edge", "shoulder" })) return make (DrumPieceType::ride, "Edge", 0.9f);
            return make (DrumPieceType::ride, "Bow", 0.9f);
        }

        if (any (n, { "crash", "cymbal", " cym", " cr1", " cr2" }))
        {
            if (any (n, { "edge" }))  return make (DrumPieceType::crash, "Edge", 0.85f);
            if (any (n, { "choke" })) return make (DrumPieceType::crash, "Choke", 0.85f);
            return make (DrumPieceType::crash, "Center", 0.85f);
        }

        if (any (n, { "floortom", "floor tom", " floor", " ftom", " ft1", " ft2", " ft " }))
            return make (DrumPieceType::floorTom, "Center", 0.9f);

        if (any (n, { "racktom", "rack tom", " rack", " tom", " rt1", " rt2" }))
            return make (DrumPieceType::rackTom, "Center", 0.85f);

        return {};
    }
}

SfzDrumMappingHint SfzDrumMappingResolver::fromMappingSource (const juce::String& mappingSource)
{
    if (mappingSource.isEmpty())
        return {};

    return classify (juce::File (mappingSource).getFileNameWithoutExtension());
}

SfzDrumMappingHint SfzDrumMappingResolver::fromSamplePath (const juce::File& sampleFile)
{
    // Combine the file name with its immediate folder for extra context
    // (e.g. ".../Crash 16/edge_rr1.wav").
    const auto context = sampleFile.getFileNameWithoutExtension()
                       + " " + sampleFile.getParentDirectory().getFileName();

    return classify (context);
}
