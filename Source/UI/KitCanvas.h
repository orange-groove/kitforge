#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"

class KitForgeAudioProcessor;
class DrumPieceComponent;

class KitCanvas final : public juce::Component
{
public:
    explicit KitCanvas (KitForgeAudioProcessor& processorIn);

    void setEditMode (bool shouldEdit);
    void rebuildFromModel();
    void refreshPieces();
    /** Writes all on-canvas piece bounds into the kit model (including in-progress drags). */
    void syncAllPieceLayoutsToModel (bool recordEdit = true);
    void setSelectedPieceId (const juce::String& pieceId);

    juce::String getSelectedPieceId() const { return selectedPieceId; }

    std::function<void()> onModelChanged;
    std::function<void(const juce::String& pieceId)> onSelectionChanged;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void handleAddVelocityLayerForSelection();
    void handleAssignSingleSample (const juce::String& pieceId);
    void handleAddVelocityLayer (const juce::String& pieceId);
    void handleAddRoundRobin (const juce::String& pieceId);

private:
    KitForgeAudioProcessor& processor;
    bool editMode = false;
    juce::String selectedPieceId;

    void wirePieceCallbacks (DrumPieceComponent& comp);
    void handleRename (const juce::String& pieceId);
    void chooseWavFile (std::function<void(const juce::File&)> onChosen);
    void applyDisplayLayerOrder();
};
