#pragma once

#include "SampleLayer.h"

/** A playable articulation (e.g. snare center, rimshot, closed hat) mapped to a MIDI note. */
struct Articulation
{
    juce::String id;
    juce::String name;
    int midiNote = 36;
    juce::String chokeGroupId;
    std::vector<SampleLayer> layers;

    static juce::String makeId() { return juce::Uuid().toString(); }

    SampleLayer* findLayerForVelocity (int velocity)
    {
        for (auto& layer : layers)
            if (layer.containsVelocity (velocity))
                return &layer;

        return layers.empty() ? nullptr : &layers.back();
    }

    const SampleLayer* findLayerForVelocity (int velocity) const
    {
        return const_cast<Articulation*> (this)->findLayerForVelocity (velocity);
    }

    SampleLayer* findLayerById (const juce::String& layerId)
    {
        for (auto& layer : layers)
            if (layer.id == layerId)
                return &layer;

        return nullptr;
    }

    SampleLayer* getOrCreateDefaultLayer()
    {
        if (layers.empty())
        {
            SampleLayer layer;
            layer.id = SampleLayer::makeId();
            layer.minVelocity = 1;
            layer.maxVelocity = 127;
            layers.push_back (std::move (layer));
        }

        return &layers.front();
    }
};
