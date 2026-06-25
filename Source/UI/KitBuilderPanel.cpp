#include "KitBuilderPanel.h"
#include "KitCanvas.h"
#include "AIBuilderPanel.h"
#include "MixerPanel.h"
#include "SettingsPanel.h"
#include "../Serialization/KitSerializer.h"
#include "../PluginProcessor.h"
#include "../Models/DrumPieceTypes.h"

KitBuilderPanel::KitBuilderPanel (KitForgeAudioProcessor& processorIn)
    : processorRef (processorIn)
{
    canvas = std::make_unique<KitCanvas> (processorRef);
    addAndMakeVisible (*canvas);

    aiOverlay = std::make_unique<AIBuilderPanel> (processorRef,
        [this]
        {
            return juce::Point<float> ((float) canvas->getWidth(), (float) canvas->getHeight());
        });
    addChildComponent (*aiOverlay);
    aiOverlay->onKitApplied = [this]
    {
        canvas->rebuildFromModel();
    };
    aiOverlay->onDismiss = [this]
    {
        kitAiButton.setToggleState (false, juce::dontSendNotification);
        resized();
    };

    for (auto* button : { static_cast<juce::Component*> (&addDrumButton),
                          static_cast<juce::Component*> (&addCymbalButton),
                          static_cast<juce::Component*> (&addAccessoryButton),
                          static_cast<juce::Component*> (&editLayoutButton),
                          static_cast<juce::Component*> (&saveKitButton),
                          static_cast<juce::Component*> (&loadKitButton),
                          static_cast<juce::Component*> (&mixerButton),
                          static_cast<juce::Component*> (&settingsButton),
                          static_cast<juce::Component*> (&kitAiButton),
                          static_cast<juce::Component*> (&addSampleLayerButton) })
        addAndMakeVisible (button);

    addDrumButton.onClick = [this] { handleAddDrumMenu(); };
    addCymbalButton.onClick = [this] { handleAddCymbalMenu(); };
    addAccessoryButton.onClick = [this] { handleAddAccessory(); };
    editLayoutButton.onClick = [this] { canvas->setEditMode (editLayoutButton.getToggleState()); };
    saveKitButton.onClick = [this] { handleSaveKit(); };
    loadKitButton.onClick = [this] { handleLoadKit(); };
    mixerButton.onClick = [this] { handleOpenMixer(); };
    settingsButton.onClick = [this] { handleOpenSettings(); };
    addSampleLayerButton.onClick = [this] { canvas->handleAddVelocityLayerForSelection(); };

    const bool isStandalone = processorRef.wrapperType == juce::AudioProcessor::wrapperType_Standalone;
    settingsButton.setVisible (! isStandalone);

    kitAiButton.setClickingTogglesState (true);
    kitAiButton.onClick = [this]
    {
        const bool show = kitAiButton.getToggleState();
        aiOverlay->setVisible (show);

        if (show)
        {
            aiOverlay->toFront (false);
            aiOverlay->grabKeyboardFocus();
        }

        resized();
    };
}

KitBuilderPanel::~KitBuilderPanel() = default;

void KitBuilderPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff242424));
}

void KitBuilderPanel::resized()
{
    auto area = getLocalBounds();
    auto toolbar = area.removeFromTop (44).reduced (6, 4);

    const int buttonWidth = 108;
    addDrumButton.setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (4);
    addCymbalButton.setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (4);
    addAccessoryButton.setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (4);
    editLayoutButton.setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (8);
    addSampleLayerButton.setBounds (toolbar.removeFromLeft (buttonWidth + 20));
    toolbar.removeFromLeft (8);
    saveKitButton.setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (4);
    loadKitButton.setBounds (toolbar.removeFromLeft (buttonWidth));
    toolbar.removeFromLeft (4);
    mixerButton.setBounds (toolbar.removeFromLeft (buttonWidth));

    if (settingsButton.isVisible())
    {
        toolbar.removeFromLeft (4);
        settingsButton.setBounds (toolbar.removeFromLeft (buttonWidth));
    }

    toolbar.removeFromLeft (12);
    kitAiButton.setBounds (toolbar.removeFromRight (buttonWidth));

    canvas->setBounds (area.reduced (8));

    if (aiOverlay->isVisible())
    {
        auto overlayBounds = getLocalBounds().reduced (8);
        overlayBounds = overlayBounds.removeFromBottom (kAiOverlayHeight);
        aiOverlay->setBounds (overlayBounds);
        aiOverlay->toFront (false);
    }
}

void KitBuilderPanel::handleAddDrumMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Kick");
    menu.addItem (2, "Snare");
    menu.addItem (3, "Rack Tom");
    menu.addItem (4, "Floor Tom");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (addDrumButton),
                        [this] (int result)
                        {
                            switch (result)
                            {
                                case 1: handleAddDrumType (DrumPieceType::kick); break;
                                case 2: handleAddDrumType (DrumPieceType::snare); break;
                                case 3: handleAddDrumType (DrumPieceType::rackTom); break;
                                case 4: handleAddDrumType (DrumPieceType::floorTom); break;
                                default: break;
                            }
                        });
}

void KitBuilderPanel::handleAddCymbalMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Hi-Hat");
    menu.addItem (2, "Crash");
    menu.addItem (3, "Ride");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (addCymbalButton),
                        [this] (int result)
                        {
                            switch (result)
                            {
                                case 1: handleAddCymbalType (DrumPieceType::hiHat); break;
                                case 2: handleAddCymbalType (DrumPieceType::crash); break;
                                case 3: handleAddCymbalType (DrumPieceType::ride); break;
                                default: break;
                            }
                        });
}

void KitBuilderPanel::handleAddDrumType (DrumPieceType type)
{
    const auto bounds = canvas->getBounds().toFloat();

    {
        const juce::ScopedLock lock (processorRef.getModelLock());
        processorRef.getKitModel().addDefaultDrum (type, bounds.getWidth(), bounds.getHeight());
    }

    processorRef.rebuildEngine();
    canvas->rebuildFromModel();
}

void KitBuilderPanel::handleAddCymbalType (DrumPieceType type)
{
    const auto bounds = canvas->getBounds().toFloat();

    {
        const juce::ScopedLock lock (processorRef.getModelLock());
        processorRef.getKitModel().addDefaultCymbal (type, bounds.getWidth(), bounds.getHeight());
    }

    processorRef.rebuildEngine();
    canvas->rebuildFromModel();
}

void KitBuilderPanel::handleAddAccessory()
{
    const auto bounds = canvas->getBounds().toFloat();

    {
        const juce::ScopedLock lock (processorRef.getModelLock());
        processorRef.getKitModel().addDefaultAccessory (bounds.getWidth(), bounds.getHeight());
    }

    processorRef.rebuildEngine();
    canvas->rebuildFromModel();
}

void KitBuilderPanel::handleSaveKit()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Save Kit", juce::File(), "*.json");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();

                              if (file == juce::File())
                                  return;

                              if (! file.hasFileExtension (".json"))
                                  file = file.withFileExtension (".json");

                              const juce::ScopedLock lock (processorRef.getModelLock());
                              KitSerializer::saveKitToFile (processorRef.getKitModel(), file);
                          });
}

void KitBuilderPanel::handleLoadKit()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Load Kit", juce::File(), "*.json");

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();

                              if (! file.existsAsFile())
                                  return;

                              bool loaded = false;

                              {
                                  const juce::ScopedLock lock (processorRef.getModelLock());
                                  loaded = KitSerializer::loadKitFromFile (processorRef.getKitModel(), file);
                              }

                              if (! loaded)
                              {
                                  juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                                          "Load Kit",
                                                                          "Could not load kit file.");
                                  return;
                              }

                              processorRef.rebuildEngine();
                              canvas->rebuildFromModel();
                          });
}

void KitBuilderPanel::handleOpenMixer()
{
    MixerPanel::showWindow (this, processorRef);
}

void KitBuilderPanel::handleOpenSettings()
{
    SettingsPanel::showWindow (this, processorRef);
}
