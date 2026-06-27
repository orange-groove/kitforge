#include "KitMigrationService.h"

KitMigrationResult KitMigrationService::validateManifest (const KitManifest& manifest)
{
    KitMigrationResult result;

    if (manifest.format != "kitforge")
    {
        result.errorMessage = "Invalid package format (expected \"kitforge\").";
        return result;
    }

    if (manifest.formatVersion > KitManifest::kSupportedFormatVersion)
    {
        result.unsupportedVersion = true;
        result.errorMessage = "Unsupported .kitforge format version "
                            + juce::String (manifest.formatVersion) + ".";
        return result;
    }

    if (manifest.formatVersion < 1)
    {
        result.errorMessage = "Invalid manifest formatVersion.";
        return result;
    }

    if (manifest.packageId.isEmpty() || manifest.name.isEmpty())
    {
        result.errorMessage = "Manifest missing packageId or name.";
        return result;
    }

    result.ok = true;
    return result;
}
