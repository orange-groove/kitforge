#include "OpenAIChat.h"

namespace OpenAIChat
{
    namespace
    {
        constexpr auto kChatUrl = "https://api.openai.com/v1/chat/completions";

        juce::String buildRequestBody (const juce::String& systemPrompt,
                                       const juce::String& userMessage,
                                       const juce::String& model,
                                       double temperature)
        {
            auto* root = new juce::DynamicObject();
            root->setProperty ("model", model);

            auto* responseFormat = new juce::DynamicObject();
            responseFormat->setProperty ("type", "json_object");
            root->setProperty ("response_format", juce::var (responseFormat));

            juce::Array<juce::var> messages;

            {
                auto* sys = new juce::DynamicObject();
                sys->setProperty ("role", "system");
                sys->setProperty ("content", systemPrompt);
                messages.add (juce::var (sys));
            }

            {
                auto* usr = new juce::DynamicObject();
                usr->setProperty ("role", "user");
                usr->setProperty ("content", userMessage);
                messages.add (juce::var (usr));
            }

            root->setProperty ("messages", messages);
            root->setProperty ("temperature", temperature);

            return juce::JSON::toString (juce::var (root));
        }

        juce::String postForResponse (const juce::String& apiKey, const juce::String& requestBody)
        {
            juce::URL url (kChatUrl);

            const auto headers = "Authorization: Bearer " + apiKey
                               + "\r\nContent-Type: application/json\r\n";

            const auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                                     .withExtraHeaders (headers)
                                     .withConnectionTimeoutMs (90000)
                                     .withNumRedirectsToFollow (3);

            if (auto stream = url.withPOSTData (requestBody).createInputStream (options))
                return stream->readEntireStreamAsString();

            return {};
        }
    }

    juce::String extractJsonContent (const juce::String& llmText)
    {
        auto trimmed = llmText.trim();

        if (trimmed.startsWithChar ('`'))
        {
            const int firstNewline = trimmed.indexOfChar ('\n');

            if (firstNewline > 0)
                trimmed = trimmed.substring (firstNewline + 1);

            const int fence = trimmed.lastIndexOf ("```");

            if (fence > 0)
                trimmed = trimmed.substring (0, fence);
        }

        return trimmed.trim();
    }

    Result complete (const juce::String& systemPrompt,
                     const juce::String& userMessage,
                     const juce::String& apiKey,
                     const juce::String& model,
                     double temperature)
    {
        Result result;

        if (apiKey.trim().isEmpty())
        {
            result.errorMessage = "OpenAI API key is empty.";
            return result;
        }

        const auto body = buildRequestBody (systemPrompt, userMessage, model, temperature);
        const auto httpResponse = postForResponse (apiKey, body);

        if (httpResponse.isEmpty())
        {
            result.errorMessage = "Could not reach OpenAI (network error or timeout).";
            return result;
        }

        juce::var parsed;

        if (juce::JSON::parse (httpResponse, parsed).failed() || parsed.getDynamicObject() == nullptr)
        {
            result.errorMessage = "Invalid response from OpenAI.";
            return result;
        }

        if (auto* errorObj = parsed.getProperty ("error", juce::var()).getDynamicObject())
        {
            result.errorMessage = errorObj->getProperty ("message").toString();

            if (result.errorMessage.isEmpty())
                result.errorMessage = "OpenAI request failed.";

            return result;
        }

        if (auto* choices = parsed.getProperty ("choices", juce::var()).getArray())
        {
            if (! choices->isEmpty())
            {
                if (auto* message = choices->getReference (0).getProperty ("message", juce::var()).getDynamicObject())
                    result.content = message->getProperty ("content").toString();
            }
        }

        if (result.content.isEmpty())
        {
            result.errorMessage = "OpenAI returned no content.";
            return result;
        }

        result.success = true;
        return result;
    }
}
