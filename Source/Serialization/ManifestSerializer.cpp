#include "ManifestSerializer.h"

juce::var ManifestSerializer::manifestToVar (const KitManifest& manifest)
{
    juce::Array<juce::var> tagVars;

    for (const auto& tag : manifest.tags)
        tagVars.add (tag);

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("format", manifest.format);
    obj->setProperty ("formatVersion", manifest.formatVersion);
    obj->setProperty ("packageId", manifest.packageId);
    obj->setProperty ("name", manifest.name);
    obj->setProperty ("version", manifest.version);
    obj->setProperty ("author", manifest.author);
    obj->setProperty ("createdAt", manifest.createdAt);
    obj->setProperty ("updatedAt", manifest.updatedAt);
    obj->setProperty ("license", manifest.license);
    obj->setProperty ("creditsFile", manifest.creditsFile);
    obj->setProperty ("licenseFile", manifest.licenseFile);
    obj->setProperty ("kitFile", manifest.kitFile);
    obj->setProperty ("thumbnail", manifest.thumbnail);
    obj->setProperty ("previewAudio", manifest.previewAudio);
    obj->setProperty ("tags", tagVars);
    obj->setProperty ("sampleCount", manifest.sampleCount);
    obj->setProperty ("pieceCount", manifest.pieceCount);
    obj->setProperty ("installSizeBytes", manifest.installSizeBytes);
    return juce::var (obj);
}

KitManifest ManifestSerializer::manifestFromVar (const juce::var& v)
{
    KitManifest manifest;

    if (auto* obj = v.getDynamicObject())
    {
        manifest.format = obj->getProperty ("format").toString();
        manifest.formatVersion = (int) obj->getProperty ("formatVersion");
        manifest.packageId = obj->getProperty ("packageId").toString();
        manifest.name = obj->getProperty ("name").toString();
        manifest.version = obj->getProperty ("version").toString();
        manifest.author = obj->getProperty ("author").toString();
        manifest.createdAt = obj->getProperty ("createdAt").toString();
        manifest.updatedAt = obj->getProperty ("updatedAt").toString();
        manifest.license = obj->getProperty ("license").toString();
        manifest.creditsFile = obj->getProperty ("creditsFile").toString();
        manifest.licenseFile = obj->getProperty ("licenseFile").toString();
        manifest.kitFile = obj->getProperty ("kitFile").toString();
        manifest.thumbnail = obj->getProperty ("thumbnail").toString();
        manifest.previewAudio = obj->getProperty ("previewAudio").toString();
        manifest.sampleCount = (int) obj->getProperty ("sampleCount");
        manifest.pieceCount = (int) obj->getProperty ("pieceCount");
        manifest.installSizeBytes = (int64) (double) obj->getProperty ("installSizeBytes");

        if (auto* tags = obj->getProperty ("tags").getArray())
            for (const auto& tag : *tags)
                manifest.tags.add (tag.toString());
    }

    if (manifest.kitFile.isEmpty())
        manifest.kitFile = "kit.json";

    return manifest;
}

bool ManifestSerializer::readFromFile (const juce::File& file, KitManifest& manifestOut, juce::String& errorOut)
{
    if (! file.existsAsFile())
    {
        errorOut = "Missing manifest.json";
        return false;
    }

    juce::var parsed;

    if (juce::JSON::parse (file.loadFileAsString(), parsed).failed())
    {
        errorOut = "Failed to parse manifest.json";
        return false;
    }

    manifestOut = manifestFromVar (parsed);
    return true;
}

bool ManifestSerializer::writeToFile (const juce::File& file, const KitManifest& manifest, juce::String& errorOut)
{
    juce::ignoreUnused (errorOut);

    return file.replaceWithText (juce::JSON::toString (manifestToVar (manifest), true));
}
