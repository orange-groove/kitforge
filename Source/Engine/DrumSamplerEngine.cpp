#include "DrumSamplerEngine.h"
#include <unordered_map>

namespace
{
    int clampMidiNote (int note)
    {
        return juce::jlimit (0, 127, note);
    }
}

DrumSamplerEngine::DrumSamplerEngine() = default;

void DrumSamplerEngine::prepare (double sampleRate, int samplesPerBlock)
{
    hostSampleRate = sampleRate;
    sampleLoader.prepare (sampleRate);
    juce::ignoreUnused (samplesPerBlock);
}

void DrumSamplerEngine::rebuildFromModel (const KitModel& model)
{
    stopAllVoices();

    juce::StringArray paths;
    collectReferencedPaths (model, paths);

    for (const auto& path : paths)
        sampleLoader.load (path);

    sampleLoader.pruneUnused (paths);
    buildMidiMap (model);
}

void DrumSamplerEngine::collectReferencedPaths (const KitModel& model, juce::StringArray& paths) const
{
    for (const auto& piece : model.getPieces())
    {
        for (const auto& art : piece.articulations)
        {
            for (const auto& layer : art.layers)
            {
                for (const auto& sample : layer.roundRobins.samples)
                {
                    if (sample.filePath.isNotEmpty() && ! paths.contains (sample.filePath))
                        paths.add (sample.filePath);
                }
            }
        }
    }
}

void DrumSamplerEngine::buildMidiMap (const KitModel& model)
{
    midiMap.clear();
    midiMap.resize (128);

    for (const auto& piece : model.getPieces())
    {
        for (const auto& art : piece.articulations)
        {
            const int note = clampMidiNote (art.midiNote);
            midiMap[(size_t) note] = { piece.id, art.id };
        }
    }
}

void DrumSamplerEngine::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                                      const KitModel& model, int startSample, int numSamples)
{
    processPendingTriggers (model);

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
            triggerMidiNote (model, message.getNoteNumber(), (float) message.getVelocity());
    }

    releaseFinishedVoices();

    for (auto& voice : voices)
    {
        if (voice.isActive())
            voice.renderNextBlock (buffer, startSample, numSamples);
    }

    releaseFinishedVoices();
}

void DrumSamplerEngine::processPendingTriggers (const KitModel& model)
{
    const int numReady = pendingFifo.getNumReady();

    for (int i = 0; i < numReady; ++i)
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        pendingFifo.prepareToRead (1, start1, size1, start2, size2);

        const auto trigger = pendingTriggers[(size_t) start1];
        pendingFifo.finishedRead (1);

        if (trigger.usePieceId)
            triggerPiece (model, trigger.pieceId, trigger.velocity);
        else
            triggerMidiNote (model, trigger.midiNote, trigger.velocity);
    }
}

void DrumSamplerEngine::triggerMidiNote (const KitModel& model, int midiNote, float velocity)
{
    const auto ref = model.findArticulationByMidiNote (midiNote);

    if (ref.piece != nullptr && ref.articulation != nullptr)
        triggerArticulation (*ref.piece, *ref.articulation, velocity);
}

void DrumSamplerEngine::triggerPiece (const KitModel& model, const juce::String& pieceId, float velocity)
{
    if (const auto* piece = model.findPieceById (pieceId))
        if (const auto* art = piece->getPrimaryArticulation())
            triggerArticulation (*piece, *art, velocity);
}

void DrumSamplerEngine::queueTriggerPiece (const juce::String& pieceId, float velocity)
{
    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    pendingFifo.prepareToWrite (1, start1, size1, start2, size2);

    if (size1 > 0)
    {
        pendingTriggers[(size_t) start1] = { 0, velocity, pieceId, true };
        pendingFifo.finishedWrite (1);
    }
}

void DrumSamplerEngine::triggerArticulation (const DrumPiece& piece, const Articulation& art, float velocity)
{
    if (piece.muted)
        return;

    const auto* layer = art.findLayerForVelocity ((int) velocity);

    if (layer == nullptr || layer->roundRobins.isEmpty())
        return;

    const auto layerKey = layer->id.toStdString();
    auto& rrIndex = roundRobinIndices[layerKey];
    const auto& samples = layer->roundRobins.samples;
    const auto& drumSample = samples[(size_t) (rrIndex % samples.size())];
    rrIndex = (rrIndex + 1) % (int) samples.size();

    if (drumSample.filePath.isEmpty())
        return;

    const auto* loaded = sampleLoader.load (drumSample.filePath);

    if (loaded == nullptr)
        return;

    const auto chokeId = piece.effectiveChokeGroupId (art);

    if (chokeId.isNotEmpty())
        chokeManager.chokeGroup (chokeId);

    if (auto* voice = allocateVoice())
    {
        voice->start (loaded, drumSample, velocity, piece.volume, piece.pan, piece.pitch,
                      chokeId, hostSampleRate);
        chokeManager.registerVoice (voice, chokeId);
    }
}

DrumVoice* DrumSamplerEngine::allocateVoice()
{
    for (auto& voice : voices)
    {
        if (! voice.isActive())
            return &voice;
    }

    voices.front().forceStop();
    chokeManager.unregisterVoice (&voices.front());
    return &voices.front();
}

void DrumSamplerEngine::releaseFinishedVoices()
{
    for (auto& voice : voices)
    {
        if (! voice.isActive())
            chokeManager.unregisterVoice (&voice);
    }
}

void DrumSamplerEngine::stopAllVoices()
{
    for (auto& voice : voices)
        voice.forceStop();

    chokeManager.clear();
    pendingFifo.reset();
    roundRobinIndices.clear();
}
