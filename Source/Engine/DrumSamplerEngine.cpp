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
    juce::StringArray paths;
    collectReferencedPaths (model, paths);
    preloadSamples (paths);
    syncFromModel (model, paths);
}

void DrumSamplerEngine::preloadSamples (const juce::StringArray& paths)
{
    sampleLoader.loadMany (paths);
}

void DrumSamplerEngine::syncFromModel (const KitModel& model, const juce::StringArray& referencedPaths)
{
    stopAllVoices();
    sampleLoader.pruneUnused (referencedPaths);
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
    std::vector<PendingTrigger> ready;

    {
        const juce::ScopedLock lock (triggerLock);
        ready.swap (pendingTriggers);
    }

    for (const auto& trigger : ready)
    {
        if (trigger.isPreview)
        {
            playPreview (trigger.previewPath, trigger.velocity);
        }
        else if (trigger.usePieceId)
        {
            if (trigger.articulationId.isNotEmpty())
                triggerPieceArticulation (model, trigger.pieceId, trigger.articulationId, trigger.velocity);
            else
                triggerPiece (model, trigger.pieceId, trigger.velocity);
        }
        else
            triggerMidiNote (model, trigger.midiNote, trigger.velocity);
    }
}

void DrumSamplerEngine::triggerMidiNote (const KitModel& model, int midiNote, float velocity)
{
    const auto ref = model.findArticulationByMidiNote (midiNote);

    if (ref.piece != nullptr && ref.articulation != nullptr)
        playArticulation (*ref.piece, *ref.articulation, velocity);
}

void DrumSamplerEngine::triggerPiece (const juce::String& pieceId, float velocity)
{
    queueTriggerPiece (pieceId, velocity);
}

void DrumSamplerEngine::triggerArticulation (const juce::String& pieceId,
                                              const juce::String& articulationId, float velocity)
{
    queueTriggerArticulation (pieceId, articulationId, velocity);
}

void DrumSamplerEngine::triggerPiece (const KitModel& model, const juce::String& pieceId, float velocity)
{
    if (const auto* piece = model.findPieceById (pieceId))
        if (const auto* art = piece->getPrimaryArticulation())
            playArticulation (*piece, *art, velocity);
}

void DrumSamplerEngine::triggerPieceArticulation (const KitModel& model, const juce::String& pieceId,
                                                   const juce::String& articulationId, float velocity)
{
    if (const auto* piece = model.findPieceById (pieceId))
    {
        if (const auto* art = piece->findArticulationById (articulationId))
        {
            playArticulation (*piece, *art, velocity);
            return;
        }

        if (const auto* art = piece->getPrimaryArticulation())
            playArticulation (*piece, *art, velocity);
    }
}

void DrumSamplerEngine::queueTriggerPiece (const juce::String& pieceId, float velocity)
{
    const juce::ScopedLock lock (triggerLock);
    pendingTriggers.push_back ({ 0, velocity, pieceId, {}, true });
}

void DrumSamplerEngine::queueTriggerArticulation (const juce::String& pieceId,
                                                   const juce::String& articulationId, float velocity)
{
    const juce::ScopedLock lock (triggerLock);
    pendingTriggers.push_back ({ 0, velocity, pieceId, articulationId, true });
}

void DrumSamplerEngine::queuePreviewFile (const juce::String& absolutePath, float velocity)
{
    if (absolutePath.isEmpty())
        return;

    PendingTrigger trigger;
    trigger.velocity = velocity;
    trigger.isPreview = true;
    trigger.previewPath = absolutePath;

    const juce::ScopedLock lock (triggerLock);
    pendingTriggers.push_back (std::move (trigger));
}

void DrumSamplerEngine::playPreview (const juce::String& absolutePath, float velocity)
{
    const auto* loaded = sampleLoader.getCached (absolutePath);

    if (loaded == nullptr)
        return;

    DrumSample sample;
    sample.filePath = absolutePath;

    if (auto* voice = allocateVoice())
        voice->start (loaded, sample, velocity, 1.0f, 0.0f, 1.0f, {}, hostSampleRate);
}

void DrumSamplerEngine::playArticulation (const DrumPiece& piece, const Articulation& art, float velocity)
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

    const auto* loaded = sampleLoader.getCached (drumSample.filePath);

    if (loaded == nullptr)
        return;

    // Record the hit for UI feedback (wobble/flash); lock-free, RT-safe.
    hitCounters[(size_t) clampMidiNote (art.midiNote)].fetch_add (1, std::memory_order_relaxed);

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

void DrumSamplerEngine::readHitCounters (juce::uint32* dest) const
{
    for (int i = 0; i < kNumMidiNotes; ++i)
        dest[i] = hitCounters[(size_t) i].load (std::memory_order_relaxed);
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

    {
        const juce::ScopedLock lock (triggerLock);
        pendingTriggers.clear();
    }

    roundRobinIndices.clear();
}
