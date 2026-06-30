#include "CatalogService.h"
#include "../Core/KitForgePaths.h"
#include "../Serialization/KitSerializer.h"
#include "../Models/KitModel.h"

namespace
{
    juce::File findFirstSfzFile (const juce::File& root)
    {
        juce::Array<juce::File> matches;
    root.findChildFiles (matches, juce::File::findFiles, true, "*.sfz");
    return matches.isEmpty() ? juce::File() : matches.getReference (0);
    }
}

CatalogManifest CatalogManifest::fromVar (const juce::var& v)
{
    CatalogManifest manifest;

    if (auto* root = v.getDynamicObject())
    {
        manifest.version = (int) root->getProperty ("version");
        manifest.catalogUrl = root->getProperty ("catalogUrl").toString();

        if (auto* arr = root->getProperty ("kits").getArray())
        {
            for (const auto& item : *arr)
            {
                if (auto* obj = item.getDynamicObject())
                {
                    CatalogKitEntry entry;
                    entry.id = obj->getProperty ("id").toString();
                    entry.name = obj->getProperty ("name").toString();
                    entry.format = obj->getProperty ("format").toString();
                    entry.sizeMb = (double) obj->getProperty ("sizeMb");
                    entry.downloadUrl = obj->getProperty ("downloadUrl").toString();
                    entry.thumbnailUrl = obj->getProperty ("thumbnail").toString();
                    entry.description = obj->getProperty ("description").toString();
                    entry.version = obj->getProperty ("version").toString();

                    if (auto* tags = obj->getProperty ("tags").getArray())
                        for (const auto& tag : *tags)
                            entry.tags.add (tag.toString());

                    manifest.kits.add (std::move (entry));
                }
            }
        }
    }

    return manifest;
}

juce::var CatalogManifest::toVar() const
{
    juce::Array<juce::var> kitVars;

    for (const auto& kit : kits)
    {
        juce::Array<juce::var> tagVars;

        for (const auto& tag : kit.tags)
            tagVars.add (tag);

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("id", kit.id);
        obj->setProperty ("name", kit.name);
        obj->setProperty ("format", kit.format);
        obj->setProperty ("sizeMb", kit.sizeMb);
        obj->setProperty ("downloadUrl", kit.downloadUrl);
        obj->setProperty ("thumbnail", kit.thumbnailUrl);
        obj->setProperty ("description", kit.description);
        obj->setProperty ("version", kit.version);
        obj->setProperty ("tags", tagVars);
        kitVars.add (juce::var (obj));
    }

    auto* root = new juce::DynamicObject();
    root->setProperty ("version", version);
    root->setProperty ("catalogUrl", catalogUrl);
    root->setProperty ("kits", kitVars);
    return juce::var (root);
}

CatalogService::CatalogService()
{
    if (! loadCachedManifestFromDisk())
        loadBundledMockManifest();

    ensureDemoCatalogEntry();
}

const CatalogKitEntry* CatalogService::findKitById (const juce::String& kitId) const
{
    for (const auto& kit : cachedManifest.kits)
        if (kit.id == kitId)
            return &kit;

    return nullptr;
}

void CatalogService::refreshManifest (ManifestCallback callback)
{
    downloadManifestAsync ([this, callback] (bool success, const CatalogManifest& manifest)
    {
        if (success)
        {
            cachedManifest = manifest;
            ensureDemoCatalogEntry();
            saveCachedManifestToDisk();
        }
        else
        {
            loadCachedManifestFromDisk();

            if (! hasCachedManifest())
                loadBundledMockManifest();

            ensureDemoCatalogEntry();
        }

        if (callback)
            callback (success || hasCachedManifest(), cachedManifest);
    });
}

void CatalogService::downloadManifestAsync (ManifestCallback callback)
{
    if (manifestUrl.startsWithIgnoreCase ("file:"))
    {
        const auto file = juce::URL (manifestUrl).getLocalFile();
        juce::var parsed;

        if (file.existsAsFile() && juce::JSON::parse (file.loadFileAsString(), parsed).wasOk())
        {
            if (callback)
                callback (true, CatalogManifest::fromVar (parsed));
            return;
        }
    }

    juce::Thread::launch ([this, callback]
    {
        juce::URL url (manifestUrl);
        const auto tempFile = KitForgePaths::getManifestCacheFile().getSiblingFile ("manifest-download.json");

        auto task = url.downloadToFile (tempFile, juce::URL::DownloadTaskOptions {});
        bool success = false;
        CatalogManifest manifest;

        if (task != nullptr)
        {
            while (! task->isFinished())
                juce::Thread::sleep (50);

            if (! task->hadError() && tempFile.existsAsFile())
            {
                juce::var parsed;

                if (juce::JSON::parse (tempFile.loadFileAsString(), parsed).wasOk())
                {
                    manifest = CatalogManifest::fromVar (parsed);
                    success = manifest.kits.size() > 0;
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

void CatalogService::ensureDemoCatalogEntry()
{
    for (int i = cachedManifest.kits.size(); --i >= 0;)
    {
        if (cachedManifest.kits.getReference (i).id == "demo-rock-kit")
            cachedManifest.kits.remove (i);
    }
}

juce::Array<CatalogKitEntry> CatalogService::search (const juce::String& query) const
{
    juce::Array<CatalogKitEntry> results;
    const auto q = query.trim().toLowerCase();

    if (q.isEmpty())
    {
        for (const auto& kit : cachedManifest.kits)
            results.add (kit);

        return results;
    }

    for (const auto& kit : cachedManifest.kits)
    {
        if (kit.name.toLowerCase().contains (q)
            || kit.id.toLowerCase().contains (q)
            || kit.description.toLowerCase().contains (q))
        {
            results.add (kit);
        }
    }

    return results;
}

juce::Array<CatalogKitEntry> CatalogService::filterByTag (const juce::String& tag) const
{
    juce::Array<CatalogKitEntry> results;

    for (const auto& kit : cachedManifest.kits)
        if (kit.tags.contains (tag, true))
            results.add (kit);

    return results;
}

bool CatalogService::loadCachedManifestFromDisk()
{
    const auto file = KitForgePaths::getManifestCacheFile();

    if (! file.existsAsFile())
        return false;

    juce::var parsed;

    if (juce::JSON::parse (file.loadFileAsString(), parsed).failed())
        return false;

    cachedManifest = CatalogManifest::fromVar (parsed);
    return cachedManifest.kits.size() > 0;
}

bool CatalogService::saveCachedManifestToDisk() const
{
    KitForgePaths::ensureDirectoryStructure();
    return KitForgePaths::getManifestCacheFile().replaceWithText (juce::JSON::toString (cachedManifest.toVar(), true));
}

bool CatalogService::removeKit (const juce::String& kitId)
{
    const auto path = KitForgePaths::getLibraryInstallPath (kitId);

    if (path.exists())
        return path.deleteRecursively();

    return false;
}

bool CatalogService::isKitInstalled (const juce::String& kitId) const
{
    const auto path = KitForgePaths::getLibraryInstallPath (kitId);

    if (! path.isDirectory())
        return false;

    return path.getChildFile ("kit.json").existsAsFile()
        || path.getChildFile (".kitforge-version").existsAsFile()
        || findFirstSfzFile (path).existsAsFile();
}

juce::String CatalogService::getInstalledVersion (const juce::String& kitId) const
{
    const auto versionFile = KitForgePaths::getLibraryInstallPath (kitId).getChildFile (".kitforge-version");

    if (! versionFile.existsAsFile())
        return {};

    return versionFile.loadFileAsString().trim();
}

bool CatalogService::installedKitIsIncomplete (const juce::String& kitId) const
{
    const auto path = KitForgePaths::getLibraryInstallPath (kitId);

    if (! path.isDirectory())
        return false;

    const auto kitJson = path.getChildFile ("kit.json");

    if (! kitJson.existsAsFile())
        return true;

    if (kitJson.existsAsFile() && ! path.getChildFile ("samples").isDirectory())
        return true;

    const auto wavFiles = path.getChildFile ("samples").findChildFiles (juce::File::findFiles, false, "*.wav");

    if (wavFiles.isEmpty())
        return true;

    juce::var parsed;

    if (juce::JSON::parse (kitJson.loadFileAsString(), parsed).failed())
        return true;

    KitModel kit;
    KitSerializer::kitFromVar (kit, parsed);

    for (const auto& piece : kit.getPieces())
    {
        for (const auto& art : piece.articulations)
        {
            for (const auto& layer : art.layers)
            {
                for (const auto& sample : layer.roundRobins.samples)
                {
                    if (sample.filePath.isNotEmpty())
                        return false;
                }
            }
        }
    }

    return true;
}

bool CatalogService::kitNeedsReinstall (const CatalogKitEntry& kit) const
{
    if (! isKitInstalled (kit.id))
        return false;

    if (installedKitIsIncomplete (kit.id))
        return true;

    if (kit.version.isEmpty())
        return false;

    return getInstalledVersion (kit.id) != kit.version;
}

void CatalogService::loadBundledMockManifest()
{
    cachedManifest.version = 1;
    cachedManifest.catalogUrl = manifestUrl;

    CatalogKitEntry sm;
    sm.id = "sm-drums";
    sm.name = "SM Drums";
    sm.format = "sfz";
    sm.sizeMb = 2200.0;
    sm.downloadUrl = "https://catalog.kitforge.app/kits/sm-drums.zip";
    sm.thumbnailUrl = "https://catalog.kitforge.app/thumbs/sm-drums.png";
    sm.description = "Multi-velocity acoustic rock kit (SFZ)";
    sm.version = "1.0";
    sm.tags = { "rock", "acoustic", "multi-velocity" };
    cachedManifest.kits.add (sm);

    CatalogKitEntry jazz;
    jazz.id = "jazz-brush-kit";
    jazz.name = "Jazz Brush Kit";
    jazz.format = "kitforgepack";
    jazz.sizeMb = 480.0;
    jazz.downloadUrl = "https://catalog.kitforge.app/kits/jazz-brush-kit.kitforgepack";
    jazz.description = "Dry jazz kit with brush articulations";
    jazz.tags = { "jazz", "brushes", "dry" };
    cachedManifest.kits.add (jazz);
}
