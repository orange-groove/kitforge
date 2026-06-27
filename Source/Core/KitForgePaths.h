#pragma once

#include <JuceHeader.h>

/** Central path helpers for KitForge kits, imports, and caches. */
namespace KitForgePaths
{
    inline juce::File getKitForgeRoot()
    {
        // Use ~/Music (not iCloud-synced). ~/Documents is part of the macOS
        // "Desktop & Documents" iCloud feature, which relocates/duplicates large
        // sample folders mid-import (producing "samples 2" and missing samples).
        return juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("KitForge");
    }

    /** Previous install root under ~/Documents (migrated away from due to iCloud sync). */
    inline juce::File getLegacyKitForgeRoot()
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KitForge");
    }

    /** Installed native `.kitforge` kit packages. */
    inline juce::File getKitsRoot()
    {
        return getKitForgeRoot().getChildFile ("Kits");
    }

    inline juce::File getKitInstallPath (const juce::String& packageId)
    {
        return getKitsRoot().getChildFile (packageId);
    }

    /** Temporary extraction folder for imports. */
    inline juce::File getImportStagingRoot()
    {
        return getKitForgeRoot().getChildFile ("ImportStaging");
    }

    inline juce::File getSampleIndexCacheFile()
    {
        return getKitForgeRoot().getChildFile ("sample-index.json");
    }

    /** Cached `.kitforge` packages (e.g. bundled demo kit). */
    inline juce::File getPackCacheRoot()
    {
        return getKitForgeRoot().getChildFile ("Cache").getChildFile ("packs");
    }

    /** @deprecated Use getKitsRoot(). */
    inline juce::File getLibrariesRoot()
    {
        return getKitsRoot();
    }

    /** @deprecated Use getKitInstallPath(). */
    inline juce::File getLibraryInstallPath (const juce::String& packageId)
    {
        return getKitInstallPath (packageId);
    }

    /** One-time move of the legacy ~/Documents/KitForge tree to the new ~/Music root. */
    inline void migrateLegacyRootIfNeeded()
    {
        const auto legacy = getLegacyKitForgeRoot();
        const auto current = getKitForgeRoot();

        if (legacy == current || ! legacy.isDirectory() || current.isDirectory())
            return;

        current.getParentDirectory().createDirectory();

        if (! legacy.moveFileTo (current))
        {
            // Move failed (e.g. cross-volume); leave legacy in place and start fresh.
            current.createDirectory();
        }
    }

    inline void ensureDirectoryStructure()
    {
        migrateLegacyRootIfNeeded();
        getKitsRoot().createDirectory();
        getImportStagingRoot().createDirectory();
        getPackCacheRoot().createDirectory();
    }
}
