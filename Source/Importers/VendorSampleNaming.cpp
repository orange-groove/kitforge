#include "VendorSampleNaming.h"
#include "../Models/DrumPieceTypes.h"
#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace
{
    struct PieceDef
    {
        juce::String groupKey;
        DrumPieceType type = DrumPieceType::accessory;
    };

    struct ArtDef
    {
        juce::String articulation;
        int midiNote = 0;
        juce::String chokeGroupId;
    };

    juce::String normalizeKey (juce::String s)
    {
        return s.trim().toLowerCase();
    }

    juce::String artLookupKey (const juce::String& instrument, const juce::String& part)
    {
        return normalizeKey (instrument) + "\n" + normalizeKey (part);
    }

    const PieceDef* lookupPiece (const juce::String& instrumentKey)
    {
        static const std::pair<const char*, PieceDef> kPieces[] = {
            { "kick",      { "kick",      DrumPieceType::kick     } },
            { "bop kick",  { "bop_kick",  DrumPieceType::kick     } },
            { "snare",     { "snare",     DrumPieceType::snare    } },
            { "rimshot",   { "snare",     DrumPieceType::snare    } },
            { "stickshot", { "snare",     DrumPieceType::snare    } },
            { "xstick",    { "snare",     DrumPieceType::snare    } },
            { "rack tom",  { "rack_tom",  DrumPieceType::rackTom  } },
            { "floor tom", { "floor_tom", DrumPieceType::floorTom } },
            { "hihat",     { "hihat",     DrumPieceType::hiHat    } },
            { "ride",      { "ride",      DrumPieceType::ride     } },
            { "flat ride", { "flat_ride", DrumPieceType::ride     } },
            { "crash",     { "crash",     DrumPieceType::crash    } },
            { "china",     { "china",     DrumPieceType::china    } },
            { "splash",    { "splash",    DrumPieceType::splash   } },
        };

        const auto key = normalizeKey (instrumentKey);

        for (const auto& entry : kPieces)
            if (key == entry.first)
                return &entry.second;

        return nullptr;
    }

    const ArtDef* lookupArticulation (const juce::String& instrumentKey, const juce::String& partKey)
    {
        static const std::pair<const char*, ArtDef> kArts[] = {
            { "kick\nsnares off",      { "Center (Snares Off)", 36, ""      } },
            { "kick\nsnares on",       { "Center (Snares On)",  36, ""      } },
            { "bop kick\nsnares off",  { "Bop (Snares Off)",    35, ""      } },
            { "bop kick\nsnares on",   { "Bop (Snares On)",     35, ""      } },
            { "snare\nsnares off",     { "Center (Snares Off)", 38, ""      } },
            { "snare\nsnares on",      { "Center (Snares On)",  38, ""      } },
            { "rimshot\nsnares off",   { "Rimshot (Snares Off)", 40, ""     } },
            { "rimshot\nsnares on",    { "Rimshot (Snares On)",  40, ""     } },
            { "stickshot\nsnares off", { "Stickshot (Snares Off)", 38, ""   } },
            { "stickshot\nsnares on",  { "Stickshot (Snares On)",  38, ""   } },
            { "xstick\nsnares off",   { "Cross Stick (Snares Off)", 37, ""  } },
            { "xstick\nsnares on",    { "Cross Stick (Snares On)",  37, ""  } },
            { "rack tom\nsnares off",  { "Hit (Snares Off)",    48, ""      } },
            { "rack tom\nsnares on",   { "Hit (Snares On)",     48, ""      } },
            { "floor tom\nsnares off", { "Hit (Snares Off)",    43, ""      } },
            { "floor tom\nsnares on",  { "Hit (Snares On)",     43, ""      } },
            { "hihat\nclose",          { "Closed Tight",        42, "hihat" } },
            { "hihat\nclosed",         { "Closed",              42, "hihat" } },
            { "hihat\nclosed side",    { "Closed Side",         42, "hihat" } },
            { "hihat\nopen",           { "Open",                46, "hihat" } },
            { "hihat\nopened 1",       { "Opened 1",            46, "hihat" } },
            { "hihat\nopened 2",       { "Opened 2",            46, "hihat" } },
            { "hihat\nopened 3",       { "Opened 3",            46, "hihat" } },
            { "hihat\nopened 4",       { "Opened 4",            46, "hihat" } },
            { "hihat\nopened 5",       { "Opened 5",            46, "hihat" } },
            { "ride\n",                { "Bow",                 51, ""      } },
            { "ride\nbell",            { "Bell",                53, ""      } },
            { "ride\ncrash",           { "Crash",               59, ""      } },
            { "flat ride\n",           { "Bow",                 51, ""      } },
            { "flat ride\ncrash",      { "Crash",               59, ""      } },
        };

        const auto key = artLookupKey (instrumentKey, partKey);

        for (const auto& entry : kArts)
            if (key == entry.first)
                return &entry.second;

        return nullptr;
    }

    bool shouldSkipName (const juce::String& baseName)
    {
        const auto lower = baseName.toLowerCase();

        return lower.startsWith ("turn snare on")
            || lower.startsWith ("turn off snares");
    }

    bool parseDelimitedName (const juce::String& baseName,
                             juce::String& instrumentOut,
                             juce::String& partOut,
                             int& layerOut)
    {
        const auto trimmed = baseName.trim();

        if (trimmed.isEmpty())
            return false;

        const int lastSep = trimmed.lastIndexOf (" - ");

        if (lastSep < 0)
            return false;

        const auto layerText = trimmed.substring (lastSep + 3).trim();

        if (! layerText.containsOnly ("0123456789"))
            return false;

        layerOut = layerText.getIntValue();

        if (layerOut <= 0)
            return false;

        const auto head = trimmed.substring (0, lastSep).trim();
        const int partSep = head.lastIndexOf (" - ");

        if (partSep >= 0)
        {
            instrumentOut = head.substring (0, partSep).trim();
            partOut = head.substring (partSep + 3).trim();
        }
        else
        {
            instrumentOut = head;
            partOut = {};
        }

        return instrumentOut.isNotEmpty();
    }

    juce::String layerGroupKey (const SampleMetadata& meta)
    {
        const auto piece = meta.pieceGroupKey.isNotEmpty()
                               ? meta.pieceGroupKey
                               : drumPieceTypeToString (meta.instrumentType);
        return piece + "|" + meta.articulation.toLowerCase();
    }

    struct LayerRange
    {
        int minVelocity = 1;
        int maxVelocity = 127;
        bool roundRobin = false;
    };

    LayerRange velocityForLayer (int layer, const std::vector<int>& sortedLayers)
    {
        LayerRange result;
        std::vector<int> high;
        std::vector<int> low;

        for (int n : sortedLayers)
            (n >= 10 ? high : low).push_back (n);

        if (layer >= 10)
        {
            result.minVelocity = juce::jmax (1, layer - 2);
            result.maxVelocity = juce::jmin (127, layer + 2);
            result.roundRobin = false;
            return result;
        }

        if (! high.empty())
        {
            result.minVelocity = 1;
            result.maxVelocity = 90;
            result.roundRobin = true;
            return result;
        }

        const int count = (int) low.size();

        if (count <= 0)
            return result;

        const auto it = std::find (low.begin(), low.end(), layer);

        if (it == low.end())
            return result;

        const int idx = (int) std::distance (low.begin(), it);
        const int span = juce::jmax (1, 127 / count);
        result.minVelocity = idx * span + 1;
        result.maxVelocity = (idx == count - 1) ? 127 : (idx + 1) * span;
        result.roundRobin = false;
        return result;
    }

    juce::String dominantLayerScheme (const std::vector<size_t>& indices,
                                      const std::vector<SampleMetadata>& samples)
    {
        juce::String scheme;

        for (auto idx : indices)
        {
            const auto& s = samples[idx].layerScheme;

            if (s.isEmpty() || s == "auto")
                continue;

            if (scheme.isEmpty())
                scheme = s;
            else if (scheme != s)
                return "auto";
        }

        return scheme.isEmpty() ? "auto" : scheme;
    }
}

bool VendorSampleNaming::tryApply (const juce::File& file, SampleMetadata& meta)
{
    if (! file.hasFileExtension ("wav"))
        return false;

    const auto baseName = file.getFileNameWithoutExtension();

    if (shouldSkipName (baseName))
        return false;

    juce::String instrument;
    juce::String part;
    int layer = 0;

    if (! parseDelimitedName (baseName, instrument, part, layer))
        return false;

    meta.id = juce::Uuid().toString();
    meta.filePath = file.getFullPathName();
    meta.layerIndex = layer;

    const auto* piece = lookupPiece (instrument);

    if (piece != nullptr)
    {
        meta.instrumentType = piece->type;
        meta.pieceGroupKey = piece->groupKey;

        if (piece->type == DrumPieceType::hiHat)
            meta.chokeGroupId = "hihat";
    }
    else
    {
        meta.instrumentType = DrumPieceType::accessory;
        meta.pieceGroupKey = normalizeKey (instrument).replaceCharacter (' ', '_');
    }

    if (const auto* art = lookupArticulation (instrument, part))
    {
        meta.articulation = art->articulation;
        meta.midiNote = art->midiNote;

        if (art->chokeGroupId.isNotEmpty())
            meta.chokeGroupId = art->chokeGroupId;
    }
    else if (part.isNotEmpty())
    {
        meta.articulation = part.substring (0, 1).toUpperCase() + part.substring (1);
    }

    meta.confidence = piece != nullptr ? 1.0f : 0.75f;
    return true;
}

void VendorSampleNaming::resolveLayerAssignments (std::vector<SampleMetadata>& samples)
{
    std::map<juce::String, std::vector<size_t>> groups;

    for (size_t i = 0; i < samples.size(); ++i)
    {
        if (samples[i].layerIndex <= 0)
            continue;

        groups[layerGroupKey (samples[i])].push_back (i);
    }

    for (const auto& kv : groups)
    {
        const auto& indices = kv.second;
        std::set<int> layerSet;

        for (auto idx : indices)
            layerSet.insert (samples[idx].layerIndex);

        const std::vector<int> sortedLayers (layerSet.begin(), layerSet.end());
        const auto scheme = dominantLayerScheme (indices, samples);

        std::map<std::pair<int, int>, std::vector<std::pair<int, size_t>>> buckets;

        for (auto idx : indices)
        {
            auto& meta = samples[idx];
            LayerRange range = velocityForLayer (meta.layerIndex, sortedLayers);

            if (scheme == "roundRobin")
            {
                range.minVelocity = 1;
                range.maxVelocity = 127;
                range.roundRobin = true;
            }
            else if (scheme == "velocity")
            {
                range.roundRobin = false;
            }

            meta.minVelocity = range.minVelocity;
            meta.maxVelocity = range.maxVelocity;
            buckets[{ range.minVelocity, range.maxVelocity }].push_back ({ meta.layerIndex, idx });
        }

        for (auto& bucket : buckets)
        {
            auto& entries = bucket.second;
            std::sort (entries.begin(), entries.end(),
                       [] (const auto& a, const auto& b) { return a.first < b.first; });

            for (size_t rr = 0; rr < entries.size(); ++rr)
            {
                auto& meta = samples[entries[rr].second];
                meta.roundRobinIndex = entries.size() > 1 ? (int) rr + 1 : 0;
            }
        }
    }
}
