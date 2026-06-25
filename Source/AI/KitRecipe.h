#pragma once

#include <JuceHeader.h>
#include "../Models/DrumPieceTypes.h"
#include <vector>

struct KitRecipeArticulation
{
    juce::String name;
    int midiNote = 36;
    juce::String chokeGroupId;
};

struct KitRecipePiece
{
    juce::String id;
    DrumPieceType type = DrumPieceType::accessory;
    juce::String name;
    juce::String articulation;
    int preferredMidiNote = -1;
    juce::String chokeGroupId;
    juce::StringArray tags;
    float layoutXNorm = -1.0f;
    float layoutYNorm = -1.0f;
    std::vector<KitRecipeArticulation> articulations;
};

/** AI-generated kit blueprint — describes WHAT to build, not audio content. */
struct KitRecipe
{
    juce::String name;
    juce::String description;
    juce::String mapProfile;
    juce::StringArray globalTags;
    std::vector<KitRecipePiece> pieces;

    static KitRecipe fromVar (const juce::var& v);
    juce::var toVar() const;
};
