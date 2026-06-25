#pragma once

#include "DrumPiece.h"
#include <functional>
#include <vector>

struct ArticulationRef
{
    DrumPiece* piece = nullptr;
    Articulation* articulation = nullptr;
};

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

    DrumPiece* findPieceById (const juce::String& id);
    const DrumPiece* findPieceById (const juce::String& id) const;

    ArticulationRef findArticulationByMidiNote (int midiNote);
    ArticulationRef findArticulationByMidiNote (int midiNote) const;
    ArticulationRef findArticulation (const juce::String& pieceId, const juce::String& articulationId);

    DrumPiece& addPiece (DrumPiece piece);
    DrumPiece& addDefaultDrum (DrumPieceType type, float canvasWidth, float canvasHeight);
    DrumPiece& addDefaultCymbal (DrumPieceType type, float canvasWidth, float canvasHeight);
    DrumPiece& addDefaultAccessory (float canvasWidth, float canvasHeight);

    bool removePiece (const juce::String& id, bool sendChangeNotification = true);
    void clear();
    void importContents (const KitModel& source);

    /** Bumped on any model edit including layout drags (for stale AI guard). */
    uint64_t getChangeGeneration() const { return changeGeneration; }
    void recordLayoutEdit();

    /** Resolves relative sample paths and fixes broken absolute paths against a kit root folder. */
    void resolveSamplePaths (const juce::File& kitRoot);

    void createDefaultKit (float canvasWidth, float canvasHeight);

    int suggestNextMidiNote() const;
    bool isMidiNoteInUse (int note, const juce::String& ignorePieceId = {}) const;

    // Sample assignment helpers
    void assignSingleSample (DrumPiece& piece, const juce::File& file, const juce::String& articulationName = {});
    void addVelocityLayer (DrumPiece& piece, int minVelocity, int maxVelocity, const juce::File& file);
    void addRoundRobinSample (DrumPiece& piece, const juce::String& layerId, const juce::File& file);

    void addListener (std::function<void()> listener);
    void notifyChanged();

private:
    std::vector<DrumPiece> pieces;
    std::vector<std::function<void()>> listeners;
    uint64_t changeGeneration = 0;

    juce::Colour colourForType (DrumPieceType type) const;
    juce::Colour nextDefaultColour() const;
    DrumPiece makeBasePiece (DrumPieceType type, const juce::String& name,
                             float canvasWidth, float canvasHeight,
                             float w, float h) const;
};
