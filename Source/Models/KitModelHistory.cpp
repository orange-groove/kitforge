#include "KitModelHistory.h"
#include "KitModel.h"

KitModelHistory::Snapshot KitModelHistory::capture (const KitModel& model)
{
    Snapshot snap;
    snap.kitName = model.kitName;
    snap.pieces = model.getPieces();
    return snap;
}

void KitModelHistory::restore (KitModel& model, const Snapshot& snap)
{
    model.kitName = snap.kitName;
    model.getPiecesMutable() = snap.pieces;
}

void KitModelHistory::saveCheckpoint (const KitModel& model)
{
    if (suppressCheckpoints)
        return;

    undoStack.push_back (capture (model));

    if (undoStack.size() > kMaxDepth)
        undoStack.erase (undoStack.begin());

    redoStack.clear();
}

bool KitModelHistory::undo (KitModel& model)
{
    if (undoStack.empty())
        return false;

    suppressCheckpoints = true;
    redoStack.push_back (capture (model));
    restore (model, undoStack.back());
    undoStack.pop_back();
    suppressCheckpoints = false;
    return true;
}

bool KitModelHistory::redo (KitModel& model)
{
    if (redoStack.empty())
        return false;

    suppressCheckpoints = true;
    undoStack.push_back (capture (model));
    restore (model, redoStack.back());
    redoStack.pop_back();
    suppressCheckpoints = false;
    return true;
}

void KitModelHistory::clear()
{
    undoStack.clear();
    redoStack.clear();
}
