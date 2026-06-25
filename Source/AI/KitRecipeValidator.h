#pragma once

#include "KitRecipe.h"

struct KitRecipeValidationResult
{
    bool ok = true;
    juce::StringArray errors;
    juce::StringArray warnings;
};

/** Validates AI-generated KitRecipe JSON before kit construction. */
class KitRecipeValidator
{
public:
    KitRecipeValidationResult validate (const KitRecipe& recipe) const;
    KitRecipeValidationResult validateVar (const juce::var& recipeJson) const;
};
