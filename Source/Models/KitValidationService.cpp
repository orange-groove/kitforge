#include "KitValidationService.h"
#include <set>

namespace
{
    bool licenseLooksRestrictive (const juce::String& license)
    {
        const auto l = license.trim().toLowerCase();

        if (l.isEmpty())
            return false;

        // Treat common permissive markers as safe; everything else is flagged.
        static const char* permissive[] = { "cc0", "public domain", "custom", "kitforge" };

        for (auto* p : permissive)
            if (l.contains (p))
                return false;

        return true;
    }
}

KitValidationReport KitValidationService::validateKit (const KitModel& kit, const SampleIndexService& index)
{
    KitValidationReport report;

    std::set<juce::String> seenSampleIds;
    int missingFiles = 0;
    int brokenRefs = 0;
    std::set<juce::String> missingLibraries;

    for (const auto& piece : kit.getPieces())
    {
        for (const auto& art : piece.articulations)
        {
            for (const auto& layer : art.layers)
            {
                for (const auto& sample : layer.roundRobins.samples)
                {
                    if (sample.id.isNotEmpty())
                    {
                        if (seenSampleIds.count (sample.id) > 0)
                            report.errors.add ("Duplicate sample id: " + sample.id);
                        else
                            seenSampleIds.insert (sample.id);
                    }

                    const auto& ref = sample.sampleRef;

                    if (ref.hasRef())
                    {
                        // Broken ref: has a library but no usable locator.
                        if (ref.libraryId.isNotEmpty()
                            && ref.relativePath.isEmpty() && ref.fallbackPath.isEmpty())
                        {
                            ++brokenRefs;
                            continue;
                        }

                        if (auto resolved = index.resolveSampleRef (ref))
                        {
                            if (! resolved->exists)
                            {
                                ++missingFiles;

                                if (ref.libraryId.isNotEmpty())
                                    missingLibraries.insert (ref.libraryId);
                            }
                        }
                    }
                    else if (! juce::File (sample.filePath).existsAsFile())
                    {
                        ++missingFiles;
                    }
                }
            }
        }
    }

    // Missing referenced libraries (those that no longer appear installed).
    for (const auto& libId : missingLibraries)
        if (! index.isLibraryInstalled (libId))
            report.warnings.add ("Referenced library not installed: " + libId);

    if (missingFiles > 0)
        report.warnings.add (juce::String (missingFiles) + " sample file(s) could not be found.");

    if (brokenRefs > 0)
        report.warnings.add (juce::String (brokenRefs) + " sample reference(s) are incomplete.");

    return report;
}

void KitValidationService::appendExportLicenseWarnings (const KitModel& kit,
                                                        const SampleIndexService& index,
                                                        bool selfContained,
                                                        KitValidationReport& report)
{
    if (! selfContained)
        return; // referenced exports don't copy audio, so no redistribution concern

    std::set<juce::String> flagged;

    for (const auto& piece : kit.getPieces())
        for (const auto& art : piece.articulations)
            for (const auto& layer : art.layers)
                for (const auto& sample : layer.roundRobins.samples)
                {
                    const auto& libId = sample.sampleRef.libraryId;

                    if (libId.isEmpty() || flagged.count (libId) > 0)
                        continue;

                    const auto license = index.getLibraryLicense (libId);

                    if (licenseLooksRestrictive (license))
                    {
                        const auto name = index.getLibraryName (libId);
                        report.warnings.add ("Exporting samples from \""
                                             + (name.isNotEmpty() ? name : libId)
                                             + "\" (license: " + license
                                             + "). Verify redistribution rights before sharing.");
                        flagged.insert (libId);
                    }
                }
}
