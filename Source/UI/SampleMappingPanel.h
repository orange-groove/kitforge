#pragma once

#include <JuceHeader.h>
#include "../Importers/SampleLibraryMapper.h"
#include "../Models/KitModel.h"

class KitForgeAudioProcessor;

/** Modal UI to map an imported sample library onto the current kit layout. */
class SampleMappingPanel final : public juce::Component
{
public:
    using AppliedCallback = std::function<void (int mappingsApplied)>;
    using CancelledCallback = std::function<void()>;

    SampleMappingPanel (const juce::String& libraryName,
                        KitModel targetKitSnapshot,
                        KitModel libraryCatalog,
                        KitForgeAudioProcessor* processorIn,
                        AppliedCallback onApplyIn,
                        CancelledCallback onCancelIn = {});

    void paint (juce::Graphics& g) override;
    void resized() override;

    static void showDialog (juce::Component* parent,
                            KitForgeAudioProcessor& processor,
                            const juce::String& libraryName,
                            KitModel libraryCatalog,
                            std::function<void()> onComplete = {});

private:
    class MappingRow final : public juce::Component
    {
    public:
        MappingRow (const SampleLibraryMapper::SourceEntry& source,
                    const juce::Array<SampleLibraryMapper::TargetSlot>& targets,
                    SampleLibraryMapper::Mapping mapping);

        SampleLibraryMapper::Mapping getMapping() const;
        void setMapping (const SampleLibraryMapper::Mapping& mapping);

        void resized() override;

    private:
        SampleLibraryMapper::SourceEntry sourceEntry;
        const juce::Array<SampleLibraryMapper::TargetSlot>* targetSlots = nullptr;
        juce::ToggleButton enabledToggle;
        juce::Label sourceLabel;
        juce::ComboBox targetCombo;
    };

    juce::String libraryName;
    KitModel targetKit;
    KitModel libraryCatalog;
    KitForgeAudioProcessor* processorRef = nullptr;
    AppliedCallback onApply;
    CancelledCallback onCancel;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Viewport viewport;
    juce::Component rowContainer;
    juce::TextButton autoMapButton { "Auto-Map" };
    juce::TextButton applyButton { "Apply to Kit" };
    juce::TextButton cancelButton { "Cancel" };

    juce::Array<SampleLibraryMapper::TargetSlot> targetSlots;
    juce::OwnedArray<MappingRow> rows;

    void rebuildRows();
    juce::Array<SampleLibraryMapper::Mapping> collectMappings() const;
    void applySuggestedMappings();
};
