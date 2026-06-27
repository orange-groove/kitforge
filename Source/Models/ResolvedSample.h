#pragma once

#include <JuceHeader.h>
#include "SampleRef.h"

/** Result of resolving a `SampleRef` to a concrete file on disk. */
struct ResolvedSample
{
    SampleRef sampleRef;
    juce::String absoluteFilePath;
    bool exists = false;
    juce::String errorMessage;

    juce::var toVar() const
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("sampleRef", sampleRef.toVar());
        obj->setProperty ("absoluteFilePath", absoluteFilePath);
        obj->setProperty ("exists", exists);

        if (errorMessage.isNotEmpty())
            obj->setProperty ("errorMessage", errorMessage);

        return juce::var (obj);
    }
};
