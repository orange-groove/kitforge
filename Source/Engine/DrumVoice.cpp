#include "DrumVoice.h"

namespace
{
    float velocityToGain (float velocity)
    {
        return juce::jlimit (0.0f, 1.0f, velocity / 127.0f);
    }

    void applyPan (float pan, float& left, float& right)
    {
        left  = pan <= 0.0f ? 1.0f : 1.0f - pan;
        right = pan >= 0.0f ? 1.0f : 1.0f + pan;
    }
}

void DrumVoice::start (const LoadedSample* loadedSample,
                       const DrumSample& sampleMeta,
                       float velocity,
                       float pieceVolume,
                       float piecePan,
                       float piecePitch,
                       const juce::String& chokeGroupIdIn,
                       double hostSampleRateIn)
{
    if (loadedSample == nullptr || loadedSample->lengthInSamples <= 0)
    {
        clear();
        return;
    }

    loaded = loadedSample;
    chokeGroupId = chokeGroupIdIn;
    hostSampleRate = hostSampleRateIn;
    sourceSampleRate = loadedSample->sampleRate;

    startOffset = juce::jmax (0, sampleMeta.startOffsetSamples);
    endSample = sampleMeta.getEndSample (loadedSample->lengthInSamples);
    endSample = juce::jmax (startOffset, endSample);

    playhead = (double) startOffset;

    const float combinedPan = juce::jlimit (-1.0f, 1.0f, piecePan + sampleMeta.pan);
    applyPan (combinedPan, leftGain, rightGain);

    pitchRatio = juce::jlimit (0.25, 4.0, (double) (piecePitch * sampleMeta.pitch));
    gain = velocityToGain (velocity) * pieceVolume * sampleMeta.gain;
    active = true;
}

void DrumVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (! active || loaded == nullptr)
        return;

    const auto& source = loaded->buffer;

    if (loaded->lengthInSamples <= 0 || source.getNumSamples() <= 0)
    {
        forceStop();
        return;
    }

    const int numChannels = source.getNumChannels();
    const double sampleRateRatio = sourceSampleRate / hostSampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        const int sourceIndex = (int) playhead;

        if (sourceIndex >= endSample)
        {
            forceStop();
            break;
        }

        const float sample = source.getSample (0, sourceIndex) * gain;

        if (outputBuffer.getNumChannels() >= 1)
            outputBuffer.addSample (0, startSample + i, sample * leftGain);

        if (outputBuffer.getNumChannels() >= 2)
        {
            const float rightSample = numChannels > 1 ? source.getSample (1, sourceIndex) * gain : sample;
            outputBuffer.addSample (1, startSample + i, rightSample * rightGain);
        }

        playhead += pitchRatio * sampleRateRatio;
    }
}

void DrumVoice::forceStop()
{
    clear();
}

void DrumVoice::clear()
{
    active = false;
    loaded = nullptr;
    chokeGroupId.clear();
    playhead = 0.0;
}
