#pragma once

#include "ImportResult.h"

/** Scans a folder recursively for WAV files and builds a KitModel from naming conventions. */
class LooseSampleFolderImporter
{
public:
    ImportResult scanFolder (const juce::File& folder) const;
};
