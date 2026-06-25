#include "DrumLibraryCatalogManifest.h"

DrumLibraryCatalogManifest DrumLibraryCatalogManifest::fromVar (const juce::var& v)
{
    DrumLibraryCatalogManifest manifest;

    if (auto* root = v.getDynamicObject())
    {
        manifest.version = (int) root->getProperty ("version");
        manifest.catalogUrl = root->getProperty ("catalogUrl").toString();

        if (auto* arr = root->getProperty ("libraries").getArray())
        {
            for (const auto& item : *arr)
            {
                if (auto* obj = item.getDynamicObject())
                {
                    OnlineDrumLibrary lib;
                    lib.id = obj->getProperty ("id").toString();
                    lib.name = obj->getProperty ("name").toString();
                    lib.format = obj->getProperty ("format").toString();
                    lib.license = obj->getProperty ("license").toString();
                    lib.sizeMb = (double) obj->getProperty ("sizeMb");
                    lib.homepageUrl = obj->getProperty ("homepageUrl").toString();
                    lib.downloadUrl = obj->getProperty ("downloadUrl").toString();
                    lib.description = obj->getProperty ("description").toString();
                    lib.recommended = (bool) obj->getProperty ("recommended");

                    if (auto* tags = obj->getProperty ("tags").getArray())
                        for (const auto& tag : *tags)
                            lib.tags.add (tag.toString());

                    manifest.libraries.add (std::move (lib));
                }
            }
        }
    }

    return manifest;
}

juce::var DrumLibraryCatalogManifest::toVar() const
{
    juce::Array<juce::var> libVars;

    for (const auto& lib : libraries)
    {
        juce::Array<juce::var> tagVars;

        for (const auto& tag : lib.tags)
            tagVars.add (tag);

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("id", lib.id);
        obj->setProperty ("name", lib.name);
        obj->setProperty ("format", lib.format);
        obj->setProperty ("license", lib.license);
        obj->setProperty ("sizeMb", lib.sizeMb);
        obj->setProperty ("homepageUrl", lib.homepageUrl);
        obj->setProperty ("downloadUrl", lib.downloadUrl);
        obj->setProperty ("description", lib.description);
        obj->setProperty ("recommended", lib.recommended);
        obj->setProperty ("tags", tagVars);
        libVars.add (juce::var (obj));
    }

    auto* root = new juce::DynamicObject();
    root->setProperty ("version", version);
    root->setProperty ("catalogUrl", catalogUrl);
    root->setProperty ("libraries", libVars);
    return juce::var (root);
}
