#include "KitPackageJsonSerializer.h"
#include "KitSerializer.h"
#include "../Models/DrumPieceTypes.h"

namespace
{
    constexpr float kLayoutRefWidth  = 980.0f;
    constexpr float kLayoutRefHeight = 680.0f;

    juce::String colourToHex (juce::Colour colour)
    {
        return "#" + juce::String::toHexString ((int) colour.getARGB()).substring (2).toUpperCase();
    }

    juce::Colour colourFromHex (const juce::String& hex)
    {
        auto s = hex.trim();

        if (s.startsWithChar ('#'))
            s = s.substring (1);

        return juce::Colour ((juce::uint32) s.getHexValue32() | 0xff000000);
    }

    float pitchToSemitones (float pitchRatio)
    {
        return 12.0f * std::log2 (juce::jmax (pitchRatio, 0.001f));
    }

    float semitonesToPitch (float semitones)
    {
        return std::pow (2.0f, semitones / 12.0f);
    }

    bool hasPackageLayoutKeys (const juce::var& pieceVar)
    {
        if (auto* obj = pieceVar.getDynamicObject())
            return obj->hasProperty ("layoutXNorm");

        return false;
    }
}

int KitPackageJsonSerializer::countSamplesInKit (const KitModel& kit)
{
    int count = 0;

    for (const auto& piece : kit.getPieces())
    {
        for (const auto& art : piece.articulations)
        {
            for (const auto& layer : art.layers)
                count += (int) layer.roundRobins.samples.size();
        }
    }

    return count;
}

juce::var KitPackageJsonSerializer::kitToPackageVar (const KitModel& kit, const juce::String& packageId)
{
    juce::Array<juce::var> pieceVars;

    for (const auto& piece : kit.getPieces())
    {
        juce::Array<juce::var> artVars;

        for (const auto& art : piece.articulations)
        {
            juce::Array<juce::var> layerVars;

            for (const auto& layer : art.layers)
            {
                juce::Array<juce::var> sampleVars;

                for (const auto& sample : layer.roundRobins.samples)
                {
                    auto* sampleObj = new juce::DynamicObject();
                    sampleObj->setProperty ("id", sample.id);
                    sampleObj->setProperty ("samplePath", sample.filePath);
                    sampleObj->setProperty ("rootMidiNote", sample.rootMidiNote);
                    sampleObj->setProperty ("gain", sample.gain);
                    sampleObj->setProperty ("pan", sample.pan);
                    sampleObj->setProperty ("pitch", pitchToSemitones (sample.pitch));
                    sampleObj->setProperty ("startOffsetSamples", sample.startOffsetSamples);
                    sampleObj->setProperty ("endOffsetSamples", sample.endOffsetSamples);
                    sampleVars.add (juce::var (sampleObj));
                }

                auto* layerObj = new juce::DynamicObject();
                layerObj->setProperty ("id", layer.id);
                layerObj->setProperty ("minVelocity", layer.minVelocity);
                layerObj->setProperty ("maxVelocity", layer.maxVelocity);
                layerObj->setProperty ("roundRobins", sampleVars);
                layerVars.add (juce::var (layerObj));
            }

            auto* artObj = new juce::DynamicObject();
            artObj->setProperty ("id", art.id);
            artObj->setProperty ("name", art.name);
            artObj->setProperty ("midiNote", art.midiNote);
            artObj->setProperty ("chokeGroupId", art.chokeGroupId);
            artObj->setProperty ("layers", layerVars);
            artVars.add (juce::var (artObj));
        }

        juce::Array<juce::var> notes;

        for (const auto note : piece.midiNotes)
            notes.add (note);

        auto* pieceObj = new juce::DynamicObject();
        pieceObj->setProperty ("id", piece.id);
        pieceObj->setProperty ("type", drumPieceTypeToString (piece.type));
        pieceObj->setProperty ("name", piece.name);
        pieceObj->setProperty ("primaryMidiNote", piece.primaryMidiNote);
        pieceObj->setProperty ("midiNotes", notes);
        pieceObj->setProperty ("layoutXNorm", piece.x / kLayoutRefWidth);
        pieceObj->setProperty ("layoutYNorm", piece.y / kLayoutRefHeight);
        pieceObj->setProperty ("widthNorm", piece.width / kLayoutRefWidth);
        pieceObj->setProperty ("heightNorm", piece.height / kLayoutRefHeight);
        pieceObj->setProperty ("rotation", piece.rotation);
        pieceObj->setProperty ("color", colourToHex (piece.color));
        pieceObj->setProperty ("chokeGroupId", piece.chokeGroupId);
        pieceObj->setProperty ("outputChannelPair", piece.outputChannelPair);
        pieceObj->setProperty ("volume", piece.volume);
        pieceObj->setProperty ("pan", piece.pan);
        pieceObj->setProperty ("pitch", pitchToSemitones (piece.pitch));
        pieceObj->setProperty ("muted", piece.muted);
        pieceObj->setProperty ("soloed", piece.soloed);
        pieceObj->setProperty ("shapeType", shapeTypeToString (piece.shapeType));
        pieceObj->setProperty ("articulations", artVars);
        pieceVars.add (juce::var (pieceObj));
    }

    auto* root = new juce::DynamicObject();
    root->setProperty ("id", packageId);
    root->setProperty ("name", kit.kitName);
    root->setProperty ("description", juce::String());
    root->setProperty ("mapProfile", "General MIDI");
    root->setProperty ("tags", juce::Array<juce::var>());
    root->setProperty ("pieces", pieceVars);
    return juce::var (root);
}

void KitPackageJsonSerializer::kitFromPackageVar (KitModel& model, const juce::var& v)
{
    if (auto* root = v.getDynamicObject())
    {
        if (auto* pieces = root->getProperty ("pieces").getArray())
        {
            if (pieces->size() > 0 && hasPackageLayoutKeys (pieces->getReference (0)))
            {
                KitModel loaded = KitModel::createEmpty();
                loaded.kitName = root->getProperty ("name").toString();

                if (loaded.kitName.isEmpty())
                    loaded.kitName = root->getProperty ("kitName").toString();

                for (const auto& pieceVar : *pieces)
                {
                    DrumPiece piece;

                    if (auto* obj = pieceVar.getDynamicObject())
                    {
                        piece.id = obj->getProperty ("id").toString();
                        piece.name = obj->getProperty ("name").toString();
                        piece.type = drumPieceTypeFromString (obj->getProperty ("type").toString());
                        piece.primaryMidiNote = (int) obj->getProperty ("primaryMidiNote");
                        piece.chokeGroupId = obj->getProperty ("chokeGroupId").toString();
                        piece.outputChannelPair = (int) obj->getProperty ("outputChannelPair");
                        piece.volume = (float) obj->getProperty ("volume");
                        piece.pan = (float) obj->getProperty ("pan");
                        piece.pitch = semitonesToPitch ((float) obj->getProperty ("pitch"));
                        piece.muted = (bool) obj->getProperty ("muted");
                        piece.soloed = (bool) obj->getProperty ("soloed");
                        piece.rotation = (float) obj->getProperty ("rotation");
                        piece.shapeType = shapeTypeFromString (obj->getProperty ("shapeType").toString());
                        piece.color = colourFromHex (obj->getProperty ("color").toString());

                        piece.x = (float) obj->getProperty ("layoutXNorm") * kLayoutRefWidth;
                        piece.y = (float) obj->getProperty ("layoutYNorm") * kLayoutRefHeight;
                        piece.width = (float) obj->getProperty ("widthNorm") * kLayoutRefWidth;
                        piece.height = (float) obj->getProperty ("heightNorm") * kLayoutRefHeight;

                        if (auto* noteArr = obj->getProperty ("midiNotes").getArray())
                        {
                            for (const auto& noteVar : *noteArr)
                                piece.midiNotes.addIfNotAlreadyThere ((int) noteVar);
                        }

                        if (auto* artArr = obj->getProperty ("articulations").getArray())
                        {
                            for (const auto& artVar : *artArr)
                            {
                                Articulation art;

                                if (auto* artObj = artVar.getDynamicObject())
                                {
                                    art.id = artObj->getProperty ("id").toString();
                                    art.name = artObj->getProperty ("name").toString();
                                    art.midiNote = (int) artObj->getProperty ("midiNote");
                                    art.chokeGroupId = artObj->getProperty ("chokeGroupId").toString();

                                    if (auto* layerArr = artObj->getProperty ("layers").getArray())
                                    {
                                        for (const auto& layerVar : *layerArr)
                                        {
                                            SampleLayer layer;

                                            if (auto* layerObj = layerVar.getDynamicObject())
                                            {
                                                layer.id = layerObj->getProperty ("id").toString();
                                                layer.minVelocity = (int) layerObj->getProperty ("minVelocity");
                                                layer.maxVelocity = (int) layerObj->getProperty ("maxVelocity");

                                                if (auto* rrArr = layerObj->getProperty ("roundRobins").getArray())
                                                {
                                                    for (const auto& sampleVar : *rrArr)
                                                    {
                                                        DrumSample sample;

                                                        if (auto* sampleObj = sampleVar.getDynamicObject())
                                                        {
                                                            sample.id = sampleObj->getProperty ("id").toString();
                                                            sample.filePath = sampleObj->getProperty ("samplePath").toString();

                                                            if (sample.filePath.isEmpty())
                                                                sample.filePath = sampleObj->getProperty ("filePath").toString();

                                                            sample.rootMidiNote = (int) sampleObj->getProperty ("rootMidiNote");
                                                            sample.gain = (float) sampleObj->getProperty ("gain");
                                                            sample.pan = (float) sampleObj->getProperty ("pan");

                                                            if (sampleObj->hasProperty ("pitch"))
                                                            {
                                                                const auto pitchVal = sampleObj->getProperty ("pitch");

                                                                if ((float) pitchVal > 4.0f || (float) pitchVal < 0.25f)
                                                                    sample.pitch = semitonesToPitch ((float) pitchVal);
                                                                else
                                                                    sample.pitch = (float) pitchVal;
                                                            }

                                                            sample.startOffsetSamples = (int) sampleObj->getProperty ("startOffsetSamples");
                                                            sample.endOffsetSamples = (int) sampleObj->getProperty ("endOffsetSamples");
                                                        }

                                                        if (sample.id.isEmpty())
                                                            sample.id = DrumSample::makeId();

                                                        layer.roundRobins.addSample (std::move (sample));
                                                    }
                                                }
                                            }

                                            if (layer.id.isEmpty())
                                                layer.id = SampleLayer::makeId();

                                            art.layers.push_back (std::move (layer));
                                        }
                                    }
                                }

                                if (art.id.isEmpty())
                                    art.id = Articulation::makeId();

                                piece.articulations.push_back (std::move (art));
                            }
                        }
                    }

                    if (piece.id.isEmpty())
                        piece.id = DrumPiece::makeId();

                    piece.syncMidiNotesFromArticulations();
                    normalizePieceVisuals (piece);
                    ensureStandardArticulations (piece);
                    loaded.addPiece (std::move (piece));
                }

                model.importContents (loaded);
                return;
            }
        }
    }

    KitSerializer::kitFromVar (model, v);
}

bool KitPackageJsonSerializer::readKitFromFile (const juce::File& file, KitModel& modelOut, juce::String& errorOut)
{
    if (! file.existsAsFile())
    {
        errorOut = "Missing kit.json";
        return false;
    }

    juce::var parsed;

    if (juce::JSON::parse (file.loadFileAsString(), parsed).failed())
    {
        errorOut = "Failed to parse kit.json";
        return false;
    }

    kitFromPackageVar (modelOut, parsed);
    return true;
}

bool KitPackageJsonSerializer::writeKitToFile (const juce::File& file, const KitModel& kit, const juce::String& packageId, juce::String& errorOut)
{
    juce::ignoreUnused (errorOut);
    return file.replaceWithText (juce::JSON::toString (kitToPackageVar (kit, packageId), true));
}
