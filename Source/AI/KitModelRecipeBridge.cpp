#include "KitModelRecipeBridge.h"
#include "../Models/KitModel.h"

KitRecipe kitRecipeFromModel (const KitModel& model, float canvasWidth, float canvasHeight)
{
    KitRecipe recipe;
    recipe.name = model.kitName;

    const float safeW = juce::jmax (canvasWidth, 1.0f);
    const float safeH = juce::jmax (canvasHeight, 1.0f);

    for (const auto& piece : model.getPieces())
    {
        KitRecipePiece spec;
        spec.id = piece.id;
        spec.type = piece.type;
        spec.name = piece.name;
        spec.chokeGroupId = piece.chokeGroupId;
        spec.layoutXNorm = (piece.x + piece.width * 0.5f) / safeW;
        spec.layoutYNorm = (piece.y + piece.height * 0.5f) / safeH;

        for (const auto& art : piece.articulations)
            spec.articulations.push_back ({ art.name, art.midiNote, art.chokeGroupId });

        if (! spec.articulations.empty())
        {
            spec.articulation = spec.articulations.front().name;
            spec.preferredMidiNote = spec.articulations.front().midiNote;
        }
        else
        {
            spec.preferredMidiNote = piece.primaryMidiNote;
        }

        recipe.pieces.push_back (std::move (spec));
    }

    return recipe;
}
