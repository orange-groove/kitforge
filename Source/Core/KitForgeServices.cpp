#include "KitForgeServices.h"
#include "KitForgePaths.h"
#include "DemoKitFactory.h"

KitForgeServices::KitForgeServices()
    : kitImportService (sampleIndex),
      aiBuilder (sampleIndex)
{
}

void KitForgeServices::initialize()
{
    KitForgePaths::ensureDirectoryStructure();
    DemoKitFactory::ensureDemoPackExists();
    demoKitWasRepaired = DemoKitFactory::repairInstalledDemoKitIfNeeded();
    sampleIndex.scanKitsOnDisk();
}
