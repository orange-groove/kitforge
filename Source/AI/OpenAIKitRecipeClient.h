#pragma once

#include "KitRecipe.h"

struct OpenAIRecipeResult
{
    bool success = false;
    juce::String errorMessage;
    KitRecipe recipe;
};

/** Calls OpenAI Chat Completions and parses a KitRecipe JSON response. */
class OpenAIKitRecipeClient
{
public:
    static juce::String kitRecipeSystemPrompt (bool editingExistingKit);

    static OpenAIRecipeResult requestRecipe (const juce::String& userPrompt,
                                             const juce::String& apiKey,
                                             const juce::String& model,
                                             const KitRecipe* currentKit = nullptr);

private:
    static juce::String buildUserMessage (const juce::String& userPrompt, const KitRecipe* currentKit);
    static juce::String buildRequestBody (const juce::String& userMessage,
                                          const juce::String& model,
                                          bool editingExistingKit);
    static juce::String extractJsonContent (const juce::String& llmText);
    static juce::String readHttpResponse (const juce::String& apiKey, const juce::String& requestBody);
};
