#pragma once

class KitModel;
struct KitRecipe;

/** Applies shell + cymbal arc layout to an imported kit (reuses AI layout enforcer). */
void applyImportKitLayout (KitModel& model, float canvasWidth = 900.0f, float canvasHeight = 520.0f);
