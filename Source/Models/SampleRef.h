#pragma once

#include <JuceHeader.h>

/**
    Cross-library reference to a single audio sample.

    A `DrumSample` may carry a `SampleRef` so the same kit can pull samples from
    multiple installed libraries/packs. Resolution is performed off the audio
    thread (see `SampleIndexService::resolveSampleRef`) and the resolved absolute
    path is written into `DrumSample::filePath` for the engine to load.
*/
struct SampleRef
{
    /** Source kinds for a referenced sample. */
    struct SourceType
    {
        static constexpr const char* installedLibrary = "installedLibrary";
        static constexpr const char* embedded         = "embedded";
        static constexpr const char* absoluteFile     = "absoluteFile";
    };

    juce::String libraryId;     // installed pack/library id
    juce::String sampleSetId;   // compatible group (e.g. "Kick Center")
    juce::String sampleId;      // exact WAV id
    juce::String relativePath;  // path inside the installed library / package
    juce::String fallbackPath;  // optional legacy/absolute path
    juce::String sourceType { SourceType::installedLibrary };

    bool isEmpty() const
    {
        return libraryId.isEmpty() && sampleSetId.isEmpty() && sampleId.isEmpty()
            && relativePath.isEmpty() && fallbackPath.isEmpty();
    }

    bool hasRef() const { return ! isEmpty(); }

    juce::var toVar() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("libraryId", libraryId);
        obj->setProperty ("sampleSetId", sampleSetId);
        obj->setProperty ("sampleId", sampleId);
        obj->setProperty ("relativePath", relativePath);

        if (fallbackPath.isNotEmpty())
            obj->setProperty ("fallbackPath", fallbackPath);

        obj->setProperty ("sourceType", sourceType.isNotEmpty() ? sourceType
                                                                 : juce::String (SourceType::installedLibrary));
        return juce::var (obj);
    }

    static SampleRef fromVar (const juce::var& v)
    {
        SampleRef ref;

        if (auto* obj = v.getDynamicObject())
        {
            ref.libraryId    = obj->getProperty ("libraryId").toString();
            ref.sampleSetId  = obj->getProperty ("sampleSetId").toString();
            ref.sampleId     = obj->getProperty ("sampleId").toString();
            ref.relativePath = obj->getProperty ("relativePath").toString();
            ref.fallbackPath = obj->getProperty ("fallbackPath").toString();

            const auto st = obj->getProperty ("sourceType").toString();
            ref.sourceType = st.isNotEmpty() ? st : juce::String (SourceType::installedLibrary);
        }

        return ref;
    }
};
