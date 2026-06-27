#include "InstalledLibrary.h"

InstalledLibrary InstalledLibrary::fromManifest (const KitManifest& manifest,
                                                  const juce::File& installFolder,
                                                  int sampleCountIn,
                                                  int pieceCountIn)
{
    InstalledLibrary lib;
    lib.id = manifest.packageId;
    lib.name = manifest.name;
    lib.format = "kitforge";
    lib.license = manifest.license;
    lib.version = manifest.version;
    lib.author = manifest.author;
    lib.installedPath = installFolder.getFullPathName();
    lib.kitForgePath = lib.installedPath;
    lib.tags = manifest.tags;
    lib.sampleCount = sampleCountIn;
    lib.pieceCount = pieceCountIn;
    lib.installedAtMs = juce::Time::getCurrentTime().toMilliseconds();
    lib.sizeBytes = 0;
    lib.thumbnailPath = installFolder.getChildFile (manifest.thumbnail).getFullPathName();
    lib.previewAudioPath = installFolder.getChildFile (manifest.previewAudio).getFullPathName();
    lib.licensePath = installFolder.getChildFile (manifest.licenseFile).getFullPathName();
    lib.creditsPath = installFolder.getChildFile (manifest.creditsFile).getFullPathName();
    return lib;
}

juce::var InstalledLibrary::toVar() const
{
    juce::Array<juce::var> tagVars;

    for (const auto& tag : tags)
        tagVars.add (tag);

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("id", id);
    obj->setProperty ("name", name);
    obj->setProperty ("format", format);
    obj->setProperty ("license", license);
    obj->setProperty ("version", version);
    obj->setProperty ("author", author);
    obj->setProperty ("installedPath", installedPath);
    obj->setProperty ("sampleCount", sampleCount);
    obj->setProperty ("pieceCount", pieceCount);
    obj->setProperty ("tags", tagVars);
    obj->setProperty ("thumbnailPath", thumbnailPath);
    obj->setProperty ("previewAudioPath", previewAudioPath);
    return juce::var (obj);
}
