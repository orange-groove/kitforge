#include "InstalledLibrary.h"

InstalledLibrary InstalledLibrary::fromInstallJson (const juce::File& installJsonFile)
{
    InstalledLibrary lib;

    if (! installJsonFile.existsAsFile())
        return lib;

    juce::var parsed;

    if (juce::JSON::parse (installJsonFile.loadFileAsString(), parsed).failed())
        return lib;

    if (auto* obj = parsed.getDynamicObject())
    {
        lib.id = obj->getProperty ("id").toString();
        lib.name = obj->getProperty ("name").toString();
        lib.format = obj->getProperty ("format").toString();
        lib.license = obj->getProperty ("license").toString();
        lib.version = obj->getProperty ("version").toString();
        lib.installedPath = obj->getProperty ("installedPath").toString();
        lib.sourcePath = obj->getProperty ("sourcePath").toString();
        lib.kitForgePath = obj->getProperty ("kitForgePath").toString();
        lib.sfzFileUsed = obj->getProperty ("sfzFileUsed").toString();
        lib.sampleCount = (int) obj->getProperty ("sampleCount");
        lib.pieceCount = (int) obj->getProperty ("pieceCount");
        lib.installedAtMs = (int64_t) obj->getProperty ("installedAtMs");

        if (auto* tags = obj->getProperty ("tags").getArray())
            for (const auto& tag : *tags)
                lib.tags.add (tag.toString());
    }

    lib.rootPath = lib.installedPath;
    return lib;
}

bool InstalledLibrary::writeInstallJson (const juce::File& libraryRoot) const
{
    const auto file = libraryRoot.getChildFile ("install.json");
    return file.replaceWithText (juce::JSON::toString (toVar(), true));
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
    obj->setProperty ("installedPath", installedPath);
    obj->setProperty ("sourcePath", sourcePath);
    obj->setProperty ("kitForgePath", kitForgePath);
    obj->setProperty ("sfzFileUsed", sfzFileUsed);
    obj->setProperty ("sampleCount", sampleCount);
    obj->setProperty ("pieceCount", pieceCount);
    obj->setProperty ("installedAtMs", (double) installedAtMs);
    obj->setProperty ("tags", tagVars);
    return juce::var (obj);
}
