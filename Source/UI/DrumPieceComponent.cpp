#include "DrumPieceComponent.h"
#include "../PluginProcessor.h"
#include "../Models/DrumPieceTypes.h"

namespace
{
    constexpr float kResizeHandleSize = 12.0f;
}

DrumPieceComponent::DrumPieceComponent (KitForgeAudioProcessor& processorIn, const DrumPiece& piece)
    : processor (processorIn),
      pieceId (piece.id),
      pieceData (piece)
{
    updateFromModel (piece);
}

void DrumPieceComponent::updateFromModel (const DrumPiece& piece)
{
    pieceData = piece;
    pieceId = piece.id;

    if (dragMode == DragMode::none)
    {
        auto bounds = piece.getBounds();

        if (std::abs (bounds.getWidth() - bounds.getHeight()) > 0.5f)
        {
            const float size = juce::jmax (bounds.getWidth(), bounds.getHeight());
            bounds = bounds.withSizeKeepingCentre (size, size);
        }

        setBounds (bounds.toNearestInt());
    }

    repaint();
}

void DrumPieceComponent::setEditMode (bool shouldEdit)
{
    editMode = shouldEdit;
    repaint();
}

void DrumPieceComponent::setSelected (bool shouldSelect)
{
    if (selected != shouldSelect)
    {
        selected = shouldSelect;
        repaint();
    }
}

bool DrumPieceComponent::hitResizeHandle (juce::Point<float> pos) const
{
    if (! editMode)
        return false;

    const auto bounds = getLocalBounds().toFloat();
    const auto handle = juce::Rectangle<float> (bounds.getRight() - kResizeHandleSize,
                                                bounds.getBottom() - kResizeHandleSize,
                                                kResizeHandleSize,
                                                kResizeHandleSize);
    return handle.contains (pos);
}

juce::Path DrumPieceComponent::buildShapePath (juce::Rectangle<float> bounds) const
{
    juce::Path path;
    const auto shape = effectiveShapeForPiece (pieceData.type, pieceData.shapeType);

    switch (shape)
    {
        case ShapeType::rectangle:
            path.addRectangle (bounds);
            break;

        case ShapeType::polygon:
            path.addStar (bounds.getCentre(), 8, bounds.getWidth() * 0.2f, bounds.getWidth() * 0.48f, 0.0f);
            break;

        case ShapeType::circle:
        case ShapeType::cymbal:
        case ShapeType::oval:
        default:
            path.addEllipse (bounds);
            break;
    }

    if (pieceData.rotation != 0.0f)
        path.applyTransform (juce::AffineTransform::rotation (pieceData.rotation, bounds.getCentreX(), bounds.getCentreY()));

    return path;
}

void DrumPieceComponent::paint (juce::Graphics& g)
{
    const auto shapeBounds = getLocalBounds().toFloat().reduced (2.0f);
    const auto path = buildShapePath (shapeBounds);
    const bool cymbal = isCymbalPieceType (pieceData.type);

    const auto fillColour = cymbal ? cymbalFillColour() : drumShellFillColour();
    const auto rimColour = cymbal ? cymbalRimColour() : drumShellRimColour();

    g.setColour (selected ? fillColour.brighter (0.12f) : fillColour);
    g.fillPath (path);

    g.setColour (selected ? rimColour.contrasting (0.25f) : rimColour);
    g.strokePath (path, juce::PathStrokeType (selected ? 3.5f : 3.0f));

    if (editMode)
    {
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.fillRect (getWidth() - (int) kResizeHandleSize,
                    getHeight() - (int) kResizeHandleSize,
                    (int) kResizeHandleSize,
                    (int) kResizeHandleSize);
    }

    const auto centre = shapeBounds.getCentre();
    constexpr float nameLineHeight = 18.0f;
    constexpr float noteLineHeight = 22.0f;
    const float blockTop = centre.y - (nameLineHeight + noteLineHeight) * 0.5f;
    const float labelWidth = juce::jmin (shapeBounds.getWidth() - 8.0f, 120.0f);

    juce::Rectangle<float> nameArea (centre.x - labelWidth * 0.5f, blockTop, labelWidth, nameLineHeight);
    juce::Rectangle<float> noteArea (centre.x - labelWidth * 0.5f, blockTop + nameLineHeight, labelWidth, noteLineHeight);

    g.setColour (cymbal ? juce::Colours::white : juce::Colours::black);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawFittedText (pieceData.name, nameArea.toNearestInt(), juce::Justification::centred, 1);

    g.setFont (juce::FontOptions (17.0f, juce::Font::bold));
    g.drawFittedText (juce::String (pieceData.primaryMidiNote), noteArea.toNearestInt(),
                      juce::Justification::centred, 1);
}

void DrumPieceComponent::mouseDown (const juce::MouseEvent& e)
{
    if (onSelected)
        onSelected (pieceId);

    if (e.mods.isPopupMenu())
    {
        showContextMenu();
        return;
    }

    if (editMode)
    {
        dragStart = e.getEventRelativeTo (getParentComponent()).position;
        boundsAtDragStart = getBounds().toFloat();
        dragMode = hitResizeHandle (e.position) ? DragMode::resize : DragMode::move;
        return;
    }

    if (onRequestAudition)
        onRequestAudition (pieceId);
}

void DrumPieceComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (! editMode || dragMode == DragMode::none)
        return;

    const auto parentPos = e.getEventRelativeTo (getParentComponent()).position;
    const auto delta = parentPos - dragStart;

    if (dragMode == DragMode::move)
    {
        applyLocalBounds (boundsAtDragStart.translated (delta.x, delta.y));
    }
    else
    {
        const float deltaMax = juce::jmax (delta.x, delta.y);
        const float newSize = juce::jmax (40.0f, boundsAtDragStart.getWidth() + deltaMax);
        auto newBounds = boundsAtDragStart.withSizeKeepingCentre (newSize, newSize);
        applyLocalBounds (newBounds);
    }
}

void DrumPieceComponent::mouseUp (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);

    if (dragMode != DragMode::none)
        commitLayoutToModel();

    dragMode = DragMode::none;
}

void DrumPieceComponent::showContextMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Learn MIDI Note");
    menu.addItem (2, "Assign Single Sample");
    menu.addItem (3, "Add Velocity Layer");
    menu.addItem (4, "Add Round Robin Sample");
    menu.addSeparator();
    menu.addItem (5, "Rename");
    menu.addItem (6, "Delete");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [this] (int result)
                        {
                            switch (result)
                            {
                                case 1: if (onRequestLearnMidi)       onRequestLearnMidi (pieceId); break;
                                case 2: if (onRequestAssignSample)     onRequestAssignSample (pieceId); break;
                                case 3: if (onRequestAddVelocityLayer) onRequestAddVelocityLayer (pieceId); break;
                                case 4: if (onRequestAddRoundRobin)    onRequestAddRoundRobin (pieceId); break;
                                case 5: if (onRequestRename)           onRequestRename (pieceId); break;
                                case 6: if (onRequestDelete)           onRequestDelete (pieceId); break;
                                default: break;
                            }
                        });
}

void DrumPieceComponent::applyLocalBounds (juce::Rectangle<float> bounds)
{
    pieceData.setBounds (bounds);
    setBounds (bounds.toNearestInt());
    repaint();
}

void DrumPieceComponent::commitLayoutToModel()
{
    const juce::ScopedLock lock (processor.getModelLock());

    if (auto* modelPiece = processor.getKitModel().findPieceById (pieceId))
        modelPiece->setBounds (pieceData.getBounds());

    processor.getKitModel().recordLayoutEdit();

    if (onPieceChanged)
        onPieceChanged (pieceId);
}
