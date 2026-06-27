#pragma once

#include <JuceHeader.h>
#include "../Models/SampleMetadata.h"

/** Parses drum sample filenames and folder paths into SampleMetadata. */
class SampleNameParser
{
public:
    SampleMetadata parseFile (const juce::File& file) const;

private:
    struct InstrumentMatch
    {
        DrumPieceType type = DrumPieceType::accessory;
        int index = 0;
        juce::String articulation;
        int midiNote = 0;
        juce::String chokeGroupId;
        float confidence = 0.0f;
    };

    InstrumentMatch matchInstrumentToken (const juce::String& token) const;
    InstrumentMatch matchCompoundInstrumentToken (const juce::String& token) const;
    InstrumentMatch matchArticulationToken (const juce::String& token, DrumPieceType type) const;
    InstrumentMatch inferFromFolderName (const juce::String& folderName) const;
    void applyVelocityToken (const juce::String& token, SampleMetadata& meta) const;
    bool applyRoundRobinToken (const juce::String& token, SampleMetadata& meta) const;
    bool isVelocityToken (const juce::String& token) const;
    bool isRoundRobinToken (const juce::String& token) const;
    juce::String defaultArticulationForType (DrumPieceType type) const;
    int defaultMidiForTypeAndArticulation (DrumPieceType type, int index,
                                           const juce::String& articulation) const;
    void finalizeMetadata (SampleMetadata& meta) const;
};
