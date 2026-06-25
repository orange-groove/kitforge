#pragma once

#include <JuceHeader.h>
#include "../Models/KitModel.h"

class KitForgeAudioProcessor;
class KitCanvas;

/** Tab 1 — visual kit builder with toolbar + canvas. */
class KitBuilderPanel final : public juce::Component
{
public:
    explicit KitBuilderPanel (KitForgeAudioProcessor& processorIn);
    ~KitBuilderPanel() override;

    KitCanvas& getCanvas() { return *canvas; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    KitForgeAudioProcessor& processorRef;

    juce::TextButton addDrumButton { "Add Drum" };
    juce::TextButton addCymbalButton { "Add Cymbal" };
    juce::TextButton addAccessoryButton { "Add Accessory" };
    juce::ToggleButton editLayoutButton { "Edit Layout" };
    juce::TextButton saveKitButton { "Save Kit" };
    juce::TextButton loadKitButton { "Load Kit" };
    juce::TextButton mixerButton { "Mixer" };
    juce::TextButton settingsButton { "Settings" };
    juce::TextButton kitAiButton { "Kit AI" };
    juce::TextButton addSampleLayerButton { "Add Sample Layer" };

    std::unique_ptr<KitCanvas> canvas;
    std::unique_ptr<class AIBuilderPanel> aiOverlay;

    static constexpr int kAiOverlayHeight = 130;

    void handleAddDrumMenu();
    void handleAddCymbalMenu();
    void handleAddDrumType (DrumPieceType type);
    void handleAddCymbalType (DrumPieceType type);
    void handleAddAccessory();
    void handleSaveKit();
    void handleLoadKit();
    void handleOpenMixer();
    void handleOpenSettings();
};
