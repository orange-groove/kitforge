#pragma once

#include "RoundRobinGroup.h"

/** Velocity layer containing one or more round-robin sample variants. */
struct SampleLayer
{
    juce::String id;
    int minVelocity = 1;
    int maxVelocity = 127;
    RoundRobinGroup roundRobins;

    static juce::String makeId() { return juce::Uuid().toString(); }

    bool containsVelocity (int velocity) const
    {
        return velocity >= minVelocity && velocity <= maxVelocity;
    }

    void clampVelocityRange()
    {
        minVelocity = juce::jlimit (1, 127, minVelocity);
        maxVelocity = juce::jlimit (1, 127, maxVelocity);

        if (minVelocity > maxVelocity)
            std::swap (minVelocity, maxVelocity);
    }
};
