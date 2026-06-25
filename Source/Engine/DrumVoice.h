#pragma once

#include <JuceHeader.h>
#include "DrumSample.h"
#include "SampleLoader.h"

/** Single polyphonic voice playing one sample to completion or until choked. */
class DrumVoice
{
public:
    bool isActive() const { return active; }

    void start (const LoadedSample* loadedSample,
                const DrumSample& sampleMeta,
                float velocity,
                float pieceVolume,
                float piecePan,
                float piecePitch,
                const juce::String& chokeGroupId,
                double hostSampleRate);

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples);

    /** Immediate stop for choke groups. */
    void forceStop();

    juce::String getChokeGroupId() const { return chokeGroupId; }

private:
    bool active = false;
    const LoadedSample* loaded = nullptr;
    juce::String chokeGroupId;

    double sourceSampleRate = 44100.0;
    double hostSampleRate = 44100.0;
    double playhead = 0.0;
    double pitchRatio = 1.0;
    float gain = 1.0f;
    float leftGain = 1.0f;
    float rightGain = 1.0f;

    int startOffset = 0;
    int endSample = 0;

    void clear();
};
