#pragma once

#include <JuceHeader.h>

/** Minimal shared wrapper around OpenAI Chat Completions (json_object responses).
    Used by both the kit-recipe builder and the sample classifier. */
namespace OpenAIChat
{
    struct Result
    {
        bool success = false;
        juce::String content;       // assistant message content (expected to be JSON)
        juce::String errorMessage;
    };

    /** Performs one blocking chat completion request. Safe to call off the message thread. */
    Result complete (const juce::String& systemPrompt,
                     const juce::String& userMessage,
                     const juce::String& apiKey,
                     const juce::String& model,
                     double temperature = 0.2);

    /** Removes ``` code fences if the model wrapped its JSON in them. */
    juce::String extractJsonContent (const juce::String& llmText);
}
