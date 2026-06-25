#pragma once

#include <JuceHeader.h>

enum class ImportWarningType
{
    unknownSample,
    missingVelocityInfo,
    unsupportedFileType,
    duplicateMidiNote,
    couldNotInferInstrument,
    sampleFileMissing,
    general
};

inline juce::String importWarningTypeToString (ImportWarningType type)
{
    switch (type)
    {
        case ImportWarningType::unknownSample:           return "Unknown sample";
        case ImportWarningType::missingVelocityInfo:     return "Missing velocity info";
        case ImportWarningType::unsupportedFileType:     return "Unsupported file type";
        case ImportWarningType::duplicateMidiNote:       return "Duplicate MIDI note";
        case ImportWarningType::couldNotInferInstrument: return "Could not infer instrument";
        case ImportWarningType::sampleFileMissing:       return "Sample file missing";
        default:                                         return "Warning";
    }
}

struct ImportWarning
{
    ImportWarningType type = ImportWarningType::general;
    juce::String message;
    juce::String filePath;

    static ImportWarning make (ImportWarningType warningType, const juce::String& msg,
                               const juce::String& path = {})
    {
        ImportWarning w;
        w.type = warningType;
        w.message = msg;
        w.filePath = path;
        return w;
    }
};
