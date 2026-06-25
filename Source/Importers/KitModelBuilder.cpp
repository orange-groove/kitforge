#include "KitModelBuilder.h"
#include "../Engine/Articulation.h"
#include "../Engine/DrumSample.h"
#include "../Engine/SampleLayer.h"

namespace
{
    juce::String sanitizeArticulationName (const juce::String& name)
    {
        if (name.isEmpty())
            return "Hit";

        return name.substring (0, 1).toUpperCase() + name.substring (1);
    }
}

KitModel KitModelBuilder::buildFromMetadata (const juce::String& kitName,
                                              const std::vector<SampleMetadata>& samples,
                                              std::vector<ImportWarning>& warnings)
{
    KitModel model = KitModel::createEmpty();
    model.kitName = kitName;

    juce::HashMap<juce::String, DrumPiece*> piecesByKey;
    juce::HashMap<int, juce::String> midiNoteOwners;

    for (const auto& meta : samples)
    {
        if (meta.confidence <= 0.0f && meta.instrumentType == DrumPieceType::accessory)
        {
            warnings.push_back (ImportWarning::make (ImportWarningType::couldNotInferInstrument,
                                                     "Could not infer instrument for: "
                                                         + juce::File (meta.filePath).getFileName(),
                                                     meta.filePath));
        }

        if (meta.velocityValue <= 0 && meta.minVelocity == 1 && meta.maxVelocity == 127)
        {
            warnings.push_back (ImportWarning::make (ImportWarningType::missingVelocityInfo,
                                                     "No velocity info: "
                                                         + juce::File (meta.filePath).getFileName(),
                                                     meta.filePath));
        }

        if (meta.midiNote > 0)
        {
            if (midiNoteOwners.contains (meta.midiNote))
            {
                const auto owner = midiNoteOwners[meta.midiNote];

                if (owner != pieceKeyToString ({ meta.instrumentType, meta.instrumentIndex })
                    + "|" + articulationKey (meta))
                {
                    warnings.push_back (ImportWarning::make (ImportWarningType::duplicateMidiNote,
                                                             "Duplicate MIDI note "
                                                                 + juce::String (meta.midiNote)
                                                                 + " for "
                                                                 + juce::File (meta.filePath).getFileName(),
                                                             meta.filePath));
                }
            }
            else
            {
                midiNoteOwners.set (meta.midiNote,
                                    pieceKeyToString ({ meta.instrumentType, meta.instrumentIndex })
                                        + "|" + articulationKey (meta));
            }
        }

        addSampleToModel (model, piecesByKey, meta, warnings);
    }

    return model;
}

juce::String KitModelBuilder::pieceKeyToString (const PieceKey& key)
{
    return drumPieceTypeToString (key.type) + "_" + juce::String (key.index);
}

juce::String KitModelBuilder::pieceDisplayName (const PieceKey& key)
{
    switch (key.type)
    {
        case DrumPieceType::kick:     return "Kick";
        case DrumPieceType::snare:    return "Snare";
        case DrumPieceType::hiHat:    return "Hi-Hat";
        case DrumPieceType::ride:     return "Ride";
        case DrumPieceType::china:    return "China";
        case DrumPieceType::splash:   return "Splash";
        case DrumPieceType::rackTom:  return "Rack Tom " + juce::String (key.index + 1);
        case DrumPieceType::floorTom: return "Floor Tom " + juce::String (key.index + 1);
        case DrumPieceType::crash:    return "Crash " + juce::String (key.index + 1);
        default:                      return "Accessory " + juce::String (key.index + 1);
    }
}

juce::String KitModelBuilder::articulationKey (const SampleMetadata& meta)
{
    return meta.articulation.toLowerCase();
}

void KitModelBuilder::applyPieceDefaults (DrumPiece& piece, const PieceKey& key)
{
    piece.type = key.type;
    piece.name = pieceDisplayName (key);
    piece.width = defaultPieceSize (key.type);
    piece.height = defaultPieceSize (key.type);
    normalizePieceVisuals (piece);

    if (key.type == DrumPieceType::hiHat)
        piece.chokeGroupId = "hihat";
}

void KitModelBuilder::addSampleToModel (KitModel& model,
                                         juce::HashMap<juce::String, DrumPiece*>& piecesByKey,
                                         const SampleMetadata& meta,
                                         std::vector<ImportWarning>& warnings)
{
    juce::ignoreUnused (warnings);

    const PieceKey pieceKey { meta.instrumentType, meta.instrumentIndex };
    const auto pKeyStr = pieceKeyToString (pieceKey);
    DrumPiece* piece = piecesByKey[pKeyStr];

    if (piece == nullptr)
    {
        DrumPiece newPiece;
        newPiece.id = DrumPiece::makeId();
        applyPieceDefaults (newPiece, pieceKey);
        newPiece.primaryMidiNote = meta.midiNote;

        auto& added = model.addPiece (std::move (newPiece));
        piece = &added;
        piecesByKey.set (pKeyStr, piece);
    }

    const auto artName = sanitizeArticulationName (meta.articulation);
    Articulation* art = piece->findArticulationByMidiNote (meta.midiNote);

    if (art == nullptr)
    {
        for (auto& existing : piece->articulations)
        {
            if (existing.name.equalsIgnoreCase (artName))
            {
                art = &existing;
                break;
            }
        }
    }

    if (art == nullptr)
    {
        Articulation newArt;
        newArt.id = Articulation::makeId();
        newArt.name = artName;
        newArt.midiNote = meta.midiNote;
        newArt.chokeGroupId = meta.chokeGroupId;
        art = &piece->addArticulation (std::move (newArt));
    }

    SampleLayer* layer = nullptr;

    for (auto& existing : art->layers)
    {
        if (existing.minVelocity == meta.minVelocity && existing.maxVelocity == meta.maxVelocity)
        {
            layer = &existing;
            break;
        }
    }

    if (layer == nullptr)
    {
        SampleLayer newLayer;
        newLayer.id = SampleLayer::makeId();
        newLayer.minVelocity = meta.minVelocity;
        newLayer.maxVelocity = meta.maxVelocity;
        art->layers.push_back (std::move (newLayer));
        layer = &art->layers.back();
    }

    DrumSample sample;
    sample.id = DrumSample::makeId();
    sample.filePath = meta.filePath;
    sample.rootMidiNote = meta.midiNote;

    if (! juce::File (meta.filePath).existsAsFile())
    {
        warnings.push_back (ImportWarning::make (ImportWarningType::sampleFileMissing,
                                                 "Sample file missing: " + meta.filePath,
                                                 meta.filePath));
    }

    if (meta.roundRobinIndex > 0)
    {
        auto& rrSamples = layer->roundRobins.samples;
        const int insertAt = juce::jmax (0, meta.roundRobinIndex - 1);

        if (insertAt >= (int) rrSamples.size())
            rrSamples.push_back (std::move (sample));
        else
            rrSamples.insert (rrSamples.begin() + insertAt, std::move (sample));
    }
    else
    {
        layer->roundRobins.addSample (std::move (sample));
    }
    piece->syncMidiNotesFromArticulations();
}
