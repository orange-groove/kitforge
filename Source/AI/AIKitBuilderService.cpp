#include "AIKitBuilderService.h"
#include "../Core/KitForgeSettings.h"
#include "../Engine/Articulation.h"
#include "../Engine/DrumSample.h"
#include "../Engine/SampleLayer.h"
#include "../Models/DrumPiece.h"
#include "../Models/DrumPieceTypes.h"
#include "KitModelRecipeBridge.h"
#include "KitShellLayoutEnforcer.h"
#include "KitMidiMapEnforcer.h"
#include "OpenAIKitRecipeClient.h"
#include <algorithm>

namespace
{
    void copySampleLayersFromExisting (Articulation& target, const Articulation& source)
    {
        target.layers = source.layers;
    }

    const Articulation* findMatchingExistingArticulation (const DrumPiece& existing,
                                                          const Articulation& spec)
    {
        if (const auto* byNote = existing.findArticulationByMidiNote (spec.midiNote))
            return byNote;

        for (const auto& art : existing.articulations)
            if (art.name.equalsIgnoreCase (spec.name))
                return &art;

        return existing.getPrimaryArticulation();
    }

    void preserveCymbalsFromCurrentRecipe (const KitRecipe& current, KitRecipe& updated)
    {
        juce::StringArray updatedIds;

        for (const auto& piece : updated.pieces)
            updatedIds.add (piece.id);

        for (const auto& piece : current.pieces)
        {
            if (! isCymbalPieceType (piece.type))
                continue;

            if (! updatedIds.contains (piece.id))
                updated.pieces.push_back (piece);
        }
    }
}

AIKitBuilderService::AIKitBuilderService (SampleIndex& sampleIndexIn)
    : sampleIndex (sampleIndexIn)
{
}

void AIKitBuilderService::submitPrompt (const juce::String& prompt,
                                        const KitModel* currentKit,
                                        float canvasWidth,
                                        float canvasHeight,
                                        CompletionCallback callback)
{
    if (busy.exchange (true))
    {
        if (callback)
        {
            AIKitBuildResult busyResult;
            busyResult.success = false;
            busyResult.message = "A kit build is already in progress.";

            juce::MessageManager::callAsync ([callback, busyResult]
            {
                callback (busyResult);
            });
        }

        return;
    }

    const auto apiKey = KitForgeSettings::get().getOpenAiApiKey();
    const auto model = KitForgeSettings::get().getOpenAiModel();
    const auto trimmedPrompt = prompt.trim();
    const bool refineExistingKit = currentKit != nullptr
        && ! currentKit->getPieces().empty()
        && isRefineKitPrompt (trimmedPrompt);

    KitModel kitSnapshot;

    if (refineExistingKit)
        kitSnapshot = *currentKit;

    KitRecipe currentRecipe;

    if (refineExistingKit)
        currentRecipe = kitRecipeFromModel (kitSnapshot, canvasWidth, canvasHeight);

    const bool hasCurrentKit = refineExistingKit;
    KitRecipe currentRecipeCopy = currentRecipe;

    if (apiKey.trim().isEmpty())
    {
        busy = false;

        if (callback)
        {
            AIKitBuildResult result;
            result.success = false;
            result.message = "Add your OpenAI API key in Settings to use AI kit building.";

            juce::MessageManager::callAsync ([callback, result]
            {
                callback (result);
            });
        }

        return;
    }

    juce::Thread::launch ([this, trimmedPrompt, apiKey, model, currentRecipeCopy, hasCurrentKit, refineExistingKit, kitSnapshot, canvasWidth, canvasHeight, callback]
    {
        AIKitBuildResult result;
        result.requestId = ++requestCounter;
        const uint64_t requestId = result.requestId;
        const KitRecipe* currentPtr = hasCurrentKit ? &currentRecipeCopy : nullptr;
        const auto llmResult = OpenAIKitRecipeClient::requestRecipe (trimmedPrompt, apiKey, model, currentPtr);

        if (! llmResult.success)
        {
            result.success = false;
            result.message = llmResult.errorMessage;
        }
        else
        {
            result.recipe = llmResult.recipe;

            if (hasCurrentKit)
                preserveCymbalsFromCurrentRecipe (currentRecipeCopy, result.recipe);

            int targetShells = parseRequestedShellCount (trimmedPrompt);

            if (targetShells == 0 && ! hasCurrentKit)
            {
                const int shellsInRecipe = countShellsInRecipe (result.recipe);

                if (shellsInRecipe >= 4 && shellsInRecipe <= 8)
                    targetShells = shellsInRecipe;
            }

            if (targetShells >= 4 && targetShells <= 8)
                enforceShellCount (result.recipe, targetShells);

            if (! refineExistingKit)
            {
                enforceDefaultCymbals (result.recipe, trimmedPrompt);
                applyDefaultCymbalLayout (result.recipe);
                applyMidiMapToRecipe (result.recipe, trimmedPrompt);
            }

            const auto validation = validator.validate (result.recipe);

            if (! validation.ok)
            {
                result.success = false;
                result.message = validation.errors.joinIntoString ("\n");
            }
            else
            {
                int samplesAttached = 0;
                const KitModel* mergeSource = refineExistingKit ? &kitSnapshot : nullptr;
                const bool explicitPieceCountChange = parseRequestedShellCount (trimmedPrompt) > 0;
                const bool layoutFromRecipe = ! refineExistingKit || explicitPieceCountChange;
                result.kit = buildKitFromRecipe (result.recipe, canvasWidth, canvasHeight,
                                                 mergeSource, layoutFromRecipe, &samplesAttached);
                result.success = true;

                result.message = refineExistingKit ? "Updated kit" : "Built kit";
                result.message += ": " + result.recipe.name;

                if (result.recipe.mapProfile.isNotEmpty())
                    result.message += " (" + result.recipe.mapProfile + ")";

                const int pieceCount = (int) result.recipe.pieces.size();

                if (samplesAttached == 0)
                    result.message += " — layout and MIDI only (no matching samples in library)";
                else if (samplesAttached < pieceCount)
                    result.message += " — " + juce::String (samplesAttached) + " of "
                                   + juce::String (pieceCount) + " pieces matched samples";
            }
        }

        busy = false;

        if (callback)
        {
            juce::MessageManager::callAsync ([callback, result, requestId]
            {
                AIKitBuildResult delivered = result;
                delivered.requestId = requestId;
                callback (delivered);
            });
        }
    });
}

KitModel AIKitBuilderService::buildKitFromRecipe (const KitRecipe& recipe,
                                                   float canvasWidth,
                                                   float canvasHeight,
                                                   const KitModel* existingKitForMerge,
                                                   bool layoutFromRecipe,
                                                   int* samplesAttachedOut) const
{
    KitModel model = KitModel::createEmpty();
    model.kitName = recipe.name.isNotEmpty() ? recipe.name
                                             : (existingKitForMerge != nullptr ? existingKitForMerge->kitName
                                                                               : juce::String ("Custom Kit"));

    const int count = (int) recipe.pieces.size();
    int index = 0;
    int samplesAttached = 0;
    juce::StringArray seenPieceIds;

    for (const auto& spec : recipe.pieces)
    {
        if (spec.id.isNotEmpty() && seenPieceIds.contains (spec.id))
            continue;

        if (spec.id.isNotEmpty())
            seenPieceIds.add (spec.id);
        const DrumPiece* existingPiece = nullptr;

        if (existingKitForMerge != nullptr && spec.id.isNotEmpty())
            existingPiece = existingKitForMerge->findPieceById (spec.id);

        DrumPiece piece;
        piece.id = spec.id.isNotEmpty() ? spec.id : DrumPiece::makeId();
        piece.name = spec.name.isNotEmpty() ? spec.name : drumPieceTypeToString (spec.type);
        piece.type = spec.type;
        piece.shapeType = defaultShapeForType (spec.type);
        piece.primaryMidiNote = spec.preferredMidiNote > 0 ? spec.preferredMidiNote : 36;
        piece.chokeGroupId = spec.chokeGroupId;

        const float size = defaultPieceSize (spec.type);
        piece.width = size;
        piece.height = size;

        if (! layoutFromRecipe && existingPiece != nullptr)
        {
            piece.setBounds (existingPiece->getBounds());
        }
        else if (spec.layoutXNorm >= 0.0f && spec.layoutYNorm >= 0.0f)
        {
            piece.x = spec.layoutXNorm * canvasWidth - piece.width * 0.5f;
            piece.y = spec.layoutYNorm * canvasHeight - piece.height * 0.5f;
        }
        else if (existingPiece != nullptr)
        {
            piece.setBounds (existingPiece->getBounds());
        }
        else
        {
            const float xNorm = 0.2f + (0.6f * (float) index / (float) juce::jmax (1, count - 1));
            const float yNorm = 0.25f + (float) (index % 2) * 0.35f;
            piece.x = xNorm * canvasWidth - piece.width * 0.5f;
            piece.y = yNorm * canvasHeight - piece.height * 0.5f;
        }

        if (! spec.articulations.empty())
        {
            for (const auto& artSpec : spec.articulations)
            {
                Articulation art;
                art.id = Articulation::makeId();
                art.name = artSpec.name;
                art.midiNote = artSpec.midiNote > 0 ? artSpec.midiNote : piece.primaryMidiNote;
                art.chokeGroupId = artSpec.chokeGroupId.isNotEmpty() ? artSpec.chokeGroupId : spec.chokeGroupId;
                piece.addArticulation (std::move (art));
            }

            if (spec.articulations.front().midiNote > 0)
                piece.primaryMidiNote = spec.articulations.front().midiNote;
        }
        else
        {
            Articulation art;
            art.id = Articulation::makeId();
            art.name = spec.articulation.isNotEmpty() ? spec.articulation : "Hit";
            art.midiNote = piece.primaryMidiNote;
            art.chokeGroupId = spec.chokeGroupId;
            piece.addArticulation (std::move (art));
        }

        if (existingPiece != nullptr)
        {
            for (auto& art : piece.articulations)
            {
                if (const auto* existingArt = findMatchingExistingArticulation (*existingPiece, art))
                {
                    art.id = existingArt->id;
                    copySampleLayersFromExisting (art, *existingArt);
                }
            }
        }

        piece.syncMidiNotesFromArticulations();
        normalizePieceVisuals (piece);

        const bool hasSamples = existingPiece != nullptr
            && std::any_of (piece.articulations.begin(), piece.articulations.end(),
                            [] (const Articulation& art) { return ! art.layers.empty(); });

        if (hasSamples)
        {
            ++samplesAttached;
        }
        else
        {
            juce::StringArray searchTags = spec.tags;
            searchTags.addArray (recipe.globalTags);

            auto matches = sampleIndex.searchByTags (searchTags, spec.type, 1);

            if (matches.empty())
                matches = sampleIndex.searchByInstrumentType (spec.type, 1);

            if (! matches.empty())
            {
                SampleLayer layer;
                layer.id = SampleLayer::makeId();
                layer.minVelocity = matches.front().minVelocity;
                layer.maxVelocity = matches.front().maxVelocity;

                DrumSample sample;
                sample.id = DrumSample::makeId();
                sample.filePath = matches.front().filePath;
                layer.roundRobins.addSample (std::move (sample));

                if (auto* artPtr = piece.getPrimaryArticulation())
                    artPtr->layers.push_back (std::move (layer));

                ++samplesAttached;
            }
        }

        model.addPiece (std::move (piece));
        ++index;
    }

    if (samplesAttachedOut != nullptr)
        *samplesAttachedOut = samplesAttached;

    return model;
}
