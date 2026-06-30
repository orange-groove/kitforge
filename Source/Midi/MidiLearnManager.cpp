#include "MidiLearnManager.h"

void MidiLearnManager::startLearning (const juce::String& pieceId, const juce::String& articulationId)
{
    targetPieceId = pieceId;
    targetArticulationId = articulationId;
    learning = true;
}

void MidiLearnManager::cancelLearning()
{
    learning = false;
    targetPieceId.clear();
    targetArticulationId.clear();
}

bool MidiLearnManager::processMidiNote (int midiNote, KitModel& model)
{
    if (! learning || targetPieceId.isEmpty())
        return false;

    auto* piece = model.findPieceById (targetPieceId);

    if (piece == nullptr)
    {
        cancelLearning();
        return false;
    }

    model.saveUndoCheckpoint();

    Articulation* targetArt = nullptr;

    if (targetArticulationId.isNotEmpty())
        targetArt = piece->findArticulationById (targetArticulationId);
    else
        targetArt = piece->getPrimaryArticulation();

    if (targetArt == nullptr)
    {
        Articulation art;
        art.id = Articulation::makeId();
        art.name = "Hit";
        art.midiNote = midiNote;
        targetArt = &piece->addArticulation (std::move (art));
    }

    // If another articulation already owns this note, swap notes to avoid collisions.
    if (auto existing = model.findArticulationByMidiNote (midiNote);
        existing.articulation != nullptr && existing.articulation != targetArt)
    {
        const int oldNote = targetArt->midiNote;
        existing.articulation->midiNote = oldNote;
    }

    targetArt->midiNote = midiNote;
    piece->primaryMidiNote = targetArt->midiNote;
    piece->syncMidiNotesFromArticulations();

    model.notifyChanged();
    cancelLearning();
    return true;
}
