#include "KitPieceAddPlacement.h"
#include "KitModel.h"
#include "../Engine/Articulation.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <vector>

namespace
{
    constexpr float kGapPx = 2.0f;
    constexpr float kInvSqrt2 = 0.7071067811865475f;

    struct NormPos
    {
        float x;
        float y;
    };

    struct RackSlot
    {
        DrumPiece* piece = nullptr;
        float savedCx = 0.5f;
        float savedCy = 0.5f;
        float radiusNormX = 0.0f;
    };

    float pieceDiameterPx (const DrumPiece& piece)
    {
        return juce::jmax (piece.width, piece.height);
    }

    float centerXNorm (const DrumPiece& piece, float canvasW)
    {
        return (piece.x + piece.width * 0.5f) / canvasW;
    }

    float centerYNorm (const DrumPiece& piece, float canvasH)
    {
        return (piece.y + piece.height * 0.5f) / canvasH;
    }

    void setCenterNorm (DrumPiece& piece, NormPos pos, float canvasW, float canvasH)
    {
        const float cx = juce::jlimit (0.04f, 0.96f, pos.x);
        const float cy = juce::jlimit (0.06f, 0.97f, pos.y);
        piece.x = cx * canvasW - piece.width * 0.5f;
        piece.y = cy * canvasH - piece.height * 0.5f;
    }

    float median (std::vector<float> values)
    {
        if (values.empty())
            return 0.5f;

        std::sort (values.begin(), values.end());
        const size_t mid = values.size() / 2;
        return values.size() % 2 == 0
            ? (values[mid - 1] + values[mid]) * 0.5f
            : values[mid];
    }

    static constexpr int kRackTomMidiNotes[] = { 48, 45, 43, 47, 50, 52 };
    static constexpr int kFloorTomMidiNotes[] = { 41, 50, 48, 45, 43 };

    int diameterInchesRounded (const DrumPiece& piece)
    {
        return juce::roundToInt (pieceDiameterPx (piece) / kLayoutPixelsPerInch);
    }

    void applyTomLabelsAndMidi (std::vector<DrumPiece*>& sorted, const int* notes, int noteCount)
    {
        const bool numbered = sorted.size() > 1;
        const auto typeLabel = sorted.front()->type == DrumPieceType::rackTom ? "Rack Tom" : "Floor Tom";

        for (size_t i = 0; i < sorted.size(); ++i)
        {
            auto* piece = sorted[i];
            const int inches = diameterInchesRounded (*piece);
            piece->name = numbered
                ? juce::String (inches) + "\" " + typeLabel + " " + juce::String ((int) i + 1)
                : juce::String (inches) + "\" " + typeLabel;

            const int midi = notes[juce::jmin ((int) i, noteCount - 1)];

            if (auto* art = piece->getPrimaryArticulation())
                art->midiNote = midi;

            piece->primaryMidiNote = midi;
            piece->syncMidiNotesFromArticulations();
        }
    }

    /** Small / high pitch on the left; anchor the existing row instead of fixed kit coords. */
    void relayoutRackTomRow (KitModel& model, const DrumPiece& addedPiece,
                             float canvasW, float canvasH)
    {
        std::vector<RackSlot> racks;

        for (auto& piece : model.getPiecesMutable())
        {
            if (piece.type != DrumPieceType::rackTom)
                continue;

            RackSlot slot;
            slot.piece = &piece;
            slot.savedCx = centerXNorm (piece, canvasW);
            slot.savedCy = centerYNorm (piece, canvasH);
            slot.radiusNormX = (piece.width * 0.5f) / canvasW;
            racks.push_back (slot);
        }

        if (racks.empty())
            return;

        if (racks.size() == 1)
        {
            setCenterNorm (*racks[0].piece, { 0.50f, 0.60f }, canvasW, canvasH);
            std::vector<DrumPiece*> only { racks[0].piece };
            applyTomLabelsAndMidi (only, kRackTomMidiNotes, 6);
            return;
        }

        std::sort (racks.begin(), racks.end(),
                   [] (const RackSlot& a, const RackSlot& b)
                   {
                       const float da = pieceDiameterPx (*a.piece);
                       const float db = pieceDiameterPx (*b.piece);
                       if (da != db)
                           return da < db;

                       return a.piece->primaryMidiNote < b.piece->primaryMidiNote;
                   });

        const RackSlot* anchorPreExisting = nullptr;

        for (const auto& slot : racks)
        {
            if (slot.piece->id == addedPiece.id)
                continue;

            if (anchorPreExisting == nullptr
                || pieceDiameterPx (*slot.piece) > pieceDiameterPx (*anchorPreExisting->piece))
            {
                anchorPreExisting = &slot;
            }
        }

        int anchorIdx = 0;
        float anchorCx = 0.50f;
        float rowY = 0.60f;

        if (anchorPreExisting != nullptr)
        {
            for (size_t i = 0; i < racks.size(); ++i)
            {
                if (racks[(size_t) i].piece->id == anchorPreExisting->piece->id)
                {
                    anchorIdx = (int) i;
                    break;
                }
            }

            anchorCx = anchorPreExisting->savedCx;

            std::vector<float> rowYs;

            for (const auto& slot : racks)
                if (slot.piece->id != addedPiece.id)
                    rowYs.push_back (slot.savedCy);

            rowY = median (rowYs);
        }

        const float gapNorm = kGapPx / canvasW;
        std::vector<float> centersX (racks.size());
        centersX[(size_t) anchorIdx] = anchorCx;

        for (int i = anchorIdx + 1; i < (int) racks.size(); ++i)
        {
            const float prevR = racks[(size_t) (i - 1)].radiusNormX;
            const float currR = racks[(size_t) i].radiusNormX;
            centersX[(size_t) i] = centersX[(size_t) (i - 1)] + prevR + gapNorm + currR;
        }

        for (int i = anchorIdx - 1; i >= 0; --i)
        {
            const float nextR = racks[(size_t) (i + 1)].radiusNormX;
            const float currR = racks[(size_t) i].radiusNormX;
            centersX[(size_t) i] = centersX[(size_t) (i + 1)] - currR - gapNorm - nextR;
        }

        std::vector<DrumPiece*> sortedPieces;
        sortedPieces.reserve (racks.size());

        for (size_t i = 0; i < racks.size(); ++i)
        {
            setCenterNorm (*racks[i].piece, { centersX[i], rowY }, canvasW, canvasH);
            sortedPieces.push_back (racks[i].piece);
        }

        applyTomLabelsAndMidi (sortedPieces, kRackTomMidiNotes, 6);
    }

    struct DiagonalSlot
    {
        DrumPiece* piece = nullptr;
        float savedCx = 0.5f;
        float savedCy = 0.5f;
    };

    static constexpr int kRideEdgeMidiNotes[] = { 51, 59, 52, 53 };

    void applyRideLabelsAndMidi (std::vector<DrumPiece*>& sorted)
    {
        const bool numbered = sorted.size() > 1;

        for (size_t i = 0; i < sorted.size(); ++i)
        {
            auto* piece = sorted[i];
            const int inches = diameterInchesRounded (*piece);
            piece->name = numbered
                ? juce::String (inches) + "\" Ride " + juce::String ((int) i + 1)
                : juce::String (inches) + "\" Ride";

            const int edgeMidi = kRideEdgeMidiNotes[juce::jmin ((int) i, 3)];
            const int bellMidi = edgeMidi + 2;

            for (auto& art : piece->articulations)
            {
                if (art.name == "Edge")
                    art.midiNote = edgeMidi;
                else if (art.name == "Bell")
                    art.midiNote = bellMidi;
            }

            piece->primaryMidiNote = edgeMidi;
            piece->syncMidiNotesFromArticulations();
        }
    }

    /** 45° chain down-right; anchor the largest pre-existing piece (floor tom / ride layout). */
    void relayoutDiagonalDownRightCluster (std::vector<DiagonalSlot>& slots,
                                           const DrumPiece& addedPiece,
                                           NormPos defaultAnchor,
                                           float canvasW, float canvasH,
                                           const std::function<void (std::vector<DrumPiece*>&)>& applyLabels)
    {
        if (slots.empty())
            return;

        if (slots.size() == 1)
        {
            setCenterNorm (*slots[0].piece, defaultAnchor, canvasW, canvasH);
            std::vector<DrumPiece*> only { slots[0].piece };
            applyLabels (only);
            return;
        }

        std::sort (slots.begin(), slots.end(),
                   [] (const DiagonalSlot& a, const DiagonalSlot& b)
                   {
                       const float da = pieceDiameterPx (*a.piece);
                       const float db = pieceDiameterPx (*b.piece);
                       if (da != db)
                           return da < db;

                       return a.piece->primaryMidiNote < b.piece->primaryMidiNote;
                   });

        const DiagonalSlot* anchorPreExisting = nullptr;

        for (const auto& slot : slots)
        {
            if (slot.piece->id == addedPiece.id)
                continue;

            if (anchorPreExisting == nullptr
                || pieceDiameterPx (*slot.piece) > pieceDiameterPx (*anchorPreExisting->piece))
            {
                anchorPreExisting = &slot;
            }
        }

        int anchorIdx = 0;
        NormPos anchorPos = defaultAnchor;

        if (anchorPreExisting != nullptr)
        {
            for (size_t i = 0; i < slots.size(); ++i)
            {
                if (slots[i].piece->id == anchorPreExisting->piece->id)
                {
                    anchorIdx = (int) i;
                    break;
                }
            }

            anchorPos = { anchorPreExisting->savedCx, anchorPreExisting->savedCy };
        }

        std::vector<NormPos> centers (slots.size());
        centers[(size_t) anchorIdx] = anchorPos;

        for (int i = anchorIdx + 1; i < (int) slots.size(); ++i)
        {
            const auto& prev = slots[(size_t) (i - 1)];
            const auto& curr = slots[(size_t) i];
            const float stepPx = (prev.piece->width + curr.piece->width) * 0.5f + kGapPx;
            const float stepNormX = stepPx * kInvSqrt2 / canvasW;
            const float stepNormY = stepPx * kInvSqrt2 / canvasH;
            centers[(size_t) i].x = centers[(size_t) (i - 1)].x + stepNormX;
            centers[(size_t) i].y = centers[(size_t) (i - 1)].y + stepNormY;
        }

        for (int i = anchorIdx - 1; i >= 0; --i)
        {
            const auto& next = slots[(size_t) (i + 1)];
            const auto& curr = slots[(size_t) i];
            const float stepPx = (curr.piece->width + next.piece->width) * 0.5f + kGapPx;
            const float stepNormX = stepPx * kInvSqrt2 / canvasW;
            const float stepNormY = stepPx * kInvSqrt2 / canvasH;
            centers[(size_t) i].x = centers[(size_t) (i + 1)].x - stepNormX;
            centers[(size_t) i].y = centers[(size_t) (i + 1)].y - stepNormY;
        }

        std::vector<DrumPiece*> sortedPieces;
        sortedPieces.reserve (slots.size());

        for (size_t i = 0; i < slots.size(); ++i)
        {
            setCenterNorm (*slots[i].piece, centers[i], canvasW, canvasH);
            sortedPieces.push_back (slots[i].piece);
        }

        applyLabels (sortedPieces);
    }

    std::vector<DiagonalSlot> collectDiagonalSlots (KitModel& model, DrumPieceType type,
                                                    float canvasW, float canvasH)
    {
        std::vector<DiagonalSlot> slots;

        for (auto& piece : model.getPiecesMutable())
        {
            if (piece.type != type)
                continue;

            DiagonalSlot slot;
            slot.piece = &piece;
            slot.savedCx = centerXNorm (piece, canvasW);
            slot.savedCy = centerYNorm (piece, canvasH);
            slots.push_back (slot);
        }

        return slots;
    }

    void relayoutFloorTomRow (KitModel& model, const DrumPiece& addedPiece,
                              float canvasW, float canvasH)
    {
        auto slots = collectDiagonalSlots (model, DrumPieceType::floorTom, canvasW, canvasH);

        relayoutDiagonalDownRightCluster (slots, addedPiece, { 0.60f, 0.75f }, canvasW, canvasH,
                                          [] (std::vector<DrumPiece*>& sorted)
                                          {
                                              applyTomLabelsAndMidi (sorted, kFloorTomMidiNotes, 5);
                                          });
    }

    void relayoutRideRow (KitModel& model, const DrumPiece& addedPiece,
                          float canvasW, float canvasH)
    {
        auto slots = collectDiagonalSlots (model, DrumPieceType::ride, canvasW, canvasH);

        relayoutDiagonalDownRightCluster (slots, addedPiece, { 0.60f, 0.75f }, canvasW, canvasH,
                                          [] (std::vector<DrumPiece*>& sorted)
                                          {
                                              applyRideLabelsAndMidi (sorted);
                                          });
    }

    const DrumPiece* findLeftmostKick (const KitModel& model)
    {
        const DrumPiece* leftmost = nullptr;
        float minLeft = std::numeric_limits<float>::max();

        for (const auto& piece : model.getPieces())
        {
            if (piece.type != DrumPieceType::kick)
                continue;

            if (piece.x < minLeft)
            {
                minLeft = piece.x;
                leftmost = &piece;
            }
        }

        return leftmost;
    }

    /** Top-right of leftmost kick → first hat; chain steps toward bottom-left (mirror of floor toms). */
    NormPos hiHatCenterFromKickTopRight (const DrumPiece& hat, const DrumPiece& kick,
                                         float canvasW, float canvasH)
    {
        const float r = hat.width * 0.5f;
        const float offsetPx = r + kGapPx;
        const float kickRight = kick.x + kick.width;
        const float kickTop = kick.y;
        const float cx = kickRight + offsetPx * kInvSqrt2;
        const float cy = kickTop - offsetPx * kInvSqrt2;
        return { cx / canvasW, cy / canvasH };
    }

    NormPos stepTowardBottomLeft (NormPos origin, float prevDiameterPx, float currDiameterPx,
                                  float canvasW, float canvasH)
    {
        const float stepPx = (prevDiameterPx + currDiameterPx) * 0.5f + kGapPx;
        return {
            origin.x - stepPx * kInvSqrt2 / canvasW,
            origin.y + stepPx * kInvSqrt2 / canvasH
        };
    }

    void applyHiHatLabels (const std::vector<DrumPiece*>& sorted)
    {
        const bool numbered = sorted.size() > 1;

        for (size_t i = 0; i < sorted.size(); ++i)
        {
            auto* piece = sorted[i];
            const int inches = diameterInchesRounded (*piece);
            piece->name = numbered
                ? juce::String (inches) + "\" Hi-Hat " + juce::String ((int) i + 1)
                : juce::String (inches) + "\" Hi-Hat";
        }
    }

    void relayoutHiHatChain (KitModel& model, const DrumPiece& addedPiece,
                             float canvasW, float canvasH)
    {
        struct HatSlot
        {
            DrumPiece* piece = nullptr;
            float savedCx = 0.5f;
            float savedCy = 0.5f;
        };

        std::vector<HatSlot> hats;

        for (auto& piece : model.getPiecesMutable())
        {
            if (piece.type != DrumPieceType::hiHat)
                continue;

            HatSlot slot;
            slot.piece = &piece;
            slot.savedCx = centerXNorm (piece, canvasW);
            slot.savedCy = centerYNorm (piece, canvasH);
            hats.push_back (slot);
        }

        if (hats.empty())
            return;

        const bool addedIsInChain = std::any_of (hats.begin(), hats.end(),
                                                 [&addedPiece] (const HatSlot& s)
                                                 {
                                                     return s.piece->id == addedPiece.id;
                                                 });

        if (! addedIsInChain)
            return;

        if (hats.size() == 1)
        {
            NormPos anchor { 0.28f, 0.30f };

            if (const auto* kick = findLeftmostKick (model))
                anchor = hiHatCenterFromKickTopRight (*hats[0].piece, *kick, canvasW, canvasH);

            setCenterNorm (*hats[0].piece, anchor, canvasW, canvasH);
            std::vector<DrumPiece*> only { hats[0].piece };
            applyHiHatLabels (only);
            return;
        }

        std::sort (hats.begin(), hats.end(),
                   [] (const HatSlot& a, const HatSlot& b)
                   {
                       const float da = pieceDiameterPx (*a.piece);
                       const float db = pieceDiameterPx (*b.piece);
                       if (da != db)
                           return da < db;

                       return a.piece->primaryMidiNote < b.piece->primaryMidiNote;
                   });

        int addedIdx = -1;

        for (size_t i = 0; i < hats.size(); ++i)
        {
            if (hats[i].piece->id == addedPiece.id)
            {
                addedIdx = (int) i;
                break;
            }
        }

        if (addedIdx < 0)
            return;

        const auto* kick = findLeftmostKick (model);
        NormPos newCenter { 0.28f, 0.30f };

        if (addedIdx == 0)
        {
            newCenter = kick != nullptr
                ? hiHatCenterFromKickTopRight (*hats[0].piece, *kick, canvasW, canvasH)
                : NormPos { 0.28f, 0.30f };

            setCenterNorm (*hats[0].piece, newCenter, canvasW, canvasH);

            for (size_t i = 1; i < hats.size(); ++i)
            {
                newCenter = stepTowardBottomLeft (newCenter,
                                                  hats[i - 1].piece->width,
                                                  hats[i].piece->width,
                                                  canvasW, canvasH);
                setCenterNorm (*hats[i].piece, newCenter, canvasW, canvasH);
            }
        }
        else
        {
            const auto& prev = hats[(size_t) (addedIdx - 1)];
            newCenter = stepTowardBottomLeft ({ prev.savedCx, prev.savedCy },
                                              prev.piece->width,
                                              hats[(size_t) addedIdx].piece->width,
                                              canvasW, canvasH);
            setCenterNorm (*hats[(size_t) addedIdx].piece, newCenter, canvasW, canvasH);
        }

        std::vector<DrumPiece*> sortedPieces;
        sortedPieces.reserve (hats.size());

        for (const auto& slot : hats)
            sortedPieces.push_back (slot.piece);

        applyHiHatLabels (sortedPieces);
    }
}

void applyTomLayoutForNewPiece (KitModel& model, DrumPiece& addedPiece,
                                float canvasWidth, float canvasHeight)
{
    switch (addedPiece.type)
    {
        case DrumPieceType::rackTom:
            relayoutRackTomRow (model, addedPiece, canvasWidth, canvasHeight);
            break;

        case DrumPieceType::floorTom:
            relayoutFloorTomRow (model, addedPiece, canvasWidth, canvasHeight);
            break;

        default:
            break;
    }
}

void applyHiHatLayoutForNewPiece (KitModel& model, DrumPiece& addedPiece,
                                  float canvasWidth, float canvasHeight)
{
    if (addedPiece.type != DrumPieceType::hiHat)
        return;

    relayoutHiHatChain (model, addedPiece, canvasWidth, canvasHeight);
}

void applyRideLayoutForNewPiece (KitModel& model, DrumPiece& addedPiece,
                                 float canvasWidth, float canvasHeight)
{
    if (addedPiece.type != DrumPieceType::ride)
        return;

    relayoutRideRow (model, addedPiece, canvasWidth, canvasHeight);
}
