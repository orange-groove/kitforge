#pragma once

#include <JuceHeader.h>
#include "../AI/AIKitBuilderService.h"
#include "../Importers/SFZImporter.h"
#include "../Importers/LooseSampleFolderImporter.h"
#include "../Models/SampleIndex.h"
#include "../Models/SampleIndexService.h"
#include "../Core/KitImportService.h"

/** Application-level services shared between processor and UI. */
class KitForgeServices
{
public:
    KitForgeServices();

    SampleIndex& getSampleIndex() { return sampleIndex; }
    SampleIndexService& getSampleIndexService() { return sampleIndexService; }
    AIKitBuilderService& getAIBuilder() { return aiBuilder; }
    SFZImporter& getSFZImporter() { return sfzImporter; }
    LooseSampleFolderImporter& getLooseFolderImporter() { return looseFolderImporter; }
    KitImportService& getKitImportService() { return kitImportService; }

    void initialize();

private:
    SampleIndex sampleIndex;
    SampleIndexService sampleIndexService { sampleIndex };
    SFZImporter sfzImporter;
    LooseSampleFolderImporter looseFolderImporter;
    KitImportService kitImportService;
    AIKitBuilderService aiBuilder;
};
