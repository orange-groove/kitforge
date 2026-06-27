#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"
#include "../Engine/DrumSample.h"
#include "../Engine/SampleLayer.h"
#include "../Engine/Articulation.h"

class KitSerializer
{
public:
    static juce::var drumSampleToVar (const DrumSample& sample);
    static DrumSample drumSampleFromVar (const juce::var& v);

    static juce::var sampleLayerToVar (const SampleLayer& layer);
    static SampleLayer sampleLayerFromVar (const juce::var& v);

    static juce::var articulationToVar (const Articulation& art);
    static Articulation articulationFromVar (const juce::var& v);
    static juce::var articulationToUiVar (const Articulation& art);

    static juce::var pieceToVar (const DrumPiece& piece);
    static DrumPiece pieceFromVar (const juce::var& v);
    static juce::var pieceToUiVar (const DrumPiece& piece);

    static juce::var kitToVar (const KitModel& model);
    static juce::var kitToUiVar (const KitModel& model);
    static void kitFromVar (KitModel& model, const juce::var& v);

    static bool saveKitToFile (const KitModel& model, const juce::File& file);
    static bool loadKitFromFile (KitModel& model, const juce::File& file);
};
