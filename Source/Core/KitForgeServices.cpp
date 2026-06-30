#include "KitForgeServices.h"
#include "KitForgePaths.h"

KitForgeServices::KitForgeServices()
    : kitImportService (sampleIndex),
      aiBuilder (sampleIndex)
{
}

void KitForgeServices::initialize()
{
    KitForgePaths::ensureDirectoryStructure();
    sampleIndex.scanKitsOnDisk();
}
