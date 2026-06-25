#include "KitRecipeValidator.h"

KitRecipeValidationResult KitRecipeValidator::validate (const KitRecipe& recipe) const
{
    KitRecipeValidationResult result;

    if (recipe.name.trim().isEmpty())
        result.errors.add ("Recipe must have a name.");

    if (recipe.pieces.empty())
        result.errors.add ("Recipe must contain at least one piece.");

    for (size_t i = 0; i < recipe.pieces.size(); ++i)
    {
        const auto& piece = recipe.pieces[i];

        if (piece.tags.isEmpty())
            result.warnings.add ("Piece " + juce::String ((int) i) + " has no tags — sample matching may be weak.");
    }

    result.ok = result.errors.isEmpty();
    return result;
}

KitRecipeValidationResult KitRecipeValidator::validateVar (const juce::var& recipeJson) const
{
    return validate (KitRecipe::fromVar (recipeJson));
}
