#include "DrumLibraryCatalogService.h"
#include "../Core/KitForgePaths.h"

#if __has_include ("KitForgeResources.h")
 #include "KitForgeResources.h"
 #define KITFORGE_HAS_BUNDLED_CATALOG 1
#else
 #define KITFORGE_HAS_BUNDLED_CATALOG 0
#endif

namespace
{
    juce::File getDrumCatalogCacheFile()
    {
        return KitForgePaths::getCatalogCacheRoot().getChildFile ("drum-catalog-manifest.json");
    }

    juce::String knownDownloadUrlForLibrary (const juce::String& libraryId)
    {
        struct KnownUrl { const char* id; const char* url; };
        static constexpr KnownUrl kKnown[] =
        {
            { "salamander-drumkit", "https://archive.org/download/SalamanderDrumkit/salamanderDrumkit.tar.bz2" },
        };

        for (const auto& entry : kKnown)
            if (libraryId == entry.id)
                return entry.url;

        return {};
    }
}

DrumLibraryCatalogService::DrumLibraryCatalogService()
{
    remoteManifestUrl = "https://catalog.kitforge.app/drum-libraries/manifest.json";

    if (! loadCachedManifestFromDisk())
        loadBundledManifest();

    removeRetiredLibraries();
}

const OnlineDrumLibrary* DrumLibraryCatalogService::findLibraryById (const juce::String& libraryId) const
{
    for (const auto& lib : cachedManifest.libraries)
        if (lib.id == libraryId)
            return &lib;

    return nullptr;
}

void DrumLibraryCatalogService::refreshManifest (ManifestCallback callback)
{
    downloadManifestAsync ([this, callback] (bool success, const DrumLibraryCatalogManifest& manifest)
    {
        if (success)
        {
            cachedManifest = manifest;
            removeRetiredLibraries();
            saveCachedManifestToDisk();
        }
        else
        {
            if (! loadCachedManifestFromDisk())
                loadBundledManifest();

            removeRetiredLibraries();
        }

        if (callback)
            callback (success || hasCachedManifest(), cachedManifest);
    });
}

void DrumLibraryCatalogService::loadBundledManifest()
{
#if KITFORGE_HAS_BUNDLED_CATALOG
    const juce::String json (KitForgeResources::catalog_manifest_json,
                             (size_t) KitForgeResources::catalog_manifest_jsonSize);
    juce::var parsed;

        if (juce::JSON::parse (json, parsed).wasOk())
        {
            cachedManifest = DrumLibraryCatalogManifest::fromVar (parsed);
            removeRetiredLibraries();
            return;
        }
#endif

    const auto file = juce::File::getCurrentWorkingDirectory()
                          .getChildFile ("Resources")
                          .getChildFile ("catalog_manifest.json");

    if (file.existsAsFile())
    {
        juce::var parsed;

        if (juce::JSON::parse (file.loadFileAsString(), parsed).wasOk())
        {
            cachedManifest = DrumLibraryCatalogManifest::fromVar (parsed);
            removeRetiredLibraries();
            return;
        }
    }

    cachedManifest = DrumLibraryCatalogManifest();
    cachedManifest.version = 1;
}

void DrumLibraryCatalogService::downloadManifestAsync (ManifestCallback callback)
{
    if (remoteManifestUrl.startsWithIgnoreCase ("file:"))
    {
        const auto file = juce::URL (remoteManifestUrl).getLocalFile();
        juce::var parsed;

        if (file.existsAsFile() && juce::JSON::parse (file.loadFileAsString(), parsed).wasOk())
        {
            if (callback)
                callback (true, DrumLibraryCatalogManifest::fromVar (parsed));

            return;
        }
    }

    juce::Thread::launch ([this, callback]
    {
        juce::URL url (remoteManifestUrl);
        const auto tempFile = getDrumCatalogCacheFile().getSiblingFile ("drum-catalog-download.json");
        bool success = false;
        DrumLibraryCatalogManifest manifest;

        if (auto task = url.downloadToFile (tempFile, juce::URL::DownloadTaskOptions {}))
        {
            while (! task->isFinished())
                juce::Thread::sleep (50);

            if (! task->hadError() && tempFile.existsAsFile())
            {
                juce::var parsed;

                if (juce::JSON::parse (tempFile.loadFileAsString(), parsed).wasOk())
                {
                    manifest = DrumLibraryCatalogManifest::fromVar (parsed);
                    success = manifest.libraries.size() > 0;
                }
            }
        }

        juce::MessageManager::callAsync ([callback, success, manifest]
        {
            if (callback)
                callback (success, manifest);
        });
    });
}

void DrumLibraryCatalogService::removeRetiredLibraries()
{
    for (int i = cachedManifest.libraries.size(); --i >= 0;)
    {
        if (cachedManifest.libraries.getReference (i).id == "demo-rock-kit")
            cachedManifest.libraries.remove (i);
    }
}

juce::String DrumLibraryCatalogService::resolveDownloadUrl (const OnlineDrumLibrary& library) const
{
    if (const auto known = knownDownloadUrlForLibrary (library.id); known.isNotEmpty())
        return known;

    return library.downloadUrl;
}

juce::Array<OnlineDrumLibrary> DrumLibraryCatalogService::search (const juce::String& query) const
{
    juce::Array<OnlineDrumLibrary> results;
    const auto q = query.trim().toLowerCase();

    if (q.isEmpty())
    {
        for (const auto& lib : cachedManifest.libraries)
            results.add (lib);

        return results;
    }

    for (const auto& lib : cachedManifest.libraries)
    {
        if (lib.name.toLowerCase().contains (q)
            || lib.id.toLowerCase().contains (q)
            || lib.description.toLowerCase().contains (q))
        {
            results.add (lib);
        }
    }

    return results;
}

juce::Array<OnlineDrumLibrary> DrumLibraryCatalogService::filterByTag (const juce::String& tag) const
{
    juce::Array<OnlineDrumLibrary> results;

    for (const auto& lib : cachedManifest.libraries)
        if (lib.tags.contains (tag, true))
            results.add (lib);

    return results;
}

bool DrumLibraryCatalogService::loadCachedManifestFromDisk()
{
    const auto file = getDrumCatalogCacheFile();

    if (! file.existsAsFile())
        return false;

    juce::var parsed;

    if (juce::JSON::parse (file.loadFileAsString(), parsed).failed())
        return false;

    cachedManifest = DrumLibraryCatalogManifest::fromVar (parsed);
    removeRetiredLibraries();
    return cachedManifest.libraries.size() > 0;
}

bool DrumLibraryCatalogService::saveCachedManifestToDisk() const
{
    KitForgePaths::ensureDirectoryStructure();
    return getDrumCatalogCacheFile().replaceWithText (juce::JSON::toString (cachedManifest.toVar(), true));
}

bool DrumLibraryCatalogService::isLibraryInstalled (const juce::String& libraryId) const
{
    const auto root = KitForgePaths::getLibraryInstallPath (libraryId);

    if (root.getChildFile ("kitforge").getChildFile ("kit.json").existsAsFile())
        return true;

    return root.getChildFile ("kit.json").existsAsFile();
}

bool DrumLibraryCatalogService::isPartialInstall (const juce::String& libraryId) const
{
    if (isLibraryInstalled (libraryId))
        return false;

    const auto root = KitForgePaths::getLibraryInstallPath (libraryId);
    return root.getChildFile ("source").isDirectory();
}

InstalledLibrary DrumLibraryCatalogService::getInstalledLibrary (const juce::String& libraryId) const
{
    const auto root = KitForgePaths::getLibraryInstallPath (libraryId);
    const auto installJson = root.getChildFile ("install.json");

    if (installJson.existsAsFile())
        return InstalledLibrary::fromInstallJson (installJson);

    InstalledLibrary legacy;
    legacy.id = libraryId;
    legacy.name = libraryId;
    legacy.installedPath = root.getFullPathName();
    legacy.rootPath = legacy.installedPath;
    legacy.kitForgePath = root.getChildFile ("kitforge").exists()
                            ? root.getChildFile ("kitforge").getFullPathName()
                            : legacy.installedPath;
    return legacy;
}

bool DrumLibraryCatalogService::removeInstalledLibrary (const juce::String& libraryId)
{
    const auto path = KitForgePaths::getLibraryInstallPath (libraryId);

    if (path.exists())
        return path.deleteRecursively();

    return false;
}
