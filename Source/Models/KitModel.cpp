#include "KitModel.h"
#include "../Engine/Articulation.h"

namespace
{
    constexpr int kPreferredNotes[] = { 36, 38, 40, 42, 46, 48, 43, 49, 51, 45, 47, 50, 52, 57, 41, 44 };

    Articulation makeArticulation (const juce::String& name, int midiNote, const juce::String& chokeGroupId = {})
    {
        Articulation art;
        art.id = Articulation::makeId();
        art.name = name;
        art.midiNote = midiNote;
        art.chokeGroupId = chokeGroupId;
        return art;
    }

    const Articulation* findBestRideEdgeSource (const DrumPiece& piece)
    {
        const Articulation* best = nullptr;
        int bestScore = -1;

        for (const auto& art : piece.articulations)
        {
            const bool nameMatch = art.name.equalsIgnoreCase ("Edge")
                || art.name.equalsIgnoreCase ("Bow")
                || art.name.equalsIgnoreCase ("Ride");

            if (! nameMatch)
                continue;

            int score = 0;

            if (! art.layers.empty())
                score += 100;

            if (art.midiNote == 51)
                score += 10;

            if (art.name.equalsIgnoreCase ("Edge"))
                score += 5;

            if (score > bestScore)
            {
                bestScore = score;
                best = &art;
            }
        }

        return best;
    }

    const Articulation* findBestRideBellSource (const DrumPiece& piece)
    {
        const Articulation* best = nullptr;
        int bestScore = -1;

        for (const auto& art : piece.articulations)
        {
            if (! art.name.equalsIgnoreCase ("Bell"))
                continue;

            int score = 0;

            if (! art.layers.empty())
                score += 100;

            if (art.midiNote == 53)
                score += 10;

            if (score > bestScore)
            {
                bestScore = score;
                best = &art;
            }
        }

        return best;
    }

    void copyLayersIfEmpty (Articulation& target, const Articulation& source)
    {
        if (target.layers.empty() && ! source.layers.empty())
            target.layers = source.layers;

        if (target.id.isEmpty() && source.id.isNotEmpty())
            target.id = source.id;
    }

    bool isBellSamplePath (const juce::String& path)
    {
        return path.toLowerCase().contains ("bell");
    }

    bool isRideBowSamplePath (const juce::String& path)
    {
        const auto lower = path.toLowerCase();
        return lower.contains ("ride") && ! lower.contains ("bell");
    }

    void appendSampleToArticulation (Articulation& articulation,
                                     const SampleLayer& layer,
                                     const DrumSample& sample)
    {
        SampleLayer* targetLayer = nullptr;

        for (auto& existing : articulation.layers)
        {
            if (existing.minVelocity == layer.minVelocity && existing.maxVelocity == layer.maxVelocity)
            {
                targetLayer = &existing;
                break;
            }
        }

        if (targetLayer == nullptr)
        {
            SampleLayer copy;
            copy.id = SampleLayer::makeId();
            copy.minVelocity = layer.minVelocity;
            copy.maxVelocity = layer.maxVelocity;
            articulation.layers.push_back (std::move (copy));
            targetLayer = &articulation.layers.back();
        }

        DrumSample copy = sample;

        if (copy.id.isEmpty())
            copy.id = DrumSample::makeId();

        targetLayer->roundRobins.addSample (std::move (copy));
    }

    juce::File resolvePathFromRoot (const juce::File& root, juce::String relativePath)
    {
        relativePath = relativePath.replaceCharacter ('\\', '/');

        if (relativePath.startsWithChar ('/'))
            return juce::File (relativePath);

        juce::File result = root;

        for (const auto& part : juce::StringArray::fromTokens (relativePath, "/", ""))
        {
            if (part == "..")
                result = result.getParentDirectory();
            else if (part.isNotEmpty() && part != ".")
                result = result.getChildFile (part);
        }

        return result;
    }
}

void ensureStandardArticulations (DrumPiece& piece)
{
    if (piece.type != DrumPieceType::ride)
        return;

    Articulation edge;
    Articulation bell;
    edge.name = "Edge";
    edge.midiNote = 51;
    bell.name = "Bell";
    bell.midiNote = 53;

    for (const auto& art : piece.articulations)
    {
        for (const auto& layer : art.layers)
        {
            for (const auto& sample : layer.roundRobins.samples)
            {
                if (isBellSamplePath (sample.filePath))
                    appendSampleToArticulation (bell, layer, sample);
                else if (isRideBowSamplePath (sample.filePath))
                    appendSampleToArticulation (edge, layer, sample);
                else if (art.name.equalsIgnoreCase ("Bell"))
                    appendSampleToArticulation (bell, layer, sample);
                else if (art.name.equalsIgnoreCase ("Bow")
                      || art.name.equalsIgnoreCase ("Edge")
                      || art.name.equalsIgnoreCase ("Ride"))
                    appendSampleToArticulation (edge, layer, sample);
            }
        }

        if (art.name.equalsIgnoreCase ("Edge") && edge.id.isEmpty())
            edge.id = art.id;

        if (art.name.equalsIgnoreCase ("Bell") && bell.id.isEmpty())
            bell.id = art.id;

        if ((art.name.equalsIgnoreCase ("Bow")
          || art.name.equalsIgnoreCase ("Ride"))
            && edge.id.isEmpty())
            edge.id = art.id;

        if (art.midiNote == 51 && edge.id.isEmpty())
            edge.id = art.id;

        if (art.midiNote == 53 && bell.id.isEmpty())
            bell.id = art.id;
    }

    if (edge.layers.empty())
    {
        if (const auto* src = findBestRideEdgeSource (piece))
            copyLayersIfEmpty (edge, *src);
    }

    if (bell.layers.empty())
    {
        if (const auto* src = findBestRideBellSource (piece))
            copyLayersIfEmpty (bell, *src);
    }

    if (edge.id.isEmpty())
        edge.id = Articulation::makeId();

    if (bell.id.isEmpty())
        bell.id = Articulation::makeId();

    piece.articulations.clear();
    piece.articulations.push_back (std::move (edge));
    piece.articulations.push_back (std::move (bell));
    piece.primaryMidiNote = 51;
    piece.syncMidiNotesFromArticulations();
}

KitModel::KitModel()
{
    createDefaultKit (900.0f, 520.0f);
}

KitModel::KitModel (EmptyInit)
{
    kitName = "Empty Kit";
}

DrumPiece* KitModel::findPieceById (const juce::String& id)
{
    for (auto& piece : pieces)
        if (piece.id == id)
            return &piece;

    return nullptr;
}

const DrumPiece* KitModel::findPieceById (const juce::String& id) const
{
    for (const auto& piece : pieces)
        if (piece.id == id)
            return &piece;

    return nullptr;
}

ArticulationRef KitModel::findArticulationByMidiNote (int midiNote)
{
    for (auto& piece : pieces)
        if (auto* art = piece.findArticulationByMidiNote (midiNote))
            return { &piece, art };

    return {};
}

ArticulationRef KitModel::findArticulationByMidiNote (int midiNote) const
{
    for (const auto& piece : pieces)
        if (const auto* art = piece.findArticulationByMidiNote (midiNote))
            return { const_cast<DrumPiece*> (&piece), const_cast<Articulation*> (art) };

    return {};
}

ArticulationRef KitModel::findArticulation (const juce::String& pieceId, const juce::String& articulationId)
{
    if (auto* piece = findPieceById (pieceId))
        if (auto* art = piece->findArticulationById (articulationId))
            return { piece, art };

    return {};
}

DrumPiece& KitModel::addPiece (DrumPiece piece)
{
    if (piece.id.isEmpty())
        piece.id = DrumPiece::makeId();

    piece.syncMidiNotesFromArticulations();
    pieces.push_back (std::move (piece));
    notifyChanged();
    return pieces.back();
}

DrumPiece KitModel::makeBasePiece (DrumPieceType type, const juce::String& name,
                                   float canvasWidth, float canvasHeight,
                                   float w, float h) const
{
    DrumPiece piece;
    piece.id = DrumPiece::makeId();
    piece.name = name;
    piece.type = type;
    piece.shapeType = defaultShapeForType (type);
    piece.color = colourForType (type);
    piece.width = w;
    piece.height = h;
    piece.x = (canvasWidth - w) * 0.5f;
    piece.y = (canvasHeight - h) * 0.5f;
    piece.primaryMidiNote = suggestNextMidiNote();
    return piece;
}

DrumPiece& KitModel::addDefaultDrum (DrumPieceType type, float canvasWidth, float canvasHeight)
{
    const float size = defaultPieceSize (type);
    auto piece = makeBasePiece (type, defaultPieceDisplayName (type), canvasWidth, canvasHeight, size, size);
    piece.shapeType = defaultShapeForType (type);
    piece.color = isCymbalPieceType (type) ? cymbalFillColour() : drumShellFillColour();

    if (type == DrumPieceType::kick)
        piece.addArticulation (makeArticulation ("Center", piece.primaryMidiNote));
    else if (type == DrumPieceType::snare)
    {
        piece.addArticulation (makeArticulation ("Center", piece.primaryMidiNote));

        int rimNote = 40;

        if (isMidiNoteInUse (rimNote) || rimNote == piece.primaryMidiNote)
            rimNote = suggestNextMidiNote();

        piece.addArticulation (makeArticulation ("Rimshot", rimNote));
        piece.syncMidiNotesFromArticulations();
    }
    else
        piece.addArticulation (makeArticulation ("Hit", piece.primaryMidiNote));

    normalizePieceVisuals (piece);
    return addPiece (std::move (piece));
}

DrumPiece& KitModel::addDefaultCymbal (DrumPieceType type, float canvasWidth, float canvasHeight)
{
    const float size = defaultPieceSize (type);
    auto piece = makeBasePiece (type, defaultPieceDisplayName (type), canvasWidth, canvasHeight, size, size);
    piece.shapeType = ShapeType::circle;
    piece.color = cymbalFillColour();

    if (type == DrumPieceType::hiHat)
    {
        piece.chokeGroupId = "hat";
        piece.addArticulation (makeArticulation ("Closed", piece.primaryMidiNote));

        int openNote = 46;

        if (isMidiNoteInUse (openNote) || openNote == piece.primaryMidiNote)
            openNote = suggestNextMidiNote();

        piece.addArticulation (makeArticulation ("Open", openNote, "hat"));
        piece.syncMidiNotesFromArticulations();
    }
    else if (type == DrumPieceType::ride)
    {
        piece.addArticulation (makeArticulation ("Edge", 51));
        piece.addArticulation (makeArticulation ("Bell", 53));
        piece.primaryMidiNote = 51;
        piece.syncMidiNotesFromArticulations();
    }
    else
    {
        piece.addArticulation (makeArticulation ("Hit", piece.primaryMidiNote));
    }

    normalizePieceVisuals (piece);
    return addPiece (std::move (piece));
}

DrumPiece& KitModel::addDefaultAccessory (float canvasWidth, float canvasHeight)
{
    auto piece = makeBasePiece (DrumPieceType::accessory, "Accessory", canvasWidth, canvasHeight, 70.0f, 70.0f);
    piece.color = nextDefaultColour();
    piece.addArticulation (makeArticulation ("Hit", piece.primaryMidiNote));
    return addPiece (std::move (piece));
}

bool KitModel::removePiece (const juce::String& id, bool sendChangeNotification)
{
    const auto it = std::find_if (pieces.begin(), pieces.end(),
                                  [&id] (const DrumPiece& p) { return p.id == id; });

    if (it == pieces.end())
        return false;

    pieces.erase (it);

    if (sendChangeNotification)
        notifyChanged();
    else
        ++changeGeneration;

    return true;
}

void KitModel::clear()
{
    pieces.clear();
    notifyChanged();
}

void KitModel::importContents (const KitModel& source)
{
    kitName = source.kitName;
    pieces = source.pieces;

    for (auto& piece : pieces)
    {
        normalizePieceVisuals (piece);
        ensureStandardArticulations (piece);
    }

    notifyChanged();
}

void KitModel::resolveSamplePaths (const juce::File& kitRoot)
{
    for (auto& piece : pieces)
    {
        for (auto& art : piece.articulations)
        {
            for (auto& layer : art.layers)
            {
                for (auto& sample : layer.roundRobins.samples)
                {
                    if (sample.filePath.isEmpty())
                        continue;

                    const auto normalized = sample.filePath.replaceCharacter ('\\', '/');
                    juce::File file (normalized);

                    if (normalized.startsWithChar ('/') && file.existsAsFile())
                    {
                        sample.filePath = file.getFullPathName();
                        continue;
                    }

                    if (file.existsAsFile())
                    {
                        sample.filePath = file.getFullPathName();
                        continue;
                    }

                    auto resolved = resolvePathFromRoot (kitRoot, normalized);

                    if (resolved.existsAsFile())
                    {
                        sample.filePath = resolved.getFullPathName();
                        continue;
                    }

                    resolved = kitRoot.getChildFile ("samples").getChildFile (juce::File (normalized).getFileName());

                    if (resolved.existsAsFile())
                    {
                        sample.filePath = resolved.getFullPathName();
                        continue;
                    }

                    resolved = kitRoot.getParentDirectory().getChildFile ("source").getChildFile (normalized);

                    if (resolved.existsAsFile())
                        sample.filePath = resolved.getFullPathName();
                }
            }
        }
    }
}

void KitModel::normalizeStandardArticulations()
{
    for (auto& piece : pieces)
        ensureStandardArticulations (piece);
}

void KitModel::createDefaultKit (float canvasWidth, float canvasHeight)
{
    pieces.clear();
    kitName = "Starter Kit";

    struct DefaultPiece
    {
        const char* name;
        DrumPieceType type;
        float xNorm;
        float yNorm;
        float w;
        float h;
        std::vector<std::pair<const char*, int>> arts;
        juce::String chokeGroupId;
    };

    static const DefaultPiece defaults[] =
    {
        { "Kick",       DrumPieceType::kick,     0.42f, 0.65f, 120.0f, 120.0f, { { "Center", 36 } }, {} },
        { "Snare",      DrumPieceType::snare,    0.42f, 0.42f,  84.0f,  84.0f, { { "Center", 38 }, { "Rimshot", 40 } }, {} },
        { "Rack Tom",   DrumPieceType::rackTom,  0.58f, 0.38f,  72.0f,  72.0f, { { "Hit", 48 } }, {} },
        { "Floor Tom",  DrumPieceType::floorTom, 0.62f, 0.58f,  96.0f,  96.0f, { { "Hit", 43 } }, {} },
        { "Hi-Hat",     DrumPieceType::hiHat,    0.28f, 0.30f,  68.0f,  68.0f, { { "Closed", 42 }, { "Open", 46 } }, "hat" },
        { "Crash",      DrumPieceType::crash,    0.18f, 0.18f, 104.0f, 104.0f, { { "Hit", 49 } }, {} },
        { "Ride",       DrumPieceType::ride,     0.72f, 0.22f, 110.0f, 110.0f, { { "Edge", 51 }, { "Bell", 53 } }, {} },
    };

    for (const auto& def : defaults)
    {
        DrumPiece piece;
        piece.id = DrumPiece::makeId();
        piece.name = def.name;
        piece.type = def.type;
        piece.shapeType = defaultShapeForType (def.type);
        piece.chokeGroupId = def.chokeGroupId;
        piece.width = def.w;
        piece.height = def.h;
        piece.x = def.xNorm * canvasWidth - def.w * 0.5f;
        piece.y = def.yNorm * canvasHeight - def.h * 0.5f;
        piece.color = isCymbalPieceType (def.type) ? cymbalFillColour() : drumShellFillColour();

        for (const auto& [artName, note] : def.arts)
        {
            auto art = makeArticulation (artName, note, def.chokeGroupId);
            piece.addArticulation (std::move (art));
        }

        if (! def.arts.empty())
            piece.primaryMidiNote = def.arts.front().second;

        normalizePieceVisuals (piece);
        pieces.push_back (std::move (piece));
    }

    notifyChanged();
}

int KitModel::suggestNextMidiNote() const
{
    for (const auto note : kPreferredNotes)
        if (! isMidiNoteInUse (note))
            return note;

    for (int note = 35; note <= 81; ++note)
        if (! isMidiNoteInUse (note))
            return note;

    return 36;
}

bool KitModel::isMidiNoteInUse (int note, const juce::String& ignorePieceId) const
{
    for (const auto& piece : pieces)
    {
        if (piece.id == ignorePieceId)
            continue;

        for (const auto& art : piece.articulations)
            if (art.midiNote == note)
                return true;
    }

    return false;
}

void KitModel::assignSingleSample (DrumPiece& piece, const juce::File& file, const juce::String& targetArticulationId)
{
    Articulation* art = nullptr;

    if (targetArticulationId.isNotEmpty())
        art = piece.findArticulationById (targetArticulationId);

    if (art == nullptr)
        art = piece.getPrimaryArticulation();

    if (art == nullptr)
    {
        Articulation newArt;
        newArt.id = Articulation::makeId();
        newArt.name = "Hit";
        newArt.midiNote = piece.primaryMidiNote;
        art = &piece.addArticulation (std::move (newArt));
    }

    art->layers.clear();

    SampleLayer layer;
    layer.id = SampleLayer::makeId();
    layer.minVelocity = 1;
    layer.maxVelocity = 127;

    DrumSample sample;
    sample.id = DrumSample::makeId();
    sample.filePath = file.getFullPathName();
    layer.roundRobins.addSample (std::move (sample));

    art->layers.push_back (std::move (layer));

    if (piece.type == DrumPieceType::ride)
        ensureStandardArticulations (piece);
    else
        piece.syncMidiNotesFromArticulations();

    notifyChanged();
}

void KitModel::addVelocityLayer (DrumPiece& piece, int minVelocity, int maxVelocity, const juce::File& file)
{
    auto* art = piece.getPrimaryArticulation();

    if (art == nullptr)
    {
        assignSingleSample (piece, file);
        return;
    }

    SampleLayer layer;
    layer.id = SampleLayer::makeId();
    layer.minVelocity = minVelocity;
    layer.maxVelocity = maxVelocity;
    layer.clampVelocityRange();

    DrumSample sample;
    sample.id = DrumSample::makeId();
    sample.filePath = file.getFullPathName();
    layer.roundRobins.addSample (std::move (sample));

    art->layers.push_back (std::move (layer));
    notifyChanged();
}

void KitModel::addRoundRobinSample (DrumPiece& piece, const juce::String& layerId, const juce::File& file)
{
    auto* art = piece.getPrimaryArticulation();

    if (art == nullptr)
    {
        assignSingleSample (piece, file);
        return;
    }

    SampleLayer* layer = layerId.isNotEmpty() ? art->findLayerById (layerId)
                                              : art->getOrCreateDefaultLayer();

    if (layer == nullptr)
        return;

    DrumSample sample;
    sample.id = DrumSample::makeId();
    sample.filePath = file.getFullPathName();
    layer->roundRobins.addSample (std::move (sample));
    notifyChanged();
}

void KitModel::addListener (std::function<void()> listener)
{
    listeners.push_back (std::move (listener));
}

void KitModel::notifyChanged()
{
    ++changeGeneration;

    for (const auto& listener : listeners)
        if (listener)
            listener();
}

void KitModel::recordLayoutEdit()
{
    ++changeGeneration;
}

juce::Colour KitModel::colourForType (DrumPieceType type) const
{
    if (isCymbalPieceType (type))
        return cymbalFillColour();

    if (type == DrumPieceType::accessory)
        return juce::Colour (0xff95a5a6);

    return drumShellFillColour();
}

juce::Colour KitModel::nextDefaultColour() const
{
    static const juce::Colour palette[] =
    {
        juce::Colour (0xff3498db),
        juce::Colour (0xff9b59b6),
        juce::Colour (0xffe67e22),
        juce::Colour (0xff1abc9c),
        juce::Colour (0xffe84393),
    };

    return palette[pieces.size() % (sizeof (palette) / sizeof (palette[0]))];
}
