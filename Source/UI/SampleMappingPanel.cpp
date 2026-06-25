#include "SampleMappingPanel.h"
#include "../PluginProcessor.h"

namespace
{
    class SampleMappingDialog final : public juce::DialogWindow
    {
    public:
        SampleMappingDialog (SampleMappingPanel* content)
            : DialogWindow ("Map Samples to Kit", juce::Colours::darkgrey, true, true)
        {
            setContentNonOwned (content, true);
            setResizable (true, true);
            setResizeLimits (640, 420, 1100, 900);
            centreWithSize (760, 620);
        }

        void closeButtonPressed() override { setVisible (false); }
    };
}

SampleMappingPanel::MappingRow::MappingRow (const SampleLibraryMapper::SourceEntry& source,
                                            const juce::Array<SampleLibraryMapper::TargetSlot>& targets,
                                            SampleLibraryMapper::Mapping mapping)
    : sourceEntry (source),
      targetSlots (&targets)
{
    enabledToggle.setToggleState (mapping.enabled, juce::dontSendNotification);
    addAndMakeVisible (enabledToggle);

    const juce::String sourceText = source.pieceName + " / " + source.articulationName
                                  + "  (MIDI " + juce::String (source.midiNote) + ", "
                                  + juce::String (source.sampleCount) + " samples)";
    sourceLabel.setText (sourceText, juce::dontSendNotification);
    sourceLabel.setFont (juce::FontOptions (13.0f));
    addAndMakeVisible (sourceLabel);

    targetCombo.addItem ("(Unmapped)", 1);

    for (int i = 0; i < targets.size(); ++i)
        targetCombo.addItem (targets.getReference (i).label, i + 2);

    setMapping (mapping);
    addAndMakeVisible (targetCombo);
}

SampleLibraryMapper::Mapping SampleMappingPanel::MappingRow::getMapping() const
{
    SampleLibraryMapper::Mapping mapping;
    mapping.sourceKey = sourceEntry.key;
    mapping.enabled = enabledToggle.getToggleState();

    const int comboId = targetCombo.getSelectedId();

    if (comboId > 1 && targetSlots != nullptr)
        mapping.targetKey = targetSlots->getReference (comboId - 2).key;

    return mapping;
}

void SampleMappingPanel::MappingRow::setMapping (const SampleLibraryMapper::Mapping& mapping)
{
    enabledToggle.setToggleState (mapping.enabled, juce::dontSendNotification);

    int selectedId = 1;

    if (targetSlots != nullptr && mapping.targetKey.isNotEmpty())
    {
        for (int i = 0; i < targetSlots->size(); ++i)
        {
            if (targetSlots->getReference (i).key == mapping.targetKey)
            {
                selectedId = i + 2;
                break;
            }
        }
    }

    targetCombo.setSelectedId (selectedId, juce::dontSendNotification);
}

void SampleMappingPanel::MappingRow::resized()
{
    auto area = getLocalBounds().reduced (4, 2);
    enabledToggle.setBounds (area.removeFromLeft (28));
    area.removeFromLeft (6);
    targetCombo.setBounds (area.removeFromRight (juce::jmax (240, area.getWidth() / 2)));
    area.removeFromRight (8);
    sourceLabel.setBounds (area);
}

SampleMappingPanel::SampleMappingPanel (const juce::String& libraryNameIn,
                                        KitModel targetKitSnapshot,
                                        KitModel libraryCatalogIn,
                                        KitForgeAudioProcessor* processorIn,
                                        AppliedCallback onApplyIn,
                                        CancelledCallback onCancelIn)
    : libraryName (libraryNameIn),
      targetKit (std::move (targetKitSnapshot)),
      libraryCatalog (std::move (libraryCatalogIn)),
      processorRef (processorIn),
      onApply (std::move (onApplyIn)),
      onCancel (std::move (onCancelIn)),
      titleLabel ({}, "Map Library Samples to Your Kit"),
      subtitleLabel ({}, {})
{
    titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setFont (juce::FontOptions (13.0f));
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (subtitleLabel);

    viewport.setViewedComponent (&rowContainer, false);
    addAndMakeVisible (viewport);

    addAndMakeVisible (autoMapButton);
    addAndMakeVisible (applyButton);
    addAndMakeVisible (cancelButton);

    targetSlots = SampleLibraryMapper::listTargetSlots (targetKit);
    rebuildRows();

    autoMapButton.onClick = [this] { applySuggestedMappings(); };

    applyButton.onClick = [this]
    {
        int applied = 0;

        if (processorRef != nullptr)
        {
            const juce::ScopedLock lock (processorRef->getModelLock());
            applied = SampleLibraryMapper::applyMappings (processorRef->getKitModel(),
                                                          libraryCatalog,
                                                          collectMappings());
            processorRef->rebuildEngine();
        }

        if (onApply)
            onApply (applied);

        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState (1);
    };

    cancelButton.onClick = [this]
    {
        if (onCancel)
            onCancel();

        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState (0);
    };
}

void SampleMappingPanel::showDialog (juce::Component* parent,
                                      KitForgeAudioProcessor& processor,
                                      const juce::String& libraryName,
                                      KitModel libraryCatalog,
                                      std::function<void()> onComplete)
{
    KitModel targetSnapshot;

    {
        const juce::ScopedLock lock (processor.getModelLock());
        targetSnapshot = processor.getKitModel();
    }

    if (targetSnapshot.getPieces().empty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                "Map Samples",
                                                "Create or load a kit layout first, then import a sample library.");
        return;
    }

    auto* panel = new SampleMappingPanel (libraryName,
                                          std::move (targetSnapshot),
                                          std::move (libraryCatalog),
                                          &processor,
                                          [onComplete] (int)
                                          {
                                              if (onComplete)
                                                  onComplete();
                                          },
                                          {});

    auto* dialog = new SampleMappingDialog (panel);
    dialog->setVisible (true);
    dialog->enterModalState (true,
                             juce::ModalCallbackFunction::create ([dialog] (int)
                             {
                                 delete dialog;
                             }),
                             parent != nullptr);
}

void SampleMappingPanel::rebuildRows()
{
    rows.clear();
    rowContainer.removeAllChildren();

    const auto sources = SampleLibraryMapper::listLibrarySources (libraryCatalog);
    const auto suggested = SampleLibraryMapper::suggestMappings (targetKit, libraryCatalog);

    int y = 0;
    const int rowHeight = 34;

    for (int i = 0; i < sources.size(); ++i)
    {
        auto mapping = i < suggested.size() ? suggested.getReference (i)
                                            : SampleLibraryMapper::Mapping { sources.getReference (i).key, {}, true };

        auto* row = rows.add (new MappingRow (sources.getReference (i), targetSlots, mapping));
        rowContainer.addAndMakeVisible (row);
        row->setBounds (0, y, 720, rowHeight);
        y += rowHeight;
    }

    rowContainer.setSize (720, y);

    subtitleLabel.setText ("Library: " + libraryName
                             + "  |  Your kit: " + targetKit.kitName
                             + "  (" + juce::String (targetSlots.size()) + " articulation slots)",
                             juce::dontSendNotification);

    resized();
}

juce::Array<SampleLibraryMapper::Mapping> SampleMappingPanel::collectMappings() const
{
    juce::Array<SampleLibraryMapper::Mapping> mappings;

    for (const auto* row : rows)
        mappings.add (row->getMapping());

    return mappings;
}

void SampleMappingPanel::applySuggestedMappings()
{
    const auto suggested = SampleLibraryMapper::suggestMappings (targetKit, libraryCatalog);

    for (int i = 0; i < rows.size() && i < suggested.size(); ++i)
        rows.getUnchecked (i)->setMapping (suggested.getReference (i));
}

void SampleMappingPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
}

void SampleMappingPanel::resized()
{
    auto area = getLocalBounds().reduced (12);
    titleLabel.setBounds (area.removeFromTop (28));
    subtitleLabel.setBounds (area.removeFromTop (22));
    area.removeFromTop (8);

    auto buttonRow = area.removeFromBottom (32);
    cancelButton.setBounds (buttonRow.removeFromRight (100));
    buttonRow.removeFromRight (8);
    applyButton.setBounds (buttonRow.removeFromRight (120));
    buttonRow.removeFromRight (8);
    autoMapButton.setBounds (buttonRow.removeFromRight (100));

    area.removeFromBottom (8);
    viewport.setBounds (area);

    const int width = juce::jmax (640, viewport.getWidth() - viewport.getScrollBarThickness());
    rowContainer.setSize (width, rowContainer.getHeight());

    int y = 0;

    for (auto* row : rows)
    {
        row->setBounds (0, y, width, 34);
        y += 34;
    }
}
