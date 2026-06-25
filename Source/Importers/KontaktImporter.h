#pragma once

#include "ImportResult.h"

struct KontaktImportOptions
{
    /** Skip "snares off" variants on shells (Kontakt bleed samples). */
    bool preferSnaresOnOnly = true;

    /** Merge close / closed / closed side hi-hat into one articulation. */
    bool consolidateHiHatClosed = true;

    /** Merge opened 1–5 into one Open articulation (velocity layers by degree). */
    bool consolidateHiHatOpen = true;

    /** Map ride-crash samples to Crash cymbal pieces instead of ride edge. */
    bool rideCrashAsCrashPiece = true;
};

/** Imports Kontakt drum libraries from folder layout + sample naming conventions. */
class KontaktImporter
{
public:
    ImportResult importLibraryFolder (const juce::File& libraryRoot,
                                      const KontaktImportOptions& options = {}) const;

private:
    static juce::File findSamplesFolder (const juce::File& libraryRoot);
    static juce::String inferKitName (const juce::File& libraryRoot);
    static KitModel buildKitFromSamples (const juce::String& kitName,
                                         const std::vector<struct KontaktParsedSample>& samples,
                                         std::vector<ImportWarning>& warnings);
    static void fixPrimaryMidiNotes (KitModel& model);
};
