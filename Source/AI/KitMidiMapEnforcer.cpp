#include "KitMidiMapEnforcer.h"
#include "../Models/DrumPieceTypes.h"

namespace
{
    enum class EdrumMapProfile
    {
        none,
        rolandVDrums,
        alesis,
        yamahaDtx,
        generalMidi
    };

    struct MapNoteTable
    {
        const char* profileName;
        int rackToms[4];
        int floorToms[3];
        int splash;
        int china;
        int crash2ByIndex;
    };

    constexpr MapNoteTable kRolandMap {
        "Roland V-Drums",
        { 48, 45, 43, 47 },
        { 41, 50, 48 },
        55,
        52,
        57
    };

    // Alesis Nitro / Surge / Nitro Pro factory pad table
    constexpr MapNoteTable kAlesisMap {
        "Alesis",
        { 48, 45, 43, 41 },
        { 41, 47, 48 },
        21,
        52,
        57
    };

    // Modern DTX502/600/700/800 GM-compatible defaults
    constexpr MapNoteTable kYamahaMap {
        "Yamaha DTX",
        { 48, 45, 43, 47 },
        { 41, 50, 48 },
        55,
        52,
        57
    };

    constexpr MapNoteTable kGeneralMidiMap {
        "General MIDI",
        { 48, 45, 43, 47 },
        { 41, 50, 48 },
        55,
        52,
        57
    };

    bool isShellType (DrumPieceType type)
    {
        return type == DrumPieceType::kick
            || type == DrumPieceType::snare
            || type == DrumPieceType::rackTom
            || type == DrumPieceType::floorTom;
    }

    void setArticulations (KitRecipePiece& piece,
                           std::initializer_list<std::pair<const char*, int>> defs)
    {
        piece.articulations.clear();

        for (const auto& [name, note] : defs)
            piece.articulations.push_back ({ name, note, {} });

        if (! piece.articulations.empty())
        {
            piece.articulation = piece.articulations.front().name;
            piece.preferredMidiNote = piece.articulations.front().midiNote;
        }
    }

    EdrumMapProfile profileFromText (const juce::String& hay)
    {
        if (hay.contains ("alesis") || hay.contains ("nitro") || hay.contains ("surge")
            || hay.contains ("crimson") || hay.contains ("strike") || hay.contains ("samplepad"))
            return EdrumMapProfile::alesis;

        if (hay.contains ("yamaha") || hay.contains ("dtx") || hay.contains ("dt-")
            || hay.contains ("dt ") || hay.contains ("ead"))
            return EdrumMapProfile::yamahaDtx;

        if (hay.contains ("roland") || hay.contains ("v-drum") || hay.contains ("v drum")
            || hay.contains ("vdrum") || hay.contains ("td-") || hay.contains ("td "))
            return EdrumMapProfile::rolandVDrums;

        if (hay.contains ("general midi") || hay.contains ("gm map") || hay.contains ("gm drum")
            || hay.contains (" gm ") || hay.endsWith (" gm"))
            return EdrumMapProfile::generalMidi;

        return EdrumMapProfile::none;
    }

    EdrumMapProfile resolveProfile (const KitRecipe& recipe, const juce::String& prompt)
    {
        const auto hay = (recipe.mapProfile + " " + prompt).toLowerCase();

        if (auto profile = profileFromText (hay); profile != EdrumMapProfile::none)
            return profile;

        return EdrumMapProfile::none;
    }

    const MapNoteTable& tableForProfile (EdrumMapProfile profile)
    {
        switch (profile)
        {
            case EdrumMapProfile::alesis:        return kAlesisMap;
            case EdrumMapProfile::yamahaDtx:     return kYamahaMap;
            case EdrumMapProfile::generalMidi:   return kGeneralMidiMap;
            case EdrumMapProfile::rolandVDrums:
            default:                             return kRolandMap;
        }
    }

    int rackTomNote (const MapNoteTable& table, int index)
    {
        return table.rackToms[juce::jlimit (0, 3, index)];
    }

    int floorTomNote (const MapNoteTable& table, int index)
    {
        return table.floorToms[juce::jlimit (0, 2, index)];
    }

    int crashNote (const MapNoteTable& table, int index, const juce::String& name)
    {
        if (name.containsIgnoreCase ("2"))
            return table.crash2ByIndex;

        if (name.containsIgnoreCase ("1"))
            return 49;

        return index == 0 ? 49 : table.crash2ByIndex;
    }

    void applyMapping (KitRecipePiece& piece,
                       const MapNoteTable& table,
                       int rackIndex,
                       int floorIndex,
                       int crashIndex)
    {
        switch (piece.type)
        {
            case DrumPieceType::kick:
                setArticulations (piece, { { "Center", 36 } });
                break;

            case DrumPieceType::snare:
                setArticulations (piece, { { "Center", 38 }, { "Rimshot", 40 } });
                break;

            case DrumPieceType::rackTom:
                setArticulations (piece, { { "Hit", rackTomNote (table, rackIndex) } });
                break;

            case DrumPieceType::floorTom:
                setArticulations (piece, { { "Hit", floorTomNote (table, floorIndex) } });
                break;

            case DrumPieceType::hiHat:
                setArticulations (piece, { { "Closed", 42 }, { "Open", 46 }, { "Foot", 44 } });
                break;

            case DrumPieceType::crash:
                setArticulations (piece, { { "Hit", crashNote (table, crashIndex, piece.name) } });
                break;

            case DrumPieceType::ride:
                setArticulations (piece, { { "Edge", 51 }, { "Bell", 53 } });
                break;

            case DrumPieceType::splash:
                setArticulations (piece, { { "Hit", table.splash } });
                break;

            case DrumPieceType::china:
                setArticulations (piece, { { "Hit", table.china } });
                break;

            default:
                break;
        }
    }
}

void applyMidiMapToRecipe (KitRecipe& recipe, const juce::String& prompt)
{
    const auto profile = resolveProfile (recipe, prompt);

    if (profile == EdrumMapProfile::none)
        return;

    const auto& table = tableForProfile (profile);
    recipe.mapProfile = table.profileName;

    int rackIndex = 0;
    int floorIndex = 0;
    int crashIndex = 0;

    for (auto& piece : recipe.pieces)
    {
        if (isShellType (piece.type))
        {
            if (piece.type == DrumPieceType::rackTom)
            {
                applyMapping (piece, table, rackIndex, floorIndex, crashIndex);
                ++rackIndex;
            }
            else if (piece.type == DrumPieceType::floorTom)
            {
                applyMapping (piece, table, rackIndex, floorIndex, crashIndex);
                ++floorIndex;
            }
            else
            {
                applyMapping (piece, table, rackIndex, floorIndex, crashIndex);
            }

            continue;
        }

        if (! isCymbalPieceType (piece.type))
            continue;

        if (piece.type == DrumPieceType::crash)
        {
            applyMapping (piece, table, rackIndex, floorIndex, crashIndex);
            ++crashIndex;
        }
        else
        {
            applyMapping (piece, table, rackIndex, floorIndex, crashIndex);
        }
    }
}
