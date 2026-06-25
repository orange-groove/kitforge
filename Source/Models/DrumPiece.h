#pragma once

#include "../Engine/Articulation.h"
#include "DrumPieceTypes.h"

/** Visual and sonic drum/cymbal piece with one or more articulations. */
struct DrumPiece
{
    juce::String id;
    juce::String name;
    DrumPieceType type = DrumPieceType::accessory;

    juce::Array<int> midiNotes;
    int primaryMidiNote = 36;

    juce::String chokeGroupId;
    int outputChannelPair = 0; // placeholder for future multi-out routing

    float volume = 1.0f;
    float pan = 0.0f;
    float pitch = 1.0f;
    bool muted = false;
    bool soloed = false;

    float x = 0.0f;
    float y = 0.0f;
    float width = 80.0f;
    float height = 80.0f;
    float rotation = 0.0f;
    juce::Colour color { 0xff4a90d9 };
    ShapeType shapeType = ShapeType::circle;

    std::vector<Articulation> articulations;

    static juce::String makeId() { return juce::Uuid().toString(); }

    bool isCymbal() const { return isCymbalPieceType (type) || shapeType == ShapeType::cymbal; }

    juce::Rectangle<float> getBounds() const { return { x, y, width, height }; }

    void setBounds (juce::Rectangle<float> bounds)
    {
        x = bounds.getX();
        y = bounds.getY();
        width = bounds.getWidth();
        height = bounds.getHeight();
    }

    Articulation* findArticulationById (const juce::String& articulationId)
    {
        for (auto& art : articulations)
            if (art.id == articulationId)
                return &art;

        return nullptr;
    }

    const Articulation* findArticulationById (const juce::String& articulationId) const
    {
        return const_cast<DrumPiece*> (this)->findArticulationById (articulationId);
    }

    Articulation* findArticulationByMidiNote (int midiNote)
    {
        for (auto& art : articulations)
            if (art.midiNote == midiNote)
                return &art;

        return nullptr;
    }

    const Articulation* findArticulationByMidiNote (int midiNote) const
    {
        return const_cast<DrumPiece*> (this)->findArticulationByMidiNote (midiNote);
    }

    Articulation* getPrimaryArticulation()
    {
        if (articulations.empty())
            return nullptr;

        for (auto& art : articulations)
            if (art.midiNote == primaryMidiNote)
                return &art;

        return &articulations.front();
    }

    const Articulation* getPrimaryArticulation() const
    {
        return const_cast<DrumPiece*> (this)->getPrimaryArticulation();
    }

    Articulation& addArticulation (Articulation art)
    {
        if (art.id.isEmpty())
            art.id = Articulation::makeId();

        articulations.push_back (std::move (art));
        syncMidiNotesFromArticulations();
        return articulations.back();
    }

    void syncMidiNotesFromArticulations()
    {
        midiNotes.clear();

        for (const auto& art : articulations)
            if (! midiNotes.contains (art.midiNote))
                midiNotes.add (art.midiNote);

        if (midiNotes.isEmpty())
            midiNotes.add (primaryMidiNote);
        else if (! midiNotes.contains (primaryMidiNote))
            primaryMidiNote = midiNotes[0];
    }

    juce::String effectiveChokeGroupId (const Articulation& art) const
    {
        if (art.chokeGroupId.isNotEmpty())
            return art.chokeGroupId;

        return chokeGroupId;
    }
};

inline void normalizePieceVisuals (DrumPiece& piece)
{
    piece.shapeType = effectiveShapeForPiece (piece.type, piece.shapeType);

    if (isCymbalPieceType (piece.type))
        piece.color = cymbalFillColour();
    else if (piece.type != DrumPieceType::accessory)
        piece.color = drumShellFillColour();
}
