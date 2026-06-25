#pragma once

#include "DrumSample.h"
#include <vector>

/** Manages round-robin cycling across multiple sample variants. */
class RoundRobinGroup
{
public:
    std::vector<DrumSample> samples;
    int currentRoundRobinIndex = 0;

    bool isEmpty() const { return samples.empty(); }
    int size() const { return (int) samples.size(); }

    void addSample (DrumSample sample)
    {
        if (sample.id.isEmpty())
            sample.id = DrumSample::makeId();

        samples.push_back (std::move (sample));
    }

    /** Select next sample and advance the round-robin index (UI/preview only; engine owns audio-thread RR). */
    const DrumSample* selectNext()
    {
        if (samples.empty())
            return nullptr;

        const auto index = currentRoundRobinIndex;
        currentRoundRobinIndex = (currentRoundRobinIndex + 1) % (int) samples.size();
        return &samples[(size_t) index];
    }
};
