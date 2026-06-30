#pragma once

#include "DrumPiece.h"
#include <vector>

class KitModel;

/** Undo/redo stack of full kit snapshots (pieces + kit name). */
class KitModelHistory
{
public:
    struct Snapshot
    {
        juce::String kitName;
        std::vector<DrumPiece> pieces;
    };

    void saveCheckpoint (const KitModel& model);
    bool undo (KitModel& model);
    bool redo (KitModel& model);

    bool canUndo() const { return ! undoStack.empty(); }
    bool canRedo() const { return ! redoStack.empty(); }

    void clear();

    void setSuppressCheckpoints (bool suppress) { suppressCheckpoints = suppress; }

private:
    static Snapshot capture (const KitModel& model);
    static void restore (KitModel& model, const Snapshot& snap);

    std::vector<Snapshot> undoStack;
    std::vector<Snapshot> redoStack;
    bool suppressCheckpoints = false;

    static constexpr size_t kMaxDepth = 50;
};
