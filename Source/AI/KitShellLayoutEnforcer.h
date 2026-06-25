#pragma once

#include "KitRecipe.h"

/** Parses N from "7 piece" / "7-piece" / "7pc" etc. Returns 0 if not specified. */
int parseRequestedShellCount (const juce::String& prompt);

/** True when the user is editing the kit on canvas, not replacing it entirely. */
bool isRefineKitPrompt (const juce::String& prompt);

int countShellsInRecipe (const KitRecipe& recipe);

/** Rebuilds shells to match targetShells using the canonical template. Cymbals untouched. */
void enforceShellCount (KitRecipe& recipe, int targetShells);

/** Applies default cymbal positions for new kits. */
void applyDefaultCymbalLayout (KitRecipe& recipe);

/** True when the prompt references cymbals or a specific cymbal type. */
bool promptMentionsCymbals (const juce::String& prompt);

/** Adds the standard cymbal set when the prompt does not mention cymbals. */
void enforceDefaultCymbals (KitRecipe& recipe, const juce::String& prompt);
