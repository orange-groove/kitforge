#include "KitSerializer.h"
#include "../Core/DemoKitSampleBindings.h"

namespace
{
    constexpr float kLayoutRefWidth  = 980.0f;
    constexpr float kLayoutRefHeight = 680.0f;

    void denormalizePieceLayoutIfNeeded (DrumPiece& piece)
    {
        const bool looksNormalized = piece.x >= 0.0f && piece.x <= 1.0f
                                  && piece.y >= 0.0f && piece.y <= 1.0f
                                  && piece.width > 0.0f && piece.width <= 1.0f
                                  && piece.height > 0.0f && piece.height <= 1.0f;

        if (! looksNormalized)
            return;

        piece.x *= kLayoutRefWidth;
        piece.y *= kLayoutRefHeight;
        piece.width *= kLayoutRefWidth;
        piece.height *= kLayoutRefHeight;
    }
}

juce::var KitSerializer::drumSampleToVar (const DrumSample& sample)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("id", sample.id);
    obj->setProperty ("filePath", sample.filePath);

    if (sample.sampleRef.hasRef())
        obj->setProperty ("sampleRef", sample.sampleRef.toVar());

    obj->setProperty ("rootMidiNote", sample.rootMidiNote);
    obj->setProperty ("gain", sample.gain);
    obj->setProperty ("pan", sample.pan);
    obj->setProperty ("pitch", sample.pitch);
    obj->setProperty ("startOffsetSamples", sample.startOffsetSamples);
    obj->setProperty ("endOffsetSamples", sample.endOffsetSamples);
    return juce::var (obj);
}

DrumSample KitSerializer::drumSampleFromVar (const juce::var& v)
{
    DrumSample sample;

    if (auto* obj = v.getDynamicObject())
    {
        sample.id = obj->getProperty ("id").toString();
        sample.filePath = obj->getProperty ("filePath").toString();

        if (obj->hasProperty ("sampleRef"))
            sample.sampleRef = SampleRef::fromVar (obj->getProperty ("sampleRef"));

        sample.rootMidiNote = (int) obj->getProperty ("rootMidiNote");
        sample.gain = (float) obj->getProperty ("gain");
        sample.pan = (float) obj->getProperty ("pan");
        sample.pitch = (float) obj->getProperty ("pitch");
        sample.startOffsetSamples = (int) obj->getProperty ("startOffsetSamples");
        sample.endOffsetSamples = (int) obj->getProperty ("endOffsetSamples");
    }

    if (sample.id.isEmpty())
        sample.id = DrumSample::makeId();

    return sample;
}

juce::var KitSerializer::sampleLayerToVar (const SampleLayer& layer)
{
    juce::Array<juce::var> samples;

    for (const auto& sample : layer.roundRobins.samples)
        samples.add (drumSampleToVar (sample));

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("id", layer.id);
    obj->setProperty ("minVelocity", layer.minVelocity);
    obj->setProperty ("maxVelocity", layer.maxVelocity);
    obj->setProperty ("currentRoundRobinIndex", layer.roundRobins.currentRoundRobinIndex);
    obj->setProperty ("roundRobins", samples);
    return juce::var (obj);
}

SampleLayer KitSerializer::sampleLayerFromVar (const juce::var& v)
{
    SampleLayer layer;

    if (auto* obj = v.getDynamicObject())
    {
        layer.id = obj->getProperty ("id").toString();
        layer.minVelocity = (int) obj->getProperty ("minVelocity");
        layer.maxVelocity = (int) obj->getProperty ("maxVelocity");
        layer.roundRobins.currentRoundRobinIndex = (int) obj->getProperty ("currentRoundRobinIndex");

        if (auto* arr = obj->getProperty ("roundRobins").getArray())
        {
            for (const auto& item : *arr)
                layer.roundRobins.addSample (drumSampleFromVar (item));
        }
    }

    if (layer.id.isEmpty())
        layer.id = SampleLayer::makeId();

    layer.clampVelocityRange();
    return layer;
}

juce::var KitSerializer::articulationToVar (const Articulation& art)
{
    juce::Array<juce::var> layers;

    for (const auto& layer : art.layers)
        layers.add (sampleLayerToVar (layer));

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("id", art.id);
    obj->setProperty ("name", art.name);
    obj->setProperty ("midiNote", art.midiNote);
    obj->setProperty ("chokeGroupId", art.chokeGroupId);
    obj->setProperty ("layers", layers);
    return juce::var (obj);
}

Articulation KitSerializer::articulationFromVar (const juce::var& v)
{
    Articulation art;

    if (auto* obj = v.getDynamicObject())
    {
        art.id = obj->getProperty ("id").toString();
        art.name = obj->getProperty ("name").toString();
        art.midiNote = (int) obj->getProperty ("midiNote");
        art.chokeGroupId = obj->getProperty ("chokeGroupId").toString();

        if (auto* arr = obj->getProperty ("layers").getArray())
        {
            for (const auto& item : *arr)
                art.layers.push_back (sampleLayerFromVar (item));
        }
    }

    if (art.id.isEmpty())
        art.id = Articulation::makeId();

    return art;
}

juce::var KitSerializer::articulationToUiVar (const Articulation& art)
{
    int sampleCount = 0;

    for (const auto& layer : art.layers)
        sampleCount += (int) layer.roundRobins.samples.size();

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("id", art.id);
    obj->setProperty ("name", art.name);
    obj->setProperty ("midiNote", art.midiNote);
    obj->setProperty ("chokeGroupId", art.chokeGroupId);
    obj->setProperty ("hasSample", sampleCount > 0);
    obj->setProperty ("layers", juce::Array<juce::var>());
    return juce::var (obj);
}

juce::var KitSerializer::pieceToVar (const DrumPiece& piece)
{
    juce::Array<juce::var> arts;

    for (const auto& art : piece.articulations)
        arts.add (articulationToVar (art));

    juce::Array<juce::var> notes;

    for (const auto note : piece.midiNotes)
        notes.add (note);

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("id", piece.id);
    obj->setProperty ("name", piece.name);
    obj->setProperty ("type", drumPieceTypeToString (piece.type));
    obj->setProperty ("midiNotes", notes);
    obj->setProperty ("primaryMidiNote", piece.primaryMidiNote);
    obj->setProperty ("chokeGroupId", piece.chokeGroupId);
    obj->setProperty ("outputChannelPair", piece.outputChannelPair);
    obj->setProperty ("volume", piece.volume);
    obj->setProperty ("pan", piece.pan);
    obj->setProperty ("pitch", piece.pitch);
    obj->setProperty ("muted", piece.muted);
    obj->setProperty ("soloed", piece.soloed);
    obj->setProperty ("x", piece.x);
    obj->setProperty ("y", piece.y);
    obj->setProperty ("width", piece.width);
    obj->setProperty ("height", piece.height);
    obj->setProperty ("rotation", piece.rotation);
    obj->setProperty ("color", (int) piece.color.getARGB());
    obj->setProperty ("shapeType", shapeTypeToString (piece.shapeType));
    obj->setProperty ("articulations", arts);
    return juce::var (obj);
}

juce::var KitSerializer::pieceToUiVar (const DrumPiece& piece)
{
    juce::Array<juce::var> arts;

    for (const auto& art : piece.articulations)
        arts.add (articulationToUiVar (art));

    juce::Array<juce::var> notes;

    for (const auto note : piece.midiNotes)
        notes.add (note);

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("id", piece.id);
    obj->setProperty ("name", piece.name);
    obj->setProperty ("type", drumPieceTypeToString (piece.type));
    obj->setProperty ("midiNotes", notes);
    obj->setProperty ("primaryMidiNote", piece.primaryMidiNote);
    obj->setProperty ("chokeGroupId", piece.chokeGroupId);
    obj->setProperty ("outputChannelPair", piece.outputChannelPair);
    obj->setProperty ("volume", piece.volume);
    obj->setProperty ("pan", piece.pan);
    obj->setProperty ("pitch", piece.pitch);
    obj->setProperty ("muted", piece.muted);
    obj->setProperty ("soloed", piece.soloed);
    obj->setProperty ("x", piece.x);
    obj->setProperty ("y", piece.y);
    obj->setProperty ("width", piece.width);
    obj->setProperty ("height", piece.height);
    obj->setProperty ("rotation", piece.rotation);
    obj->setProperty ("color", (int) piece.color.getARGB());
    obj->setProperty ("shapeType", shapeTypeToString (piece.shapeType));
    obj->setProperty ("articulations", arts);
    return juce::var (obj);
}

DrumPiece KitSerializer::pieceFromVar (const juce::var& v)
{
    DrumPiece piece;

    if (auto* obj = v.getDynamicObject())
    {
        piece.id = obj->getProperty ("id").toString();
        piece.name = obj->getProperty ("name").toString();
        piece.type = drumPieceTypeFromString (obj->getProperty ("type").toString());
        piece.primaryMidiNote = (int) obj->getProperty ("primaryMidiNote");
        piece.chokeGroupId = obj->getProperty ("chokeGroupId").toString();
        piece.outputChannelPair = (int) obj->getProperty ("outputChannelPair");
        piece.volume = (float) obj->getProperty ("volume");
        piece.pan = (float) obj->getProperty ("pan");
        piece.pitch = (float) obj->getProperty ("pitch");
        piece.muted = (bool) obj->getProperty ("muted");
        piece.soloed = (bool) obj->getProperty ("soloed");
        piece.x = (float) obj->getProperty ("x");
        piece.y = (float) obj->getProperty ("y");
        piece.width = (float) obj->getProperty ("width");
        piece.height = (float) obj->getProperty ("height");
        piece.rotation = (float) obj->getProperty ("rotation");
        piece.color = juce::Colour ((juce::uint32) (int) obj->getProperty ("color"));
        piece.shapeType = shapeTypeFromString (obj->getProperty ("shapeType").toString());

        if (auto* noteArr = obj->getProperty ("midiNotes").getArray())
        {
            for (const auto& noteVar : *noteArr)
                piece.midiNotes.addIfNotAlreadyThere ((int) noteVar);
        }

        if (auto* artArr = obj->getProperty ("articulations").getArray())
        {
            for (const auto& item : *artArr)
                piece.articulations.push_back (articulationFromVar (item));
        }
    }

    if (piece.id.isEmpty())
        piece.id = DrumPiece::makeId();

    piece.syncMidiNotesFromArticulations();
    normalizePieceVisuals (piece);
    denormalizePieceLayoutIfNeeded (piece);
    ensureStandardArticulations (piece);
    return piece;
}

juce::var KitSerializer::kitToVar (const KitModel& model)
{
    juce::Array<juce::var> pieces;

    for (const auto& piece : model.getPieces())
        pieces.add (pieceToVar (piece));

    auto* root = new juce::DynamicObject();
    root->setProperty ("version", 2);
    root->setProperty ("kitName", model.kitName);
    root->setProperty ("pieces", pieces);
    return juce::var (root);
}

juce::var KitSerializer::kitToUiVar (const KitModel& model)
{
    juce::Array<juce::var> pieces;

    for (const auto& piece : model.getPieces())
        pieces.add (pieceToUiVar (piece));

    auto* root = new juce::DynamicObject();
    root->setProperty ("version", 2);
    root->setProperty ("kitName", model.kitName);
    root->setProperty ("pieces", pieces);
    return juce::var (root);
}

void KitSerializer::kitFromVar (KitModel& model, const juce::var& v)
{
    KitModel loaded = KitModel::createEmpty();

    if (auto* root = v.getDynamicObject())
    {
        loaded.kitName = root->getProperty ("kitName").toString();

        if (auto* arr = root->getProperty ("pieces").getArray())
        {
            for (const auto& item : *arr)
                loaded.addPiece (pieceFromVar (item));
        }
    }

    model.importContents (loaded);
}

bool KitSerializer::saveKitToFile (const KitModel& model, const juce::File& file)
{
    return file.replaceWithText (juce::JSON::toString (kitToVar (model), true));
}

bool KitSerializer::loadKitFromFile (KitModel& model, const juce::File& file)
{
    juce::var parsed;

    if (juce::JSON::parse (file.loadFileAsString(), parsed).failed())
        return false;

    kitFromVar (model, parsed);
    model.resolveSamplePaths (file.getParentDirectory());
    DemoKitSampleBindings::bindSamplePathsFromFolder (model, file.getParentDirectory());
    model.resolveSamplePaths (file.getParentDirectory());
    return true;
}
