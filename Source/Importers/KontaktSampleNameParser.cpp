#include "KontaktSampleNameParser.h"
#include <optional>
#include <tuple>

namespace
{
    juce::String lower (const juce::String& s) { return s.trim().toLowerCase(); }

    bool isSnaresOffPart (const juce::String& part)
    {
        return part.contains ("snares off");
    }

    juce::String shellHitName (const juce::String& instrument, const juce::String& part)
    {
        if (instrument == "snare" && part == "snares on")
            return "Center";

        if (instrument == "rimshot" && part == "snares on")
            return "Rimshot";

        if (instrument == "stickshot" && part == "snares on")
            return "Stickshot";

        if (instrument == "xstick" && part == "snares on")
            return "Cross Stick";

        if ((instrument == "kick" || instrument == "bop kick") && part == "snares on")
            return "Center";

        if ((instrument == "rack tom" || instrument == "floor tom") && part == "snares on")
            return "Hit";

        return {};
    }
}

KontaktSampleNameParser::KontaktSampleNameParser (KontaktParseOptions opts)
    : options (std::move (opts))
{
}

bool KontaktSampleNameParser::shouldSkip (const juce::String& baseName)
{
    const auto name = lower (baseName);
    return name.startsWith ("turn snare on")
        || name.startsWith ("turn off snares");
}

std::optional<std::tuple<juce::String, juce::String, int>>
KontaktSampleNameParser::parseBaseName (const juce::String& baseName)
{
    const auto trimmed = baseName.trim();

    const int lastDash = trimmed.lastIndexOfChar ('-');

    if (lastDash < 0)
        return std::nullopt;

    const auto layerStr = trimmed.substring (lastDash + 1).trim();

    if (! layerStr.containsOnly ("0123456789"))
        return std::nullopt;

    const int layer = layerStr.getIntValue();
    const auto beforeLayer = trimmed.substring (0, lastDash).trim();

    const int middleDash = beforeLayer.lastIndexOfChar ('-');

    if (middleDash >= 0)
    {
        return std::make_tuple (lower (beforeLayer.substring (0, middleDash).trim()),
                                lower (beforeLayer.substring (middleDash + 1).trim()),
                                layer);
    }

    return std::make_tuple (lower (beforeLayer), juce::String(), layer);
}

std::optional<KontaktSampleNameParser::InstrumentDef>
KontaktSampleNameParser::matchInstrument (const juce::String& instrument)
{
    struct Row { const char* key; InstrumentDef def; };
    static const Row rows[] = {
        { "kick",      { "kick",      "Kick",      DrumPieceType::kick } },
        { "bop kick",  { "bop_kick",  "Bop Kick",  DrumPieceType::kick } },
        { "snare",     { "snare",     "Snare",     DrumPieceType::snare } },
        { "rimshot",   { "snare",     "Snare",     DrumPieceType::snare } },
        { "stickshot", { "snare",     "Snare",     DrumPieceType::snare } },
        { "xstick",    { "snare",     "Snare",     DrumPieceType::snare } },
        { "rack tom",  { "rack_tom",  "Rack Tom",  DrumPieceType::rackTom } },
        { "floor tom", { "floor_tom", "Floor Tom", DrumPieceType::floorTom } },
        { "hihat",     { "hihat",     "Hi-Hat",    DrumPieceType::hiHat } },
        { "ride",      { "ride",      "Ride",      DrumPieceType::ride } },
        { "flat ride", { "flat_ride", "Flat Ride", DrumPieceType::ride } },
    };

    for (const auto& row : rows)
    {
        if (instrument == row.key)
            return row.def;
    }

    return std::nullopt;
}

KontaktSampleNameParser::ArticulationDef
KontaktSampleNameParser::matchArticulation (const juce::String& instrument, const juce::String& part,
                                             DrumPieceType type, const KontaktParseOptions& options)
{
    if (options.consolidateHiHatClosed && instrument == "hihat")
    {
        if (part == "close" || part == "closed" || part == "closed side")
            return { "Closed", 42, "hihat" };
    }

    if (options.consolidateHiHatOpen && instrument == "hihat")
    {
        if (part == "open" || part.startsWith ("opened "))
            return { "Open", 46, "hihat" };
    }

    if (options.preferSnaresOnOnly)
    {
        if (const auto name = shellHitName (instrument, part); name.isNotEmpty())
        {
            KontaktParseOptions tableOpts = options;
            tableOpts.preferSnaresOnOnly = false;
            const auto base = matchArticulation (instrument, part, type, tableOpts);
            return { name, base.midiNote, base.chokeGroupId };
        }
    }

    struct Key { juce::String inst; juce::String part; };
    static const std::pair<Key, ArticulationDef> table[] = {
        { { "kick", "snares off" },      { "Center (Snares Off)", 36, "" } },
        { { "kick", "snares on" },       { "Center (Snares On)", 36, "" } },
        { { "bop kick", "snares off" },  { "Bop (Snares Off)", 35, "" } },
        { { "bop kick", "snares on" },   { "Bop (Snares On)", 35, "" } },
        { { "snare", "snares off" },     { "Center (Snares Off)", 38, "" } },
        { { "snare", "snares on" },      { "Center (Snares On)", 38, "" } },
        { { "rimshot", "snares off" },   { "Rimshot (Snares Off)", 40, "" } },
        { { "rimshot", "snares on" },    { "Rimshot (Snares On)", 40, "" } },
        { { "stickshot", "snares off" }, { "Stickshot (Snares Off)", 38, "" } },
        { { "stickshot", "snares on" },  { "Stickshot (Snares On)", 38, "" } },
        { { "xstick", "snares off" },    { "Cross Stick (Snares Off)", 37, "" } },
        { { "xstick", "snares on" },     { "Cross Stick (Snares On)", 37, "" } },
        { { "rack tom", "snares off" },  { "Hit (Snares Off)", 48, "" } },
        { { "rack tom", "snares on" },   { "Hit (Snares On)", 48, "" } },
        { { "floor tom", "snares off" }, { "Hit (Snares Off)", 43, "" } },
        { { "floor tom", "snares on" },  { "Hit (Snares On)", 43, "" } },
        { { "hihat", "close" },          { "Closed Tight", 42, "hihat" } },
        { { "hihat", "closed" },         { "Closed", 42, "hihat" } },
        { { "hihat", "closed side" },    { "Closed Side", 42, "hihat" } },
        { { "hihat", "open" },           { "Open", 46, "hihat" } },
        { { "hihat", "opened 1" },       { "Opened 1", 46, "hihat" } },
        { { "hihat", "opened 2" },       { "Opened 2", 46, "hihat" } },
        { { "hihat", "opened 3" },       { "Opened 3", 46, "hihat" } },
        { { "hihat", "opened 4" },       { "Opened 4", 46, "hihat" } },
        { { "hihat", "opened 5" },       { "Opened 5", 46, "hihat" } },
        { { "ride", "" },                { "Bow", 51, "" } },
        { { "ride", "bell" },            { "Bell", 53, "" } },
        { { "ride", "crash" },           { "Edge", 59, "" } },
        { { "flat ride", "" },           { "Bow", 51, "" } },
        { { "flat ride", "crash" },      { "Edge", 59, "" } },
    };

    for (const auto& [key, def] : table)
    {
        if (key.inst == instrument && key.part == part)
            return def;
    }

    ArticulationDef fallback;
    fallback.name = part.isNotEmpty()
                        ? part.substring (0, 1).toUpperCase() + part.substring (1)
                        : "Hit";
    fallback.midiNote = 36;
    fallback.chokeGroupId = type == DrumPieceType::hiHat ? "hihat" : juce::String();
    return fallback;
}

std::optional<KontaktParsedSample> KontaktSampleNameParser::parseFile (const juce::File& file) const
{
    if (! file.hasFileExtension ("wav"))
        return std::nullopt;

    const auto baseName = file.getFileNameWithoutExtension();

    if (shouldSkip (baseName))
    {
        KontaktParsedSample skipped;
        skipped.file = file;
        skipped.skipped = true;
        return skipped;
    }

    const auto parsed = parseBaseName (baseName);

    if (! parsed.has_value())
        return std::nullopt;

    const auto [instrument, part, layer] = *parsed;

    if (options.preferSnaresOnOnly && isSnaresOffPart (part))
    {
        KontaktParsedSample skipped;
        skipped.file = file;
        skipped.skipped = true;
        return skipped;
    }

    const auto instDef = matchInstrument (instrument);

    if (! instDef.has_value())
        return std::nullopt;

    const auto artDef = matchArticulation (instrument, part, instDef->type, options);

    KontaktParsedSample sample;
    sample.file = file;
    sample.pieceKey = instDef->pieceKey;
    sample.pieceName = instDef->pieceName;
    sample.pieceType = instDef->type;
    sample.articulationName = artDef.name;
    sample.midiNote = artDef.midiNote;
    sample.chokeGroupId = artDef.chokeGroupId;
    sample.layerNumber = layer;
    sample.confidence = 0.9f;

    if (options.rideCrashAsCrashPiece && part == "crash")
    {
        if (instrument == "ride")
        {
            sample.pieceKey = "crash_1";
            sample.pieceName = "Crash";
            sample.pieceType = DrumPieceType::crash;
            sample.articulationName = "Edge";
            sample.midiNote = 49;
        }
        else if (instrument == "flat ride")
        {
            sample.pieceKey = "crash_2";
            sample.pieceName = "Crash 2";
            sample.pieceType = DrumPieceType::crash;
            sample.articulationName = "Edge";
            sample.midiNote = 57;
        }
    }

    return sample;
}

void KontaktSampleNameParser::assignVelocityLayers (std::vector<KontaktParsedSample>& group)
{
    if (group.empty())
        return;

    juce::Array<int> layerNums;

    for (const auto& s : group)
        if (! layerNums.contains (s.layerNumber))
            layerNums.add (s.layerNumber);

    layerNums.sort();

    juce::Array<int> highLayers, lowLayers;

    for (const auto n : layerNums)
    {
        if (n >= 10)
            highLayers.add (n);
        else
            lowLayers.add (n);
    }

    for (auto& sample : group)
    {
        const int layer = sample.layerNumber;

        if (layer >= 10)
        {
            sample.minVelocity = juce::jmax (1, layer - 2);
            sample.maxVelocity = juce::jmin (127, layer + 2);
            sample.roundRobinIndex = 0;
        }
        else if (highLayers.size() > 0)
        {
            sample.minVelocity = 1;
            sample.maxVelocity = 90;
            sample.roundRobinIndex = layer;
        }
        else
        {
            const int count = lowLayers.size();
            const int idx = lowLayers.indexOf (layer);

            if (idx < 0)
                continue;

            const int span = 127 / juce::jmax (1, count);
            sample.minVelocity = idx * span + 1;
            sample.maxVelocity = (idx == count - 1) ? 127 : (idx + 1) * span;
            sample.roundRobinIndex = 0;
        }
    }
}
