#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"
#include "../Models/KitManifest.h"
#include "../Models/InstalledLibrary.h"

struct KitPackageValidation
{
    bool valid = false;
    juce::StringArray errors;
    juce::StringArray warnings;
};

/** Reads, validates, and installs native `.kitforge` packages. */
class KitForgePackageReader
{
public:
    struct ReadResult
    {
        bool success = false;
        juce::String errorMessage;
        KitModel kit;
        KitManifest manifest;
        juce::File packageRoot;
        KitPackageValidation validation;
    };

    struct InstallResult
    {
        bool success = false;
        juce::String errorMessage;
        InstalledLibrary installed;
        juce::File installPath;
    };

    ReadResult readFolder (const juce::File& packageFolder) const;
    ReadResult readPackageFile (const juce::File& packageFile) const;

    KitPackageValidation validateFolder (const juce::File& packageFolder) const;

    /** Extract/install package into ~/Documents/KitForge/Kits/<packageId>/ */
    InstallResult installPackageFile (const juce::File& packageFile) const;

    /** Register an on-disk package folder (already at its install location). */
    InstallResult installPackageFolder (const juce::File& packageFolder) const;

    /** Load kit from an already-installed kit folder. */
    static ReadResult loadInstalledKit (const juce::File& installFolder);
};
