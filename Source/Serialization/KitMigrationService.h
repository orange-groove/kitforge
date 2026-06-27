#pragma once

#include <JuceHeader.h>
#include "../Models/KitManifest.h"

struct KitMigrationResult
{
    bool ok = false;
    bool unsupportedVersion = false;
    juce::String errorMessage;
};

/** Validates package format version and reports migration needs. */
class KitMigrationService
{
public:
    static KitMigrationResult validateManifest (const KitManifest& manifest);
};
