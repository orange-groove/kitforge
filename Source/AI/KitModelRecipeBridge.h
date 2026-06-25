#pragma once

#include "KitRecipe.h"

class KitModel;

/** Converts between KitModel and KitRecipe for AI edit/refine flows. */
KitRecipe kitRecipeFromModel (const KitModel& model, float canvasWidth, float canvasHeight);
