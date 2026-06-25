#pragma once

#include "CatalogTypes.h"
#include <functional>

class KitInstaller;

class CatalogService
{
public:
    using ManifestCallback = std::function<void(bool success, const CatalogManifest& manifest)>;

    CatalogService();

    void setManifestUrl (const juce::String& url) { manifestUrl = url; }
    juce::String getManifestUrl() const { return manifestUrl; }

    void refreshManifest (ManifestCallback callback);

    const CatalogManifest& getCachedManifest() const { return cachedManifest; }
    bool hasCachedManifest() const { return cachedManifest.kits.size() > 0; }

    const CatalogKitEntry* findKitById (const juce::String& kitId) const;

    juce::Array<CatalogKitEntry> search (const juce::String& query) const;
    juce::Array<CatalogKitEntry> filterByTag (const juce::String& tag) const;

    bool loadCachedManifestFromDisk();
    bool saveCachedManifestToDisk() const;

    bool removeKit (const juce::String& kitId);
    bool isKitInstalled (const juce::String& kitId) const;
    juce::String getInstalledVersion (const juce::String& kitId) const;
    bool installedKitIsIncomplete (const juce::String& kitId) const;
    bool kitNeedsReinstall (const CatalogKitEntry& kit) const;

    void ensureDemoCatalogEntry();

private:
    juce::String manifestUrl { "https://catalog.kitforge.app/manifest.json" };
    CatalogManifest cachedManifest;

    void loadBundledMockManifest();
    void downloadManifestAsync (ManifestCallback callback);
};
