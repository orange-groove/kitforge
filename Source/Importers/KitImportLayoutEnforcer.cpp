#include "KitImportLayoutEnforcer.h"
#include "../AI/KitModelRecipeBridge.h"
#include "../AI/KitShellLayoutEnforcer.h"
#include "../Models/KitModel.h"

namespace
{
    constexpr float kFloorAnchorX = 0.60f;
    constexpr float kFloorAnchorY = 0.75f;

    void assignShellLayout (KitRecipe& recipe)
    {
        for (auto& piece : recipe.pieces)
        {
            if (piece.type == DrumPieceType::kick)
            {
                if (piece.name.containsIgnoreCase ("bop"))
                {
                    piece.layoutXNorm = 0.36f;
                    piece.layoutYNorm = 0.70f;
                }
                else
                {
                    piece.layoutXNorm = 0.50f;
                    piece.layoutYNorm = 0.65f;
                }
            }
            else if (piece.type == DrumPieceType::snare)
            {
                piece.layoutXNorm = 0.40f;
                piece.layoutYNorm = 0.76f;
            }
            else if (piece.type == DrumPieceType::rackTom)
            {
                piece.layoutXNorm = 0.50f;
                piece.layoutYNorm = 0.60f;
            }
            else if (piece.type == DrumPieceType::floorTom)
            {
                piece.layoutXNorm = kFloorAnchorX;
                piece.layoutYNorm = kFloorAnchorY;
            }
        }
    }

    void applyRecipeLayoutToModel (KitModel& model, const KitRecipe& recipe,
                                   float canvasWidth, float canvasHeight)
    {
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
    }
}

void applyImportKitLayout (KitModel& model, float canvasWidth, float canvasHeight)
{
    auto recipe = kitRecipeFromModel (model, canvasWidth, canvasHeight);
    assignShellLayout (recipe);
    applyDefaultCymbalLayout (recipe);
    applyRecipeLayoutToModel (model, recipe, canvasWidth, canvasHeight);
    model.recordLayoutEdit();
}
