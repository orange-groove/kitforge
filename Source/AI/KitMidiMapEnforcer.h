#pragma once

#include "KitRecipe.h"

/** Applies canonical MIDI notes for supported e-drum maps after the LLM response. */
void applyMidiMapToRecipe (KitRecipe& recipe, const juce::String& prompt);
