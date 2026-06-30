#include "KitCanvas.h"
#include "../PluginProcessor.h"
#include "DrumPieceComponent.h"
#include "../Models/DrumPieceTypes.h"
#include <algorithm>
#include <vector>

KitCanvas::KitCanvas (KitForgeAudioProcessor& processorIn)
    : processor (processorIn)
{
    processor.getKitModel().addListener ([this]
    {
        juce::MessageManager::callAsync ([this] { rebuildFromModel(); });
    });

    rebuildFromModel();
}

void KitCanvas::setEditMode (bool shouldEdit)
{
    editMode = shouldEdit;

    for (int i = 0; i < getNumChildComponents(); ++i)
        if (auto* pieceComp = dynamic_cast<DrumPieceComponent*> (getChildComponent (i)))
            pieceComp->setEditMode (editMode);

    repaint();
}

void KitCanvas::setSelectedPieceId (const juce::String& pieceId)
{
    selectedPieceId = pieceId;

    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        if (auto* comp = dynamic_cast<DrumPieceComponent*> (getChildComponent (i)))
            comp->setSelected (comp->getPieceId() == pieceId);
    }

    if (onSelectionChanged)
        onSelectionChanged (pieceId);
}

void KitCanvas::applyDisplayLayerOrder()
{
    struct LayerItem
    {
        DrumPieceComponent* comp = nullptr;
        int order = 0;
        float y = 0.0f;
    };

    std::vector<LayerItem> items;

    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        if (auto* comp = dynamic_cast<DrumPieceComponent*> (getChildComponent (i)))
        {
            items.push_back ({ comp, displayLayerOrder (comp->getPieceType()), (float) comp->getY() });
        }
    }

    std::sort (items.begin(), items.end(), [] (const LayerItem& a, const LayerItem& b)
    {
        if (a.order != b.order)
            return a.order < b.order;

        return a.y < b.y;
    });

    for (const auto& item : items)
        item.comp->toFront (false);
}

void KitCanvas::syncAllPieceLayoutsToModel (bool recordEdit)
{
    const juce::ScopedLock lock (processor.getModelLock());

    for (int i = 0; i < getNumChildComponents(); ++i)
    {
        if (auto* comp = dynamic_cast<DrumPieceComponent*> (getChildComponent (i)))
        {
            if (auto* modelPiece = processor.getKitModel().findPieceById (comp->getPieceId()))
                modelPiece->setBounds (comp->getBounds().toFloat());
        }
    }

    if (recordEdit)
        processor.getKitModel().recordLayoutEdit();
}

void KitCanvas::rebuildFromModel()
{
    removeAllChildren();

    const juce::ScopedLock lock (processor.getModelLock());

    std::vector<const DrumPiece*> sorted;

    for (const auto& piece : processor.getKitModel().getPieces())
        sorted.push_back (&piece);

    std::stable_sort (sorted.begin(), sorted.end(), [] (const DrumPiece* a, const DrumPiece* b)
    {
        return displayLayerOrder (a->type) < displayLayerOrder (b->type);
    });

    for (const auto* piece : sorted)
    {
        auto* comp = new DrumPieceComponent (processor, *piece);
        comp->setEditMode (editMode);
        wirePieceCallbacks (*comp);
        addAndMakeVisible (comp);
    }

    applyDisplayLayerOrder();
}

void KitCanvas::refreshPieces()
{
    const juce::ScopedLock lock (processor.getModelLock());

    for (const auto& piece : processor.getKitModel().getPieces())
    {
        for (int i = 0; i < getNumChildComponents(); ++i)
        {
            if (auto* comp = dynamic_cast<DrumPieceComponent*> (getChildComponent (i)))
            {
                if (comp->getPieceId() == piece.id)
                    comp->updateFromModel (piece);
            }
        }
    }
}

void KitCanvas::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));

    g.setColour (juce::Colours::white.withAlpha (0.06f));
    const int grid = 40;

    for (int x = 0; x < getWidth(); x += grid)
        g.drawVerticalLine (x, 0.0f, (float) getHeight());

    for (int y = 0; y < getHeight(); y += grid)
        g.drawHorizontalLine (y, 0.0f, (float) getWidth());

    if (processor.getMidiLearnManager().isLearning())
    {
        g.setColour (juce::Colours::orange.withAlpha (0.9f));
        g.setFont (15.0f);
        g.drawFittedText ("MIDI Learn: play a note on your controller...",
                          getLocalBounds().removeFromTop (28),
                          juce::Justification::centred, 1);
    }
}

void KitCanvas::resized()
{
    // Don't reset child bounds from model here — that fights live drag/resize.
}

void KitCanvas::chooseWavFile (std::function<void(const juce::File&)> onChosen)
{
    auto chooser = std::make_shared<juce::FileChooser> ("Select WAV sample", juce::File(), "*.wav");

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [chooser, onChosen = std::move (onChosen)] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();

                              if (file.existsAsFile())
                                  onChosen (file);
                          });
}

void KitCanvas::handleAssignSingleSample (const juce::String& pieceId)
{
    chooseWavFile ([this, pieceId] (const juce::File& file)
    {
        {
            const juce::ScopedLock lock (processor.getModelLock());

            if (auto* piece = processor.getKitModel().findPieceById (pieceId))
                processor.getKitModel().assignSingleSample (*piece, file);
        }

        processor.rebuildEngine();
        refreshPieces();
    });
}

void KitCanvas::handleAddVelocityLayer (const juce::String& pieceId)
{
    auto* dialog = new juce::AlertWindow ("Add Velocity Layer", "Set velocity range (1-127):", juce::AlertWindow::QuestionIcon);
    dialog->addTextEditor ("min", "1", "Min velocity:");
    dialog->addTextEditor ("max", "127", "Max velocity:");
    dialog->addButton ("Next", 1);
    dialog->addButton ("Cancel", 0);

    dialog->enterModalState (true, juce::ModalCallbackFunction::create ([this, dialog, pieceId] (int result)
    {
        if (result == 1)
        {
            const int minVel = dialog->getTextEditorContents ("min").getIntValue();
            const int maxVel = dialog->getTextEditorContents ("max").getIntValue();

            chooseWavFile ([this, pieceId, minVel, maxVel] (const juce::File& file)
            {
                {
                    const juce::ScopedLock lock (processor.getModelLock());

                    if (auto* piece = processor.getKitModel().findPieceById (pieceId))
                        processor.getKitModel().addVelocityLayer (*piece, minVel, maxVel, file);
                }

                processor.rebuildEngine();
                refreshPieces();
            });
        }

        delete dialog;
    }));
}

void KitCanvas::handleAddRoundRobin (const juce::String& pieceId)
{
    chooseWavFile ([this, pieceId] (const juce::File& file)
    {
        {
            const juce::ScopedLock lock (processor.getModelLock());

            if (auto* piece = processor.getKitModel().findPieceById (pieceId))
            {
                juce::String layerId;

                if (const auto* art = piece->getPrimaryArticulation())
                    if (! art->layers.empty())
                        layerId = art->layers.back().id;

                processor.getKitModel().addRoundRobinSample (*piece, layerId, file);
            }
        }

        processor.rebuildEngine();
        refreshPieces();
    });
}

void KitCanvas::handleAddVelocityLayerForSelection()
{
    if (selectedPieceId.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                                                "No Selection",
                                                "Select a drum piece first (click it in edit or normal mode).");
        return;
    }

    handleAddVelocityLayer (selectedPieceId);
}

void KitCanvas::wirePieceCallbacks (DrumPieceComponent& comp)
{
    comp.onSelected = [this] (const juce::String& id) { setSelectedPieceId (id); };

    comp.onPieceChanged = [this] (const juce::String&)
    {
        applyDisplayLayerOrder();

        if (onModelChanged)
            onModelChanged();
    };

    comp.onRequestAudition = [this] (const juce::String& pieceId)
    {
        processor.triggerPiece (pieceId, 127.0f);
    };

    comp.onRequestDelete = [this] (const juce::String& pieceId)
    {
        syncAllPieceLayoutsToModel (false);

        {
            const juce::ScopedLock lock (processor.getModelLock());
            processor.getKitModel().removePiece (pieceId, false);
        }

        if (selectedPieceId == pieceId)
            selectedPieceId.clear();

        processor.rebuildEngine();
        rebuildFromModel();
    };

    comp.onRequestLearnMidi = [this] (const juce::String& pieceId)
    {
        processor.getMidiLearnManager().startLearning (pieceId);
        repaint();
    };

    comp.onRequestAssignSample = [this] (const juce::String& pieceId) { handleAssignSingleSample (pieceId); };
    comp.onRequestAddVelocityLayer = [this] (const juce::String& pieceId) { handleAddVelocityLayer (pieceId); };
    comp.onRequestAddRoundRobin = [this] (const juce::String& pieceId) { handleAddRoundRobin (pieceId); };
    comp.onRequestRename = [this] (const juce::String& pieceId) { handleRename (pieceId); };
}

void KitCanvas::handleRename (const juce::String& pieceId)
{
    juce::String currentName;

    {
        const juce::ScopedLock lock (processor.getModelLock());

        if (const auto* piece = processor.getKitModel().findPieceById (pieceId))
            currentName = piece->name;
        else
            return;
    }

    auto* w = new juce::AlertWindow ("Rename Piece", "Enter a new name:", juce::AlertWindow::QuestionIcon);
    w->addTextEditor ("name", currentName, "Name:");
    w->addButton ("OK", 1);
    w->addButton ("Cancel", 0);

    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w, pieceId] (int result)
    {
        if (result == 1)
        {
            const auto newName = w->getTextEditorContents ("name");

            if (newName.isNotEmpty())
            {
                const juce::ScopedLock lock (processor.getModelLock());

                if (auto* piece = processor.getKitModel().findPieceById (pieceId))
                    piece->name = newName;
            }

            processor.getKitModel().notifyChanged();
            refreshPieces();
        }

        delete w;
    }));
}
