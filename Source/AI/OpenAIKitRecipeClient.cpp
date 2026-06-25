#include "OpenAIKitRecipeClient.h"

namespace
{
    constexpr auto kOpenAiChatUrl = "https://api.openai.com/v1/chat/completions";
}

juce::String OpenAIKitRecipeClient::kitRecipeSystemPrompt (bool editingExistingKit)
{
    juce::String prompt = R"(You help KitForge build drum kits from user prompts. Reply with ONLY valid JSON:

{
  "name": string,
  "description": string,
  "mapProfile": string,
  "tags": string[],
  "pieces": [{
    "id": string,
    "type": "kick|snare|rackTom|floorTom|hiHat|crash|ride|china|splash",
    "name": string,
    "layoutXNorm": number (optional — omit for new kits; layout is applied locally),
    "layoutYNorm": number (optional),
    "chokeGroupId": string,
    "tags": string[],
    "articulations": [{ "name": string, "midiNote": number, "chokeGroupId": string }]
  }]
}

Each piece needs a stable "id" string. When editing, keep existing piece ids unchanged. New pieces get a new unique id.

PIECE COUNT
"N-piece" counts drum shells only (kick, snare, rackTom, floorTom). Cymbals are NEVER part of the count.

Shell inventory by piece count (you choose names, MIDI, and tags — NOT layout):
- 4-piece: kick, snare, 1 rackTom, 1 floorTom
- 5-piece: kick, snare, 2 rackTom, 1 floorTom
- 6-piece: kick, snare, 2 rackTom, 2 floorTom OR 3 rackTom + 1 floorTom
- 7-piece: kick, snare, 3 rackTom, 2 floorTom (exactly)
- 8-piece: kick, snare, 3 rackTom, 3 floorTom (exactly)

DEFAULT CYMBALS (new kits)
Unless the user mentions cymbals or asks to omit them, always include exactly these five cymbals (in this order):
1. Hi-Hat
2. Splash
3. Crash 1
4. Crash 2
5. Ride

KitForge applies left-to-right arc layout locally. Omit layout coordinates.
Optional extras (china, extra crashes) only when the user asks.

MIDI
When the user names a map (Roland V-Drums, Alesis, Yamaha DTX, General MIDI), set mapProfile accordingly.
KitForge applies canonical MIDI locally — focus on piece types and names, not note numbers.

Supported maps (primary hit notes):
Roland V-Drums — Kick 36 | Snare 38 (rim 40) | Hi-Hat 42 (open 46) | Racks 48/45/43 | Floors 41/50/48 | Crash 49/57 | Ride 51 | Splash 55 | China 52
Alesis (Nitro/Surge) — same shells/cymbals except Floors 41/47/48 | Splash 21
Yamaha DTX — same as Roland (modern DTX502/600/700 GM-compatible defaults)
General MIDI — same as Roland

LAYOUT
Do NOT calculate positions. KitForge applies shell and cymbal layout locally after your response.
- For new kits: omit layoutXNorm and layoutYNorm (or set to -1) on all pieces.
- Focus on which pieces exist, their types, names, articulations, and tags.
- When editing an existing kit: copy layoutXNorm and layoutYNorm exactly from the current kit for unchanged pieces.

Include genre tags. No sample file paths.)";

    if (editingExistingKit)
    {
        prompt << R"(

EDITING AN EXISTING KIT (CRITICAL)
The user message includes the current kit JSON. Apply ONLY the changes the user explicitly requests.
- Do NOT change any piece's id, name, type, layoutXNorm, layoutYNorm, articulations, tags, or mapProfile unless the user asked to change that specific thing.
- Return the COMPLETE kit JSON with ALL pieces — unchanged pieces copied exactly from the current kit.
- When the user changes piece count (e.g. "make it a 5-piece kit"): return the correct shell types/count for that setup. KEEP every cymbal from the current kit with the same ids and layout. Never drop cymbals because of a piece-count change. Shell layout is applied locally — omit layout on new/changed shells.
- Do not redesign the whole kit unless the user asked for that.)";
    }
    else
    {
        prompt << R"(

NEW KIT
No current kit was provided. Build the full kit from the user's request.)";
    }

    return prompt;
}

juce::String OpenAIKitRecipeClient::buildUserMessage (const juce::String& userPrompt,
                                                        const KitRecipe* currentKit)
{
    if (currentKit == nullptr || currentKit->pieces.empty())
        return userPrompt;

    return "Current kit JSON (preserve unchanged unless the request says otherwise):\n"
         + juce::JSON::toString (currentKit->toVar())
         + "\n\nRequested change:\n"
         + userPrompt;
}

juce::String OpenAIKitRecipeClient::buildRequestBody (const juce::String& userMessage,
                                                      const juce::String& model,
                                                      bool editingExistingKit)
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("model", model);

    auto* responseFormat = new juce::DynamicObject();
    responseFormat->setProperty ("type", "json_object");
    root->setProperty ("response_format", juce::var (responseFormat));

    juce::Array<juce::var> messages;

    {
        auto* systemMessage = new juce::DynamicObject();
        systemMessage->setProperty ("role", "system");
        systemMessage->setProperty ("content", kitRecipeSystemPrompt (editingExistingKit));
        messages.add (juce::var (systemMessage));
    }

    {
        auto* userMsg = new juce::DynamicObject();
        userMsg->setProperty ("role", "user");
        userMsg->setProperty ("content", userMessage);
        messages.add (juce::var (userMsg));
    }

    root->setProperty ("messages", messages);
    root->setProperty ("temperature", 0.2);

    return juce::JSON::toString (juce::var (root));
}

juce::String OpenAIKitRecipeClient::extractJsonContent (const juce::String& llmText)
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

juce::String OpenAIKitRecipeClient::readHttpResponse (const juce::String& apiKey, const juce::String& requestBody)
{
    juce::URL url (kOpenAiChatUrl);

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

OpenAIRecipeResult OpenAIKitRecipeClient::requestRecipe (const juce::String& userPrompt,
                                                         const juce::String& apiKey,
                                                         const juce::String& model,
                                                         const KitRecipe* currentKit)
{
    OpenAIRecipeResult result;

    if (apiKey.trim().isEmpty())
    {
        result.errorMessage = "OpenAI API key is empty.";
        return result;
    }

    if (userPrompt.trim().isEmpty())
    {
        result.errorMessage = "Prompt is empty.";
        return result;
    }

    const bool editing = currentKit != nullptr && ! currentKit->pieces.empty();
    const auto userMessage = buildUserMessage (userPrompt, currentKit);
    const auto requestBody = buildRequestBody (userMessage, model, editing);
    const auto httpResponse = readHttpResponse (apiKey, requestBody);

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

    juce::String content;

    if (auto* choices = parsed.getProperty ("choices", juce::var()).getArray())
    {
        if (! choices->isEmpty())
        {
            if (auto* message = choices->getReference (0).getProperty ("message", juce::var()).getDynamicObject())
                content = message->getProperty ("content").toString();
        }
    }

    if (content.isEmpty())
    {
        result.errorMessage = "OpenAI returned no kit recipe content.";
        return result;
    }

    const auto recipeJson = extractJsonContent (content);
    juce::var recipeVar;

    if (juce::JSON::parse (recipeJson, recipeVar).failed())
    {
        result.errorMessage = "Could not parse kit recipe JSON from OpenAI.";
        return result;
    }

    result.recipe = KitRecipe::fromVar (recipeVar);

    if (result.recipe.pieces.empty())
    {
        result.errorMessage = "OpenAI returned a recipe with no drum pieces.";
        return result;
    }

    result.success = true;
    return result;
}
