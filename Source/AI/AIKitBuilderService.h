#pragma once

#include "KitRecipe.h"
#include "KitRecipeValidator.h"
#include "../Models/KitModel.h"
#include "../Models/SampleIndex.h"
#include <atomic>
#include <functional>

struct AIKitBuildResult
{
    bool success = false;
    uint64_t requestId = 0;
    juce::String message;
    KitRecipe recipe;
    KitModel kit;
};

/** Sends prompts to OpenAI for kit recipes, then builds KitModel locally. */
class AIKitBuilderService
{
public:
    using CompletionCallback = std::function<void(const AIKitBuildResult& result)>;

    explicit AIKitBuilderService (SampleIndex& sampleIndexIn);

    bool isBusy() const { return busy.load(); }

    void submitPrompt (const juce::String& prompt,
                       const KitModel* currentKit,
                       float canvasWidth,
                       float canvasHeight,
                       CompletionCallback callback);

private:
    SampleIndex& sampleIndex;
    KitRecipeValidator validator;
    std::atomic<bool> busy { false };

    KitModel buildKitFromRecipe (const KitRecipe& recipe,
                                 float canvasWidth,
                                 float canvasHeight,
                                 const KitModel* existingKitForMerge = nullptr,
                                 bool layoutFromRecipe = true,
                                 int* samplesAttachedOut = nullptr) const;

    std::atomic<uint64_t> requestCounter { 0 };
};
