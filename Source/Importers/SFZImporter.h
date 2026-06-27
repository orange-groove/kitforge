#pragma once

#include <JuceHeader.h>
#include "ImportResult.h"
#include "../Models/SampleMetadata.h"

struct SFZImportOptions
{
    juce::String sfzFilePath;
    juce::String sampleRootPath;
    juce::String kitName;
};

/** Converts SFZ drum libraries into KitForge KitModel (partial drum-focused SFZ support). */
class SFZImporter
{
public:
    ImportResult importFile (const SFZImportOptions& options) const;

private:
    struct SFZRegion
    {
        int key = -1;
        int lokey = -1;
        int hikey = -1;
        int pitchKeyCenter = -1;
        int lovel = 1;
        int hivel = 127;
        int group = 0;
        int offBy = 0;
        int seqLength = 0;
        int seqPosition = 0;
        float loRand = 0.0f;
        float hiRand = 1.0f;
        juce::String samplePath;
        juce::String mappingSource;
    };

    std::vector<SFZRegion> parseRegions (const juce::String& sfzText) const;
    juce::String expandSfzDocument (const juce::File& sfzFile) const;
    void applyDefineSubstitutions (juce::String& line, const juce::HashMap<juce::String, juce::String>& defines) const;
    void applyOpcodeLine (const juce::String& line, SFZRegion& region) const;
    void applyOpcodeTokens (const juce::String& line, SFZRegion& region) const;
    static juce::String stripInlineComment (const juce::String& line);
    static juce::String stripSfzTagSuffix (const juce::String& line);
    SampleMetadata regionToMetadata (const SFZRegion& region,
                                     const SFZImportOptions& options,
                                     std::vector<ImportWarning>& warnings) const;
    static DrumPieceType inferTypeFromMidiNote (int midiNote);
    static int inferIndexFromMidiNote (int midiNote, DrumPieceType type);
    static juce::String inferArticulationFromMidiNote (int midiNote, DrumPieceType type);
    static juce::String inferArticulationFromSamplePath (const juce::String& samplePath);
};
