#include "AIBuilderPanel.h"
#include "../PluginProcessor.h"
#include "../AI/AIKitBuilderService.h"
#include "../AI/KitShellLayoutEnforcer.h"
#include "../Core/KitForgeSettings.h"
#include "../Models/KitModel.h"

AIBuilderPanel::AIBuilderPanel (KitForgeAudioProcessor& processorIn,
                                std::function<juce::Point<float>()> canvasSizeProviderIn)
    : processorRef (processorIn),
      canvasSizeProvider (std::move (canvasSizeProviderIn))
{
    addAndMakeVisible (promptBox);
    promptBox.setMultiLine (true);
    promptBox.setReturnKeyStartsNewLine (true);
    promptBox.setTextToShowWhenEmpty ("Describe your kit (e.g. Roland V-Drums 8-piece rock kit)...",
                                      juce::Colours::grey);
    promptBox.setText ("Using a Roland V drum mapping, create a 7 piece kit for rock");

    addAndMakeVisible (buildButton);
    buildButton.onClick = [this] { handleBuild(); };

    addAndMakeVisible (closeButton);
    closeButton.onClick = [this]
    {
        setVisible (false);

        if (onDismiss)
            onDismiss();
    };

    addAndMakeVisible (statusLabel);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    statusLabel.setFont (juce::FontOptions (12.0f));
}

void AIBuilderPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xf0181818));
    g.setColour (juce::Colour (0xff505050));
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());
}

void AIBuilderPanel::resized()
{
    auto area = getLocalBounds().reduced (12, 10);
    auto buttonRow = area.removeFromBottom (30);
    buildButton.setBounds (buttonRow.removeFromRight (72));
    buttonRow.removeFromRight (8);
    closeButton.setBounds (buttonRow.removeFromRight (64));
    statusLabel.setBounds (buttonRow);
    promptBox.setBounds (area);
}

void AIBuilderPanel::handleBuild()
{
    if (KitForgeSettings::get().getOpenAiApiKey().trim().isEmpty())
    {
        statusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
        statusLabel.setText ("Add your OpenAI API key in Settings first.", juce::dontSendNotification);
        return;
    }

    buildButton.setEnabled (false);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
    statusLabel.setText ("Generating kit layout and MIDI mapping...", juce::dontSendNotification);

    const auto canvasSize = canvasSizeProvider != nullptr
        ? canvasSizeProvider()
        : juce::Point<float> (900.0f, 520.0f);
    const float canvasWidth = juce::jmax (canvasSize.x, 1.0f);
    const float canvasHeight = juce::jmax (canvasSize.y, 1.0f);

    KitModel kitSnapshot;
    uint64_t changeGenerationAtSubmit = 0;
    const auto promptText = promptBox.getText();
    const bool refineExistingKit = isRefineKitPrompt (promptText);
    const KitModel* kitForRequest = nullptr;

    {
        const juce::ScopedLock lock (processorRef.getModelLock());

        if (refineExistingKit)
        {
            kitSnapshot = processorRef.getKitModel();
            kitForRequest = &kitSnapshot;
        }

        changeGenerationAtSubmit = processorRef.getKitModel().getChangeGeneration();
    }

    const uint64_t capturedRequestId = ++pendingRequestId;

    processorRef.getServices().getAIBuilder().submitPrompt (promptText,
        kitForRequest,
        canvasWidth,
        canvasHeight,
        [this, capturedRequestId, changeGenerationAtSubmit, refineExistingKit] (const AIKitBuildResult& result)
        {
            buildButton.setEnabled (true);

            if (result.requestId != capturedRequestId || capturedRequestId != pendingRequestId)
                return;

            if (refineExistingKit)
            {
                uint64_t currentGeneration = 0;

                {
                    const juce::ScopedLock lock (processorRef.getModelLock());
                    currentGeneration = processorRef.getKitModel().getChangeGeneration();
                }

                if (currentGeneration != changeGenerationAtSubmit)
                {
                    statusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
                    statusLabel.setText ("Kit was edited while waiting — AI result discarded.",
                                         juce::dontSendNotification);
                    return;
                }
            }

            if (result.success)
            {
                {
                    const juce::ScopedLock lock (processorRef.getModelLock());
                    processorRef.getKitModel().importContents (result.kit);
                }

                processorRef.rebuildEngine();
                statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
                statusLabel.setText (result.message, juce::dontSendNotification);

                if (onKitApplied)
                    onKitApplied();
            }
            else
            {
                statusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
                statusLabel.setText (result.message, juce::dontSendNotification);
            }
        });
}
