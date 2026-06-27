#include "SampleSwapService.h"

std::vector<SampleLayer> SampleSwapService::cloneLayers (const std::vector<SampleLayer>& src)
{
    std::vector<SampleLayer> out;
    out.reserve (src.size());

    for (const auto& srcLayer : src)
    {
        SampleLayer layer;
        layer.id = SampleLayer::makeId();
        layer.minVelocity = srcLayer.minVelocity;
        layer.maxVelocity = srcLayer.maxVelocity;
        layer.clampVelocityRange();

        for (const auto& srcSample : srcLayer.roundRobins.samples)
        {
            DrumSample sample = srcSample;       // keeps filePath + sampleRef
            sample.id = DrumSample::makeId();    // fresh id so round-robin state stays distinct
            layer.roundRobins.addSample (std::move (sample));
        }

        out.push_back (std::move (layer));
    }

    return out;
}

SampleSwapService::Result SampleSwapService::swap (KitModel& kit,
                                                   const SampleSet& set,
                                                   const Target& target,
                                                   const Options& options)
{
    Result result;

    if (set.layers.empty())
    {
        result.errorMessage = "Selected sample set has no samples.";
        return result;
    }

    auto* piece = kit.findPieceById (target.pieceId);

    if (piece == nullptr)
    {
        result.errorMessage = "Target piece not found.";
        return result;
    }

    switch (target.mode)
    {
        case Mode::articulation:
        {
            auto* art = target.articulationId.isNotEmpty()
                            ? piece->findArticulationById (target.articulationId)
                            : piece->getPrimaryArticulation();

            if (art == nullptr)
            {
                result.errorMessage = "Target articulation not found.";
                return result;
            }

            // Preserve identity/name/midi/choke; replace only the sample content.
            art->layers = cloneLayers (set.layers);

            if (options.useSourceMidi && set.previewSampleRef.hasRef())
            {
                // rootMidiNote travels with samples; trigger note is left to the kit.
            }

            result.replacedCount = 1;
            break;
        }

        case Mode::layer:
        {
            auto* art = target.articulationId.isNotEmpty()
                            ? piece->findArticulationById (target.articulationId)
                            : piece->getPrimaryArticulation();

            if (art == nullptr)
            {
                result.errorMessage = "Target articulation not found.";
                return result;
            }

            SampleLayer* layer = target.layerId.isNotEmpty()
                                     ? art->findLayerById (target.layerId)
                                     : art->getOrCreateDefaultLayer();

            if (layer == nullptr)
            {
                result.errorMessage = "Target velocity layer not found.";
                return result;
            }

            const int keepMin = layer->minVelocity;
            const int keepMax = layer->maxVelocity;

            auto replacement = cloneLayers (set.layers);
            const auto& sourceLayer = replacement.front();

            layer->roundRobins = sourceLayer.roundRobins;

            if (! options.useSourceVelocityRanges)
            {
                layer->minVelocity = keepMin;
                layer->maxVelocity = keepMax;
            }
            else
            {
                layer->minVelocity = sourceLayer.minVelocity;
                layer->maxVelocity = sourceLayer.maxVelocity;
            }

            layer->clampVelocityRange();
            result.replacedCount = 1;
            break;
        }

        case Mode::piece:
        {
            if (piece->articulations.empty())
            {
                result.errorMessage = "Target piece has no articulations.";
                return result;
            }

            for (auto& art : piece->articulations)
            {
                art.layers = cloneLayers (set.layers);
                ++result.replacedCount;
            }

            if (options.useSourceName && set.displayName.isNotEmpty())
                piece->name = set.displayName;

            // Layout (x/y/width/height/rotation/color) and ids are intentionally untouched.
            break;
        }
    }

    piece->syncMidiNotesFromArticulations();
    result.success = result.replacedCount > 0;

    if (! result.success && result.errorMessage.isEmpty())
        result.errorMessage = "Nothing was replaced.";

    return result;
}
