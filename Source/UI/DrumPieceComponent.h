#pragma once

#include <JuceHeader.h>
#include "../Models/DrumPiece.h"

class KitForgeAudioProcessor;

class DrumPieceComponent final : public juce::Component
{
public:
    DrumPieceComponent (KitForgeAudioProcessor& processorIn, const DrumPiece& piece);

    void updateFromModel (const DrumPiece& piece);
    juce::String getPieceId() const { return pieceId; }
    DrumPieceType getPieceType() const { return pieceData.type; }
    bool isLayoutEditing() const { return dragMode != DragMode::none; }

    void setEditMode (bool shouldEdit);
    void setSelected (bool shouldSelect);
    bool isEditMode() const { return editMode; }

    std::function<void(const juce::String& pieceId)> onPieceChanged;
    std::function<void(const juce::String& pieceId)> onSelected;
    std::function<void(const juce::String& pieceId)> onRequestAudition;
    std::function<void(const juce::String& pieceId)> onRequestDelete;
    std::function<void(const juce::String& pieceId)> onRequestLearnMidi;
    std::function<void(const juce::String& pieceId)> onRequestAssignSample;
    std::function<void(const juce::String& pieceId)> onRequestAddVelocityLayer;
    std::function<void(const juce::String& pieceId)> onRequestAddRoundRobin;
    std::function<void(const juce::String& pieceId)> onRequestRename;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    enum class DragMode { none, move, resize };

    KitForgeAudioProcessor& processor;
    juce::String pieceId;
    DrumPiece pieceData;
    bool editMode = false;
    bool selected = false;

    DragMode dragMode = DragMode::none;
    juce::Point<float> dragStart;
    juce::Rectangle<float> boundsAtDragStart;

    bool hitResizeHandle (juce::Point<float> pos) const;
    juce::Path buildShapePath (juce::Rectangle<float> bounds) const;
    void showContextMenu();
    void applyLocalBounds (juce::Rectangle<float> bounds);
    void commitLayoutToModel();
};
