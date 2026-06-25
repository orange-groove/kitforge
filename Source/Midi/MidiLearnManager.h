#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"

/** Assigns the next incoming MIDI note to a target piece articulation. */
class MidiLearnManager
{
public:
    void startLearning (const juce::String& pieceId, const juce::String& articulationId = {});
    void cancelLearning();

    bool isLearning() const { return learning; }
    juce::String getTargetPieceId() const { return targetPieceId; }
    juce::String getTargetArticulationId() const { return targetArticulationId; }

    /** Returns true if the note was consumed by learn mode. */
    bool processMidiNote (int midiNote, KitModel& model);

private:
    bool learning = false;
    juce::String targetPieceId;
    juce::String targetArticulationId;
};
