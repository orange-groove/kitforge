#include "KitForgeServices.h"
#include "KitForgePaths.h"
#include "DemoKitFactory.h"

KitForgeServices::KitForgeServices()
    : drumLibraryDownloadManager (downloads, sfzImporter, packImporter, drumLibraryCatalog),
      kitInstaller (downloads, packImporter, sfzImporter, sampleIndex),
      aiBuilder (sampleIndex)
{
}

void KitForgeServices::initialize()
{
    KitForgePaths::ensureDirectoryStructure();
    DemoKitFactory::ensureDemoPackExists();
    catalog.ensureDemoCatalogEntry();
    demoKitWasRepaired = DemoKitFactory::repairInstalledDemoKitIfNeeded();
    sampleIndex.scanLibrariesOnDisk();
}
