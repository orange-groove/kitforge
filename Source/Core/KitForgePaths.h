#pragma once

#include <JuceHeader.h>

/** Central path helpers for KitForge library, catalog cache, and pack storage. */
namespace KitForgePaths
{
    inline juce::File getKitForgeRoot()
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KitForge");
    }

    inline juce::File getLibrariesRoot()
    {
        return getKitForgeRoot().getChildFile ("Libraries");
    }

    inline juce::File getCatalogCacheRoot()
    {
        return getKitForgeRoot().getChildFile ("Catalog");
    }

    inline juce::File getManifestCacheFile()
    {
        return getCatalogCacheRoot().getChildFile ("manifest.json");
    }

    inline juce::File getDownloadsCacheRoot()
    {
        return getCatalogCacheRoot().getChildFile ("downloads");
    }

    inline juce::File getPackCacheRoot()
    {
        return getCatalogCacheRoot().getChildFile ("packs");
    }

    inline juce::File getInstalledKitsRoot()
    {
        return getKitForgeRoot().getChildFile ("InstalledKits");
    }

    inline juce::File getSampleIndexCacheFile()
    {
        return getKitForgeRoot().getChildFile ("sample-index.json");
    }

    inline juce::File getLibraryInstallPath (const juce::String& kitId)
    {
        return getLibrariesRoot().getChildFile (kitId);
    }

    inline juce::File getDownloadArchivePath (const juce::String& kitId, const juce::String& extension)
    {
        return getDownloadsCacheRoot().getChildFile (kitId + extension);
    }

    inline void ensureDirectoryStructure()
    {
        getLibrariesRoot().createDirectory();
        getCatalogCacheRoot().createDirectory();
        getDownloadsCacheRoot().createDirectory();
        getPackCacheRoot().createDirectory();
        getInstalledKitsRoot().createDirectory();
    }
}
