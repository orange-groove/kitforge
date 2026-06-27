#pragma once

#include "DrumLibraryCatalogManifest.h"
#include "../Models/InstalledLibrary.h"
#include <functional>

/** Loads and caches the online SFZ drum library catalog manifest. */
class DrumLibraryCatalogService
{
public:
    using ManifestCallback = std::function<void(bool success, const DrumLibraryCatalogManifest& manifest)>;

    DrumLibraryCatalogService();

    void setRemoteManifestUrl (const juce::String& url) { remoteManifestUrl = url; }
    juce::String getRemoteManifestUrl() const { return remoteManifestUrl; }

    void refreshManifest (ManifestCallback callback);

    const DrumLibraryCatalogManifest& getCachedManifest() const { return cachedManifest; }
    bool hasCachedManifest() const { return cachedManifest.libraries.size() > 0; }

    const OnlineDrumLibrary* findLibraryById (const juce::String& libraryId) const;

    juce::Array<OnlineDrumLibrary> search (const juce::String& query) const;
    juce::Array<OnlineDrumLibrary> filterByTag (const juce::String& tag) const;

    bool loadCachedManifestFromDisk();
    bool saveCachedManifestToDisk() const;

    bool isLibraryInstalled (const juce::String& libraryId) const;
    bool isPartialInstall (const juce::String& libraryId) const;
    InstalledLibrary getInstalledLibrary (const juce::String& libraryId) const;
    bool removeInstalledLibrary (const juce::String& libraryId);

    /** Resolve download URL from catalog entry. */
    juce::String resolveDownloadUrl (const OnlineDrumLibrary& library) const;

private:
    juce::String remoteManifestUrl;
    DrumLibraryCatalogManifest cachedManifest;

    void loadBundledManifest();
    void downloadManifestAsync (ManifestCallback callback);
    void removeRetiredLibraries();
};
