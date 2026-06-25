#pragma once

#include "ImportWarning.h"
#include "../Models/KitModel.h"

struct ImportPreviewStats
{
    int wavFileCount = 0;
    int pieceCount = 0;
    int articulationCount = 0;
    int layerCount = 0;
    int roundRobinCount = 0;
};

struct ImportResult
{
    bool success = false;
    juce::String errorMessage;
    juce::String kitName;
    KitModel kit;
    std::vector<ImportWarning> warnings;
    ImportPreviewStats stats;
    juce::File sourceRoot;
    juce::String importFormat; // "loose_wav", "sfz"

    /** SFZ catalog import — multiple candidates and user selection. */
    juce::Array<juce::File> sfzCandidates;
    juce::File selectedSfzFile;

    static ImportPreviewStats computeStats (const KitModel& model, int wavFileCount);
};
