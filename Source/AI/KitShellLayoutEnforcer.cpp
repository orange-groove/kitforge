#include "KitShellLayoutEnforcer.h"
#include <algorithm>
#include <vector>

namespace
{
    // Floor tom diagonal: 45° on 900×520 reference (equal pixel dx and dy per step).
    constexpr float kFloorAnchorX = 0.60f;
    constexpr float kFloorAnchorY = 0.75f;
    constexpr float kFloorStepPx  = 72.0f;
    constexpr float kRefCanvasW   = 900.0f;
    constexpr float kRefCanvasH   = 520.0f;
    constexpr float kFloorStepX   = kFloorStepPx / kRefCanvasW;
    constexpr float kFloorStepY   = kFloorStepPx / kRefCanvasH;

    constexpr float kRackRadiusPx   = 36.0f;
    constexpr float kCrashRadiusPx  = 52.0f;
    constexpr float kHiHatRadiusPx  = 34.0f;
    constexpr float kSnareRadiusPx  = 42.0f;
    constexpr float kBarelyOverlapPx = 8.0f;

    float pxToNormX (float px) { return px / kRefCanvasW; }
    float pxToNormY (float px) { return px / kRefCanvasH; }

    constexpr float kFloorTomRadiusPx = 48.0f;
    constexpr float kRideRadiusPx     = 55.0f;
    constexpr float kRideFloorOverlapPx = 12.0f;

    struct ShellAnchors
    {
        float snareX = 0.40f;
        float snareY = 0.76f;
        float rackY = 0.60f;
        float leftRackX = 0.50f;
        float rightRackX = 0.50f;
        float floorX = kFloorAnchorX;
        float floorY = kFloorAnchorY;
        float floor2X = kFloorAnchorX + kFloorStepX;
        float floor2Y = kFloorAnchorY + kFloorStepY;
    };

    ShellAnchors collectShellAnchors (const std::vector<KitRecipePiece>& pieces)
    {
        ShellAnchors anchors;
        std::vector<float> rackXs;

        for (const auto& piece : pieces)
        {
            if (piece.layoutXNorm < 0.0f || piece.layoutYNorm < 0.0f)
                continue;

            switch (piece.type)
            {
                case DrumPieceType::snare:
                    anchors.snareX = piece.layoutXNorm;
                    anchors.snareY = piece.layoutYNorm;
                    break;

                case DrumPieceType::rackTom:
                    rackXs.push_back (piece.layoutXNorm);
                    anchors.rackY = piece.layoutYNorm;
                    break;

                case DrumPieceType::floorTom:
                    if (piece.layoutXNorm <= anchors.floorX)
                    {
                        anchors.floorX = piece.layoutXNorm;
                        anchors.floorY = piece.layoutYNorm;
                    }
                    else
                    {
                        anchors.floor2X = piece.layoutXNorm;
                        anchors.floor2Y = piece.layoutYNorm;
                    }
                    break;

                default:
                    break;
            }
        }

        if (! rackXs.empty())
        {
            std::sort (rackXs.begin(), rackXs.end());
            anchors.leftRackX = rackXs.front();
            anchors.rightRackX = rackXs.back();
        }

        return anchors;
    }

    float crashYForRack (float rackY)
    {
        const float sepPx = kRackRadiusPx + kCrashRadiusPx - kBarelyOverlapPx;
        return rackY - pxToNormY (sepPx);
    }

    std::pair<float, float> hiHatLayout (const ShellAnchors& anchors)
    {
        const float sepPx = kSnareRadiusPx + kHiHatRadiusPx - 6.0f;
        return { anchors.snareX - pxToNormX (sepPx), anchors.snareY };
    }

    std::pair<float, float> rideLayout (const ShellAnchors& anchors)
    {
        // Right of the floor tom cluster, overlapping floor tom 1 — not stacked on crash 2.
        const float clusterX = (anchors.floorX + anchors.floor2X) * 0.5f;
        const float clusterY = (anchors.floorY + anchors.floor2Y) * 0.5f;
        const float overlapPx = kFloorTomRadiusPx + kRideRadiusPx - kRideFloorOverlapPx;

        return { clusterX + pxToNormX (overlapPx * 0.55f),
                 clusterY - pxToNormY (overlapPx * 0.20f) };
    }

    std::pair<float, float> defaultCymbalArcPosition (int slotIndex, const ShellAnchors& anchors)
    {
        const float crashY = crashYForRack (anchors.rackY);
        const auto [hiHatX, hiHatY] = hiHatLayout (anchors);

        float crash1X = anchors.leftRackX;
        float crash2X = anchors.rightRackX;

        if (std::abs (crash1X - crash2X) < 0.001f)
        {
            crash1X -= pxToNormX (36.0f);
            crash2X += pxToNormX (36.0f);
        }

        const auto [rideX, rideY] = rideLayout (anchors);

        switch (slotIndex)
        {
            case 0:
                return { hiHatX, hiHatY };

            case 1:
            {
                const float splashX = hiHatX + (crash1X - hiHatX) * 0.45f;
                const float splashY = hiHatY + (crashY - hiHatY) * 0.42f;
                return { splashX, splashY };
            }

            case 2:
                return { crash1X, crashY };

            case 3:
                return { crash2X, crashY };

            case 4:
                return { rideX, rideY };

            default:
            {
                const float dx = pxToNormX (48.0f) * (float) (slotIndex - 4);
                return { rideX + dx, rideY - pxToNormY (12.0f) };
            }
        }
    }

    KitRecipePiece makeDefaultCymbal (DrumPieceType type,
                                      const char* name,
                                      std::initializer_list<std::pair<const char*, int>> articulationDefs,
                                      const juce::StringArray& tags)
    {
        KitRecipePiece piece;
        piece.id = juce::Uuid().toString();
        piece.type = type;
        piece.name = name;
        piece.tags = tags;

        for (const auto& [artName, midiNote] : articulationDefs)
            piece.articulations.push_back ({ artName, midiNote, {} });

        if (! piece.articulations.empty())
        {
            piece.articulation = piece.articulations.front().name;
            piece.preferredMidiNote = piece.articulations.front().midiNote;
        }

        return piece;
    }

    bool recipeHasCymbalType (const KitRecipe& recipe, DrumPieceType type)
    {
        for (const auto& piece : recipe.pieces)
            if (piece.type == type)
                return true;

        return false;
    }

    int countCymbalsOfType (const KitRecipe& recipe, DrumPieceType type)
    {
        int count = 0;

        for (const auto& piece : recipe.pieces)
            if (piece.type == type)
                ++count;

        return count;
    }

    bool isShell (DrumPieceType type)
    {
        return type == DrumPieceType::kick
            || type == DrumPieceType::snare
            || type == DrumPieceType::rackTom
            || type == DrumPieceType::floorTom;
    }

    struct ShellSlot
    {
        DrumPieceType type;
        const char* name;
        float x;
        float y;
        int midiNote;
    };

    KitRecipePiece makeShellFromSlot (const ShellSlot& slot, const juce::StringArray& tags)
    {
        KitRecipePiece piece;
        piece.id = juce::Uuid().toString();
        piece.type = slot.type;
        piece.name = slot.name;
        piece.layoutXNorm = slot.x;
        piece.layoutYNorm = slot.y;
        piece.tags = tags;

        if (slot.type == DrumPieceType::kick)
            piece.articulations = { { "Center", 36, {} } };
        else if (slot.type == DrumPieceType::snare)
            piece.articulations = { { "Center", 38, {} }, { "Rimshot", 40, {} } };
        else
            piece.articulations = { { "Hit", slot.midiNote, {} } };

        piece.articulation = piece.articulations.front().name;
        piece.preferredMidiNote = piece.articulations.front().midiNote;
        return piece;
    }

    KitRecipePiece copyShellWithLayout (KitRecipePiece piece, const ShellSlot& slot)
    {
        piece.layoutXNorm = slot.x;
        piece.layoutYNorm = slot.y;
        piece.name = slot.name;

        if (slot.type == DrumPieceType::kick)
            piece.articulations = { { "Center", 36, {} } };
        else if (slot.type == DrumPieceType::snare)
            piece.articulations = { { "Center", 38, {} }, { "Rimshot", 40, {} } };
        else
            piece.articulations = { { "Hit", slot.midiNote, {} } };

        piece.articulation = piece.articulations.front().name;
        piece.preferredMidiNote = piece.articulations.front().midiNote;
        return piece;
    }

    const ShellSlot* slotsForCount (int count, int& outCount)
    {
        static const ShellSlot fourPiece[] =
        {
            { DrumPieceType::kick,     "Kick",       0.50f, 0.65f, 36 },
            { DrumPieceType::snare,    "Snare",      0.40f, 0.76f, 38 },
            { DrumPieceType::rackTom,  "Rack Tom",   0.50f, 0.60f, 48 },
            { DrumPieceType::floorTom, "Floor Tom",  kFloorAnchorX, kFloorAnchorY, 41 },
        };

        static const ShellSlot fivePiece[] =
        {
            { DrumPieceType::kick,     "Kick",        0.50f, 0.65f, 36 },
            { DrumPieceType::snare,    "Snare",       0.40f, 0.76f, 38 },
            { DrumPieceType::rackTom,  "Rack Tom 1",  0.46f, 0.60f, 48 },
            { DrumPieceType::rackTom,  "Rack Tom 2",  0.54f, 0.60f, 45 },
            { DrumPieceType::floorTom, "Floor Tom",   kFloorAnchorX, kFloorAnchorY, 41 },
        };

        static const ShellSlot sixPiece[] =
        {
            { DrumPieceType::kick,     "Kick",         0.50f, 0.65f, 36 },
            { DrumPieceType::snare,    "Snare",        0.40f, 0.76f, 38 },
            { DrumPieceType::rackTom,  "Rack Tom 1",   0.46f, 0.60f, 48 },
            { DrumPieceType::rackTom,  "Rack Tom 2",   0.54f, 0.60f, 45 },
            { DrumPieceType::floorTom, "Floor Tom 1",  kFloorAnchorX, kFloorAnchorY, 41 },
            { DrumPieceType::floorTom, "Floor Tom 2",  kFloorAnchorX + kFloorStepX, kFloorAnchorY + kFloorStepY, 50 },
        };

        static const ShellSlot sevenPiece[] =
        {
            { DrumPieceType::kick,     "Kick",         0.50f, 0.65f, 36 },
            { DrumPieceType::snare,    "Snare",        0.40f, 0.76f, 38 },
            { DrumPieceType::rackTom,  "Rack Tom 1",   0.42f, 0.60f, 48 },
            { DrumPieceType::rackTom,  "Rack Tom 2",   0.50f, 0.60f, 45 },
            { DrumPieceType::rackTom,  "Rack Tom 3",   0.58f, 0.60f, 43 },
            { DrumPieceType::floorTom, "Floor Tom 1",  kFloorAnchorX, kFloorAnchorY, 41 },
            { DrumPieceType::floorTom, "Floor Tom 2",  kFloorAnchorX + kFloorStepX, kFloorAnchorY + kFloorStepY, 50 },
        };

        static const ShellSlot eightPiece[] =
        {
            { DrumPieceType::kick,     "Kick",         0.50f, 0.65f, 36 },
            { DrumPieceType::snare,    "Snare",        0.40f, 0.76f, 38 },
            { DrumPieceType::rackTom,  "Rack Tom 1",   0.42f, 0.60f, 48 },
            { DrumPieceType::rackTom,  "Rack Tom 2",   0.50f, 0.60f, 45 },
            { DrumPieceType::rackTom,  "Rack Tom 3",   0.58f, 0.60f, 43 },
            { DrumPieceType::floorTom, "Floor Tom 1",  kFloorAnchorX, kFloorAnchorY, 41 },
            { DrumPieceType::floorTom, "Floor Tom 2",  kFloorAnchorX + kFloorStepX, kFloorAnchorY + kFloorStepY, 50 },
            { DrumPieceType::floorTom, "Floor Tom 3",  0.718f, 0.958f, 48 },
        };

        switch (count)
        {
            case 4: outCount = 4; return fourPiece;
            case 5: outCount = 5; return fivePiece;
            case 6: outCount = 6; return sixPiece;
            case 7: outCount = 7; return sevenPiece;
            case 8: outCount = 8; return eightPiece;
            default: outCount = 0; return nullptr;
        }
    }

    std::vector<KitRecipePiece> collectByTypeInOrder (const std::vector<KitRecipePiece>& pieces,
                                                        DrumPieceType type)
    {
        std::vector<KitRecipePiece> result;

        for (const auto& piece : pieces)
            if (piece.type == type)
                result.push_back (piece);

        return result;
    }
}

int parseRequestedShellCount (const juce::String& prompt)
{
    const auto p = prompt.toLowerCase();

    if (p.contains ("8 piece") || p.contains ("8-piece") || p.contains ("8 pc") || p.contains ("8pc")
        || p.contains ("eight piece"))
        return 8;

    if (p.contains ("7 piece") || p.contains ("7-piece") || p.contains ("7 pc") || p.contains ("7pc")
        || p.contains ("seven piece"))
        return 7;

    if (p.contains ("6 piece") || p.contains ("6-piece") || p.contains ("6 pc") || p.contains ("6pc")
        || p.contains ("six piece"))
        return 6;

    if (p.contains ("5 piece") || p.contains ("5-piece") || p.contains ("5 pc") || p.contains ("5pc")
        || p.contains ("five piece"))
        return 5;

    if (p.contains ("4 piece") || p.contains ("4-piece") || p.contains ("4 pc") || p.contains ("4pc")
        || p.contains ("four piece"))
        return 4;

    return 0;
}

bool isRefineKitPrompt (const juce::String& prompt)
{
    const auto p = prompt.toLowerCase().trim();

    if (p.isEmpty())
        return false;

    if (parseRequestedShellCount (prompt) > 0)
    {
        if (p.contains ("create ") || p.contains ("build ") || p.contains ("generate ")
            || p.contains ("design ") || p.startsWith ("make a ") || p.startsWith ("make me a "))
            return false;
    }

    static constexpr const char* refineHints[] =
    {
        "add ", "remove ", "delete ", "drop ", "change ", "swap ", "rename ",
        "move ", "replace ", "update ", "convert ", "make it a ", "make this a ",
        "keep ", "without ", "lose the ", "get rid of "
    };

    for (auto* hint : refineHints)
    {
        if (p.contains (hint))
            return true;
    }

    return false;
}

int countShellsInRecipe (const KitRecipe& recipe)
{
    int count = 0;

    for (const auto& piece : recipe.pieces)
        if (isShell (piece.type))
            ++count;

    return count;
}

void enforceShellCount (KitRecipe& recipe, int targetShells)
{
    int slotCount = 0;
    const ShellSlot* slots = slotsForCount (targetShells, slotCount);

    if (slots == nullptr || slotCount == 0)
        return;

    std::vector<KitRecipePiece> cymbals;
    std::vector<KitRecipePiece> shells;

    for (const auto& piece : recipe.pieces)
    {
        if (isCymbalPieceType (piece.type))
            cymbals.push_back (piece);
        else if (isShell (piece.type))
            shells.push_back (piece);
    }

    const auto kicks = collectByTypeInOrder (shells, DrumPieceType::kick);
    const auto snares = collectByTypeInOrder (shells, DrumPieceType::snare);
    const auto racks = collectByTypeInOrder (shells, DrumPieceType::rackTom);
    const auto floors = collectByTypeInOrder (shells, DrumPieceType::floorTom);

    juce::StringArray tags = recipe.globalTags;
    std::vector<KitRecipePiece> builtShells;
    builtShells.reserve ((size_t) slotCount);

    int rackIndex = 0;
    int floorIndex = 0;

    for (int i = 0; i < slotCount; ++i)
    {
        const auto& slot = slots[i];
        KitRecipePiece piece;

        if (slot.type == DrumPieceType::kick && ! kicks.empty())
            piece = copyShellWithLayout (kicks.front(), slot);
        else if (slot.type == DrumPieceType::snare && ! snares.empty())
            piece = copyShellWithLayout (snares.front(), slot);
        else if (slot.type == DrumPieceType::rackTom && rackIndex < (int) racks.size())
            piece = copyShellWithLayout (racks[(size_t) rackIndex++], slot);
        else if (slot.type == DrumPieceType::floorTom && floorIndex < (int) floors.size())
            piece = copyShellWithLayout (floors[(size_t) floorIndex++], slot);
        else
            piece = makeShellFromSlot (slot, tags);

        builtShells.push_back (std::move (piece));
    }

    recipe.pieces.clear();

    for (auto& shell : builtShells)
        recipe.pieces.push_back (std::move (shell));

    for (auto& cymbal : cymbals)
        recipe.pieces.push_back (std::move (cymbal));
}

void applyDefaultCymbalLayout (KitRecipe& recipe)
{
    const ShellAnchors anchors = collectShellAnchors (recipe.pieces);
    int crashIndex = 0;
    int rideIndex = 0;
    int extraIndex = 0;

    for (auto& piece : recipe.pieces)
    {
        if (! isCymbalPieceType (piece.type))
            continue;

        int slot = -1;

        switch (piece.type)
        {
            case DrumPieceType::hiHat:
                slot = 0;
                break;

            case DrumPieceType::splash:
                slot = 1;
                if (piece.name.isEmpty())
                    piece.name = "Splash";
                break;

            case DrumPieceType::crash:
            {
                if (piece.name.containsIgnoreCase ("2"))
                    slot = 3;
                else if (piece.name.containsIgnoreCase ("1"))
                    slot = 2;
                else if (crashIndex == 0)
                    slot = 2;
                else if (crashIndex == 1)
                    slot = 3;
                else
                    slot = 5 + extraIndex++;

                if (slot == 2 && (piece.name.isEmpty() || piece.name.equalsIgnoreCase ("crash")))
                    piece.name = "Crash 1";
                else if (slot == 3 && (piece.name.isEmpty() || piece.name.equalsIgnoreCase ("crash")))
                    piece.name = "Crash 2";
                else if (piece.name.isEmpty() || piece.name.equalsIgnoreCase ("crash"))
                    piece.name = "Crash " + juce::String (crashIndex + 1);

                ++crashIndex;
                break;
            }

            case DrumPieceType::ride:
                if (rideIndex == 0)
                    slot = 4;
                else
                    slot = 5 + extraIndex++;

                ++rideIndex;

                if (piece.name.isEmpty())
                    piece.name = rideIndex == 1 ? "Ride" : "Ride " + juce::String (rideIndex);
                break;

            case DrumPieceType::china:
                slot = 5 + extraIndex++;
                if (piece.name.isEmpty())
                    piece.name = "China";
                break;

            default:
                slot = 5 + extraIndex++;
                break;
        }

        const auto [x, y] = defaultCymbalArcPosition (slot, anchors);
        piece.layoutXNorm = x;
        piece.layoutYNorm = y;
    }
}

bool promptMentionsCymbals (const juce::String& prompt)
{
    const auto p = prompt.toLowerCase();

    static constexpr const char* hints[] =
    {
        "cymbal", "hi-hat", "hi hat", "hihat",
        "crash", "ride", "splash", "china", "sizzle", "stack"
    };

    for (auto* hint : hints)
    {
        if (p.contains (hint))
            return true;
    }

    return false;
}

void enforceDefaultCymbals (KitRecipe& recipe, const juce::String& prompt)
{
    if (promptMentionsCymbals (prompt))
        return;

    const auto p = prompt.toLowerCase();

    if (p.contains ("no cymbal") || p.contains ("without cymbal") || p.contains ("shells only")
        || p.contains ("shells-only") || p.contains ("no hats"))
        return;

    juce::StringArray tags = recipe.globalTags;

    if (! recipeHasCymbalType (recipe, DrumPieceType::hiHat))
    {
        recipe.pieces.push_back (makeDefaultCymbal (DrumPieceType::hiHat, "Hi-Hat",
                                                    { { "Closed", 42 }, { "Open", 46 } }, tags));
    }

    if (! recipeHasCymbalType (recipe, DrumPieceType::splash))
    {
        recipe.pieces.push_back (makeDefaultCymbal (DrumPieceType::splash, "Splash",
                                                    { { "Hit", 55 } }, tags));
    }

    const int crashCount = countCymbalsOfType (recipe, DrumPieceType::crash);

    if (crashCount < 1)
    {
        recipe.pieces.push_back (makeDefaultCymbal (DrumPieceType::crash, "Crash 1",
                                                    { { "Hit", 49 } }, tags));
    }

    if (crashCount < 2)
    {
        recipe.pieces.push_back (makeDefaultCymbal (DrumPieceType::crash, "Crash 2",
                                                    { { "Hit", 57 } }, tags));
    }

    if (! recipeHasCymbalType (recipe, DrumPieceType::ride))
    {
        recipe.pieces.push_back (makeDefaultCymbal (DrumPieceType::ride, "Ride",
                                                    { { "Bow", 51 } }, tags));
    }
}
