#include "KitImportLayoutEnforcer.h"
#include "../AI/KitModelRecipeBridge.h"
#include "../Models/KitModel.h"
#include <algorithm>
#include <vector>

namespace
{
    struct Pos
    {
        float x;
        float y;
    };

    struct PieceRef
    {
        KitRecipePiece* piece = nullptr;
        int note = 0;
        int articulationCount = 0;
    };

    int primaryMidiForPiece (const KitRecipePiece& piece)
    {
        if (piece.preferredMidiNote > 0)
            return piece.preferredMidiNote;

        if (! piece.articulations.empty())
            return piece.articulations.front().midiNote;

        return 0;
    }

    float lerp (float a, float b, float t) { return a + (b - a) * t; }

    Pos clampPos (Pos p)
    {
        return { juce::jlimit (0.04f, 0.96f, p.x), juce::jlimit (0.06f, 0.97f, p.y) };
    }

    void place (PieceRef& ref, Pos pos)
    {
        const auto p = clampPos (pos);
        ref.piece->layoutXNorm = p.x;
        ref.piece->layoutYNorm = p.y;
    }
}

void applyImportKitLayout (KitModel& model, float canvasWidth, float canvasHeight)
{
    auto recipe = kitRecipeFromModel (model, canvasWidth, canvasHeight);

    std::vector<PieceRef> kicks, snares, racks, floors, hats, crashes, rides, chinas, splashes, others;

    for (auto& piece : recipe.pieces)
    {
        PieceRef ref { &piece, primaryMidiForPiece (piece), (int) piece.articulations.size() };

        switch (piece.type)
        {
            case DrumPieceType::kick:     kicks.push_back (ref);    break;
            case DrumPieceType::snare:    snares.push_back (ref);   break;
            case DrumPieceType::rackTom:  racks.push_back (ref);    break;
            case DrumPieceType::floorTom: floors.push_back (ref);   break;
            case DrumPieceType::hiHat:    hats.push_back (ref);     break;
            case DrumPieceType::crash:    crashes.push_back (ref);  break;
            case DrumPieceType::ride:     rides.push_back (ref);    break;
            case DrumPieceType::china:    chinas.push_back (ref);   break;
            case DrumPieceType::splash:   splashes.push_back (ref); break;
            default:                      others.push_back (ref);   break;
        }
    }

    const auto byNoteDesc    = [] (const PieceRef& a, const PieceRef& b) { return a.note > b.note; };
    const auto byNoteAsc     = [] (const PieceRef& a, const PieceRef& b) { return a.note < b.note; };
    const auto byArtCountDesc = [] (const PieceRef& a, const PieceRef& b)
    {
        return a.articulationCount > b.articulationCount;
    };

    // --- Kick(s): front centre ---
    for (size_t i = 0; i < kicks.size(); ++i)
    {
        const float t = kicks.size() == 1 ? 0.5f : (float) i / (float) (kicks.size() - 1);
        place (kicks[i], { lerp (0.44f, 0.56f, t), 0.67f });
    }

    // --- Snares: "main" (most articulations) front-left, extras in a front row ---
    std::sort (snares.begin(), snares.end(), byArtCountDesc);

    if (! snares.empty())
        place (snares[0], { 0.405f, 0.755f }); // tucked into the kick's bottom-left corner

    const int extraSnareCount = (int) snares.size() - 1;

    for (int i = 1; i <= extraSnareCount; ++i)
    {
        const float t = extraSnareCount == 1 ? 0.5f : (float) (i - 1) / (float) (extraSnareCount - 1);
        place (snares[(size_t) i], { lerp (0.30f, 0.70f, t), 0.92f });
    }

    // --- Rack toms: highest pitch on the left, over the kick ---
    std::sort (racks.begin(), racks.end(), byNoteDesc);

    static constexpr Pos kRackPos[] =
    {
        { 0.42f, 0.620f }, { 0.50f, 0.610f }, { 0.58f, 0.620f }, { 0.645f, 0.635f }, { 0.36f, 0.635f }
    };

    for (size_t i = 0; i < racks.size(); ++i)
        place (racks[i], kRackPos[std::min (i, (size_t) 4)]);

    // --- Floor toms: start at the kick's bottom-right corner, descending out right ---
    std::sort (floors.begin(), floors.end(), byNoteDesc);

    static constexpr Pos kFloorPos[] =
    {
        { 0.585f, 0.745f }, { 0.675f, 0.80f }, { 0.765f, 0.855f }
    };

    for (size_t i = 0; i < floors.size(); ++i)
        place (floors[i], kFloorPos[std::min (i, (size_t) 2)]);

    // --- Hi-hat: directly to the left of the snare (same height) ---
    for (size_t i = 0; i < hats.size(); ++i)
        place (hats[i], { 0.255f, 0.755f - (float) i * 0.10f });

    // --- Cymbals form one fan centred on the kick. The two main crashes sit up
    //     top, straddling the kick; everything else fans out-and-down from that
    //     pair (extra crashes to the left, rides then china to the right), so the
    //     cymbals stay pulled in close around the kit. ---
    std::sort (crashes.begin(), crashes.end(), byNoteAsc);
    std::sort (rides.begin(),   rides.end(),   byNoteAsc);

    constexpr Pos crashLeftAnchor  { 0.440f, 0.490f }; // centred over kick (x ~0.50)
    constexpr Pos crashRightAnchor { 0.560f, 0.490f };
    constexpr float fanStepX = 0.108f;
    constexpr float fanStepY = 0.078f;

    if (! crashes.empty())
        place (crashes[0], crashLeftAnchor);

    if (crashes.size() >= 2)
        place (crashes[1], crashRightAnchor);

    // Extra crashes fan down-and-left from the left anchor.
    for (size_t i = 2; i < crashes.size(); ++i)
    {
        const int k = (int) i - 1;
        place (crashes[i], { crashLeftAnchor.x - (float) k * fanStepX,
                             crashLeftAnchor.y + (float) k * fanStepY });
    }

    // Rides then china fan down-and-right from the right anchor.
    std::vector<PieceRef*> rightFan;
    for (auto& r : rides)  rightFan.push_back (&r);
    for (auto& c : chinas) rightFan.push_back (&c);

    for (size_t i = 0; i < rightFan.size(); ++i)
    {
        const int k = (int) i + 1;
        place (*rightFan[i], { crashRightAnchor.x + (float) k * fanStepX,
                               crashRightAnchor.y + (float) k * fanStepY });
    }

    // --- Splash: between the two top crashes, nudged up. ---
    for (size_t i = 0; i < splashes.size(); ++i)
        place (splashes[i], { 0.50f, 0.420f - (float) i * 0.085f });

    // --- Anything else: tuck along the front-left edge ---
    for (size_t i = 0; i < others.size(); ++i)
        place (others[i], { 0.10f + (float) i * 0.07f, 0.95f });

    for (const auto& spec : recipe.pieces)
    {
        if (spec.layoutXNorm < 0.0f || spec.layoutYNorm < 0.0f)
            continue;

        if (auto* piece = model.findPieceById (spec.id))
        {
            piece->x = spec.layoutXNorm * canvasWidth - piece->width * 0.5f;
            piece->y = spec.layoutYNorm * canvasHeight - piece->height * 0.5f;
        }
    }

    model.recordLayoutEdit();
}
