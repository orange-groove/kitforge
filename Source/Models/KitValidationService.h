#pragma once

#include <JuceHeader.h>
#include "KitModel.h"
#include "SampleIndexService.h"

struct KitValidationReport
{
    juce::StringArray errors;
    juce::StringArray warnings;

    bool ok() const { return errors.isEmpty(); }
    bool hasIssues() const { return ! errors.isEmpty() || ! warnings.isEmpty(); }

    juce::var toVar() const
    {
        juce::Array<juce::var> errorVars, warningVars;

        for (const auto& e : errors)   errorVars.add (e);
        for (const auto& w : warnings) warningVars.add (w);

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("errors", errorVars);
        obj->setProperty ("warnings", warningVars);
        return juce::var (obj);
    }
};

/** Integrity checks for a KitModel and its (possibly cross-library) sample refs. */
class KitValidationService
{
public:
    static KitValidationReport validateKit (const KitModel& kit, const SampleIndexService& index);

    /** Adds license warnings for samples borrowed from libraries with restrictive terms. */
    static void appendExportLicenseWarnings (const KitModel& kit,
                                             const SampleIndexService& index,
                                             bool selfContained,
                                             KitValidationReport& report);
};
