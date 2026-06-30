#pragma once

#include "DrumPiece.h"
#include "KitModelHistory.h"
#include <functional>
#include <vector>

struct ArticulationRef
{
    DrumPiece* piece = nullptr;
    Articulation* articulation = nullptr;
};

/** Ensures standard articulations exist (e.g. ride Edge + Bell). */
void ensureStandardArticulations (DrumPiece& piece);

class KitModel
{
public:
    struct EmptyInit {};

    KitModel();
    explicit KitModel (EmptyInit);

    /** Model with no pieces — use for import/deserialize paths. */
    static KitModel createEmpty() { return KitModel (EmptyInit {}); }

    juce::String kitName { "Default Kit" };

    const std::vector<DrumPiece>& getPieces() const { return pieces; }

    /** Mutable access for in-place edits (e.g. sample-ref resolution, swaps). */
    std::vector<DrumPiece>& getPiecesMutable() { return pieces; }

    DrumPiece* findPieceById (const juce::String& id);
    const DrumPiece* findPieceById (const juce::String& id) const;

    ArticulationRef findArticulationByMidiNote (int midiNote);
    ArticulationRef findArticulationByMidiNote (int midiNote) const;
    ArticulationRef findArticulation (const juce::String& pieceId, const juce::String& articulationId);

    DrumPiece& addPiece (DrumPiece piece);
    DrumPiece& addDefaultDrum (DrumPieceType type, float canvasWidth, float canvasHeight);
    DrumPiece& addDefaultCymbal (DrumPieceType type, float canvasWidth, float canvasHeight);
    DrumPiece& addDefaultAccessory (float canvasWidth, float canvasHeight);

    /** Adds a drum or cymbal; `diameterInches` sets width/height on the layout canvas. */
    DrumPiece& addPieceWithDiameter (DrumPieceType type, float diameterInches,
                                     float canvasWidth, float canvasHeight);

    bool removePiece (const juce::String& id, bool sendChangeNotification = true);

    /** Reorders a piece within the draw/stack order. Later pieces render on top.
        mode: "front" (top), "back" (bottom), "forward" (+1), "backward" (-1).
        Returns true if the order changed. */
    bool reorderPiece (const juce::String& id, const juce::String& mode);

    void clear();
    void importContents (const KitModel& source);

    /** Bumped on any model edit including layout drags (for stale AI guard). */
    uint64_t getChangeGeneration() const { return changeGeneration; }
    void recordLayoutEdit();

    /** Resolves relative sample paths and fixes broken absolute paths against a kit root folder. */
    void resolveSamplePaths (const juce::File& kitRoot);

    /** Collapses ride pieces to Edge + Bell articulations (safe to call repeatedly). */
    void normalizeStandardArticulations();

    void createDefaultKit (float canvasWidth, float canvasHeight);

    int suggestNextMidiNote() const;
    bool isMidiNoteInUse (int note, const juce::String& ignorePieceId = {}) const;

    // Sample assignment helpers
    void assignSingleSample (DrumPiece& piece, const juce::File& file, const juce::String& targetArticulationId = {});
    void addVelocityLayer (DrumPiece& piece, int minVelocity, int maxVelocity, const juce::File& file);
    void addRoundRobinSample (DrumPiece& piece, const juce::String& layerId, const juce::File& file);

    void addListener (std::function<void()> listener);
    void notifyChanged();

    /** Saves the current kit on the undo stack (call before mutating in-place). */
    void saveUndoCheckpoint();

    bool undo();
    bool redo();
    bool canUndo() const { return history.canUndo(); }
    bool canRedo() const { return history.canRedo(); }
    void clearUndoHistory();

    /** One undo step per layout drag/resize gesture. */
    void beginLayoutEdit();
    void commitLayoutEdit();

private:
    std::vector<DrumPiece> pieces;
    std::vector<std::function<void()>> listeners;
    uint64_t changeGeneration = 0;
    KitModelHistory history;
    bool layoutEditCheckpointSaved = false;

    void assignSingleSampleInternal (DrumPiece& piece, const juce::File& file,
                                     const juce::String& targetArticulationId);

    juce::Colour colourForType (DrumPieceType type) const;
    juce::Colour nextDefaultColour() const;
    DrumPiece makeBasePiece (DrumPieceType type, const juce::String& name,
                             float canvasWidth, float canvasHeight,
                             float w, float h) const;
    void setupDefaultArticulations (DrumPiece& piece, DrumPieceType type);
    void sortPiecesByDisplayLayer();
};
