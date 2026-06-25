#pragma once

#include <JuceHeader.h>
#include "../Catalog/CatalogService.h"
#include "../Catalog/DownloadManager.h"
#include "../Catalog/DrumLibraryCatalogService.h"
#include "../Catalog/DrumLibraryDownloadManager.h"
#include "../Catalog/KitInstaller.h"
#include "../AI/AIKitBuilderService.h"
#include "../Importers/SFZImporter.h"
#include "../Importers/KontaktImporter.h"
#include "../Importers/KitForgePackImporter.h"
#include "../Models/SampleIndex.h"

/** Application-level services shared between processor and UI. */
class KitForgeServices
{
public:
    KitForgeServices();

    CatalogService& getCatalog() { return catalog; }
    DownloadManager& getDownloads() { return downloads; }
    DrumLibraryCatalogService& getDrumLibraryCatalog() { return drumLibraryCatalog; }
    DrumLibraryDownloadManager& getDrumLibraryDownloadManager() { return drumLibraryDownloadManager; }
    KitInstaller& getKitInstaller() { return kitInstaller; }
    SampleIndex& getSampleIndex() { return sampleIndex; }
    AIKitBuilderService& getAIBuilder() { return aiBuilder; }
    SFZImporter& getSFZImporter() { return sfzImporter; }
    KontaktImporter& getKontaktImporter() { return kontaktImporter; }
    KitForgePackImporter& getPackImporter() { return packImporter; }
    KitForgePackExporter& getPackExporter() { return packExporter; }

    void initialize();

    bool consumeDemoKitRepairFlag() { return std::exchange (demoKitWasRepaired, false); }

private:
    bool demoKitWasRepaired = false;
    CatalogService catalog;
    DownloadManager downloads;
    DrumLibraryCatalogService drumLibraryCatalog;
    SampleIndex sampleIndex;
    KitForgePackImporter packImporter;
    KitForgePackExporter packExporter;
    SFZImporter sfzImporter;
    KontaktImporter kontaktImporter;
    DrumLibraryDownloadManager drumLibraryDownloadManager;
    KitInstaller kitInstaller;
    AIKitBuilderService aiBuilder;
};
