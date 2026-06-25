#include "KitRecipe.h"

namespace
{
    DrumPieceType typeFromRecipeString (const juce::String& s)
    {
        if (s == "kick")      return DrumPieceType::kick;
        if (s == "snare")     return DrumPieceType::snare;
        if (s == "rackTom")   return DrumPieceType::rackTom;
        if (s == "floorTom")  return DrumPieceType::floorTom;
        if (s == "hiHat")     return DrumPieceType::hiHat;
        if (s == "crash")     return DrumPieceType::crash;
        if (s == "ride")      return DrumPieceType::ride;
        if (s == "china")     return DrumPieceType::china;
        if (s == "splash")    return DrumPieceType::splash;
        return DrumPieceType::accessory;
    }

    KitRecipeArticulation articulationFromVar (const juce::var& v)
    {
        KitRecipeArticulation art;

        if (auto* obj = v.getDynamicObject())
        {
            art.name = obj->getProperty ("name").toString();
            art.midiNote = (int) obj->getProperty ("midiNote");
            art.chokeGroupId = obj->getProperty ("chokeGroupId").toString();
        }

        return art;
    }
}

KitRecipe KitRecipe::fromVar (const juce::var& v)
{
    KitRecipe recipe;

    if (auto* root = v.getDynamicObject())
    {
        recipe.name = root->getProperty ("name").toString();
        recipe.description = root->getProperty ("description").toString();
        recipe.mapProfile = root->getProperty ("mapProfile").toString();

        if (auto* tags = root->getProperty ("tags").getArray())
            for (const auto& tag : *tags)
                recipe.globalTags.add (tag.toString());

        if (auto* arr = root->getProperty ("pieces").getArray())
        {
            for (const auto& item : *arr)
            {
                if (auto* obj = item.getDynamicObject())
                {
                    KitRecipePiece piece;
                    piece.id = obj->getProperty ("id").toString();
                    piece.type = typeFromRecipeString (obj->getProperty ("type").toString());
                    piece.name = obj->getProperty ("name").toString();
                    piece.articulation = obj->getProperty ("articulation").toString();
                    piece.preferredMidiNote = (int) obj->getProperty ("preferredMidiNote");
                    piece.chokeGroupId = obj->getProperty ("chokeGroupId").toString();
                    piece.layoutXNorm = (float) (double) obj->getProperty ("layoutXNorm");
                    piece.layoutYNorm = (float) (double) obj->getProperty ("layoutYNorm");

                    if (auto* pieceTags = obj->getProperty ("tags").getArray())
                        for (const auto& tag : *pieceTags)
                            piece.tags.add (tag.toString());

                    if (auto* arts = obj->getProperty ("articulations").getArray())
                    {
                        for (const auto& artVar : *arts)
                            piece.articulations.push_back (articulationFromVar (artVar));
                    }

                    if (! piece.articulations.empty())
                    {
                        piece.articulation = piece.articulations.front().name;
                        piece.preferredMidiNote = piece.articulations.front().midiNote;
                    }

                    recipe.pieces.push_back (std::move (piece));
                }
            }
        }
    }

    return recipe;
}

juce::var KitRecipe::toVar() const
{
    juce::Array<juce::var> pieceVars;

    for (const auto& piece : pieces)
    {
        juce::Array<juce::var> tagVars;

        for (const auto& tag : piece.tags)
            tagVars.add (tag);

        juce::Array<juce::var> artVars;

        for (const auto& art : piece.articulations)
        {
            auto* artObj = new juce::DynamicObject();
            artObj->setProperty ("name", art.name);
            artObj->setProperty ("midiNote", art.midiNote);
            artObj->setProperty ("chokeGroupId", art.chokeGroupId);
            artVars.add (juce::var (artObj));
        }

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("id", piece.id);
        obj->setProperty ("type", drumPieceTypeToString (piece.type));
        obj->setProperty ("name", piece.name);
        obj->setProperty ("articulation", piece.articulation);
        obj->setProperty ("preferredMidiNote", piece.preferredMidiNote);
        obj->setProperty ("chokeGroupId", piece.chokeGroupId);
        obj->setProperty ("layoutXNorm", piece.layoutXNorm);
        obj->setProperty ("layoutYNorm", piece.layoutYNorm);
        obj->setProperty ("tags", tagVars);
        obj->setProperty ("articulations", artVars);
        pieceVars.add (juce::var (obj));
    }

    juce::Array<juce::var> globalTagVars;

    for (const auto& tag : globalTags)
        globalTagVars.add (tag);

    auto* root = new juce::DynamicObject();
    root->setProperty ("name", name);
    root->setProperty ("description", description);
    root->setProperty ("mapProfile", mapProfile);
    root->setProperty ("tags", globalTagVars);
    root->setProperty ("pieces", pieceVars);
    return juce::var (root);
}
