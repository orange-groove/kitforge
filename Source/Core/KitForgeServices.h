#pragma once

#include <JuceHeader.h>
#include "../AI/AIKitBuilderService.h"
#include "../Importers/SFZImporter.h"
#include "../Importers/LooseSampleFolderImporter.h"
#include "../Models/SampleIndex.h"
#include "../Core/KitImportService.h"

/** Application-level services shared between processor and UI. */
class KitForgeServices
{
public:
    KitForgeServices();

    SampleIndex& getSampleIndex() { return sampleIndex; }
    AIKitBuilderService& getAIBuilder() { return aiBuilder; }
    SFZImporter& getSFZImporter() { return sfzImporter; }
    LooseSampleFolderImporter& getLooseFolderImporter() { return looseFolderImporter; }
    KitImportService& getKitImportService() { return kitImportService; }

    void initialize();

    bool consumeDemoKitRepairFlag() { return std::exchange (demoKitWasRepaired, false); }

private:
    bool demoKitWasRepaired = false;
    SampleIndex sampleIndex;
    SFZImporter sfzImporter;
    LooseSampleFolderImporter looseFolderImporter;
    KitImportService kitImportService;
    AIKitBuilderService aiBuilder;
};
