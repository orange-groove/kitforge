#include "DemoKitSampleBindings.h"
#include "DemoSampleAssets.h"
#include "../Engine/Articulation.h"

namespace
{
    void assignRelativeSample (Articulation& art, const juce::String& relativePath)
    {
        art.layers.clear();

        SampleLayer layer;
        layer.id = SampleLayer::makeId();
        layer.minVelocity = 1;
        layer.maxVelocity = 127;

        DrumSample sample;
        sample.id = DrumSample::makeId();
        sample.filePath = relativePath;
        layer.roundRobins.addSample (std::move (sample));

        art.layers.push_back (std::move (layer));
    }

    bool articulationNeedsSampleBinding (const Articulation& art)
    {
        for (const auto& layer : art.layers)
        {
            for (const auto& sample : layer.roundRobins.samples)
            {
                if (sample.filePath.isEmpty())
                    continue;

                if (juce::File (sample.filePath).existsAsFile())
                    return false;
            }
        }

        return true;
    }
}

juce::String DemoKitSampleBindings::sampleFileNameFor (DrumPieceType type, const juce::String& articulationName)
{
    const auto artSlug = articulationName.toLowerCase().replaceCharacter (' ', '_');

    switch (type)
    {
        case DrumPieceType::kick:     return "kick_" + artSlug + ".wav";
        case DrumPieceType::snare:    return "snare_" + artSlug + ".wav";
        case DrumPieceType::rackTom:  return "rack_tom_" + artSlug + ".wav";
        case DrumPieceType::floorTom: return "floor_tom_" + artSlug + ".wav";
        case DrumPieceType::hiHat:    return "hihat_" + artSlug + ".wav";
        case DrumPieceType::crash:    return "crash_" + artSlug + ".wav";
        case DrumPieceType::ride:
            if (artSlug == "edge" || artSlug == "bow")
                return "ride_bow.wav";

            return "ride_" + artSlug + ".wav";
        default:                      return "drum_" + artSlug + ".wav";
    }
}

bool DemoKitSampleBindings::writeBundledSamplesToFolder (const juce::File& kitRoot)
{
    const auto samplesDir = kitRoot.getChildFile ("samples");
    samplesDir.createDirectory();

    static const char* kAllDemoFiles[] =
    {
        "kick_center.wav", "snare_center.wav", "snare_rimshot.wav",
        "rack_tom_hit.wav", "floor_tom_hit.wav", "hihat_closed.wav",
        "hihat_open.wav", "crash_hit.wav", "ride_bow.wav"
    };

    bool anyWritten = false;

    for (const auto* fileName : kAllDemoFiles)
    {
        if (DemoSampleAssets::writeBundledSample (samplesDir.getChildFile (fileName), fileName))
            anyWritten = true;
    }

    return anyWritten;
}

void DemoKitSampleBindings::bindSamplePathsFromFolder (KitModel& model, const juce::File& kitRoot)
{
    const auto samplesDir = kitRoot.getChildFile ("samples");

    if (! samplesDir.isDirectory())
        return;

    for (const auto& pieceRef : model.getPieces())
    {
        auto* piece = model.findPieceById (pieceRef.id);

        if (piece == nullptr)
            continue;

        for (auto& art : piece->articulations)
        {
            if (! articulationNeedsSampleBinding (art))
                continue;

            const auto fileName = sampleFileNameFor (piece->type, art.name);
            const auto wavFile = samplesDir.getChildFile (fileName);

            if (! wavFile.existsAsFile())
                continue;

            assignRelativeSample (art, juce::String ("samples/") + fileName);
        }
    }
}

bool DemoKitSampleBindings::kitHasAssignedSamples (const KitModel& model)
{
    for (const auto& piece : model.getPieces())
    {
        for (const auto& art : piece.articulations)
        {
            for (const auto& layer : art.layers)
            {
                for (const auto& sample : layer.roundRobins.samples)
                {
                    if (sample.filePath.isNotEmpty())
                        return true;
                }
            }
        }
    }

    return false;
}
