#include "ImportPreviewPanel.h"

namespace
{
    class ImportPreviewDialog final : public juce::DialogWindow
    {
    public:
        ImportPreviewDialog (ImportResult result,
                             ImportPreviewPanel::ImportConfirmedCallback onImport,
                             ImportPreviewPanel::CancelledCallback onCancel,
                             ImportPreviewPanel::SfzReimportFunction reimportFn)
            : DialogWindow ("Import Preview", juce::Colours::darkgrey, true, true)
        {
            setContentOwned (new ImportPreviewPanel (std::move (result),
                                                     std::move (onImport),
                                                     std::move (onCancel),
                                                     std::move (reimportFn)),
                             true);
            setResizable (true, true);
            setResizeLimits (480, 460, 900, 820);
            centreWithSize (560, 560);
        }

        void closeButtonPressed() override { setVisible (false); }
    };
}

ImportPreviewPanel::ImportPreviewPanel (ImportResult resultIn,
                                        ImportConfirmedCallback onImportIn,
                                        CancelledCallback onCancelIn,
                                        SfzReimportFunction reimportFnIn)
    : importResult (std::move (resultIn)),
      onImport (std::move (onImportIn)),
      onCancel (std::move (onCancelIn)),
      reimportFn (std::move (reimportFnIn)),
      titleLabel ({}, "Import Preview")
{
    titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    sfzHeader.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addChildComponent (sfzHeader);
    addChildComponent (sfzSelector);
    setupSfzSelector();

    summaryEditor.setMultiLine (true, true);
    summaryEditor.setReadOnly (true);
    summaryEditor.setCaretVisible (false);
    addAndMakeVisible (summaryEditor);
    refreshSummary();

    warningsHeader.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (warningsHeader);

    warningsEditor.setMultiLine (true, true);
    warningsEditor.setReadOnly (true);
    warningsEditor.setCaretVisible (false);
    warningsEditor.setText (buildWarningsText());
    addAndMakeVisible (warningsEditor);

    addAndMakeVisible (importButton);
    addAndMakeVisible (cancelButton);

    importButton.onClick = [this]
    {
        if (onImport)
            onImport (std::move (importResult));

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

void ImportPreviewPanel::showDialog (juce::Component* parent,
                                      ImportResult result,
                                      ImportConfirmedCallback onImport,
                                      CancelledCallback onCancel,
                                      SfzReimportFunction reimportFn)
{
    auto* dialog = new ImportPreviewDialog (std::move (result),
                                            std::move (onImport),
                                            std::move (onCancel),
                                            std::move (reimportFn));
    dialog->setVisible (true);
    dialog->enterModalState (true,
                             juce::ModalCallbackFunction::create ([dialog] (int) { delete dialog; }),
                             parent != nullptr);
}

void ImportPreviewPanel::setupSfzSelector()
{
    if (importResult.sfzCandidates.size() <= 1)
        return;

    sfzHeader.setVisible (true);
    sfzSelector.setVisible (true);

    int selectedId = 1;

    for (int i = 0; i < importResult.sfzCandidates.size(); ++i)
    {
        const auto& file = importResult.sfzCandidates.getReference (i);
        sfzSelector.addItem (file.getFileName(), i + 1);

        if (importResult.selectedSfzFile == file)
            selectedId = i + 1;
    }

    sfzSelector.setSelectedId (selectedId, juce::dontSendNotification);

    sfzSelector.onChange = [this]
    {
        const int index = sfzSelector.getSelectedId() - 1;

        if (! juce::isPositiveAndBelow (index, importResult.sfzCandidates.size()))
            return;

        const auto sfzFile = importResult.sfzCandidates.getReference (index);

        if (reimportFn)
        {
            importResult = reimportFn (sfzFile);
            importResult.selectedSfzFile = sfzFile;
        }
        else
        {
            importResult.selectedSfzFile = sfzFile;
        }

        refreshSummary();
        warningsEditor.setText (buildWarningsText());
    };
}

void ImportPreviewPanel::refreshSummary()
{
    summaryEditor.setText (buildSummaryText());
}

void ImportPreviewPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
}

void ImportPreviewPanel::resized()
{
    auto area = getLocalBounds().reduced (12);
    titleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (8);

    if (sfzSelector.isVisible())
    {
        sfzHeader.setBounds (area.removeFromTop (18));
        sfzSelector.setBounds (area.removeFromTop (26));
        area.removeFromTop (8);
    }

    auto buttonRow = area.removeFromBottom (32);
    cancelButton.setBounds (buttonRow.removeFromRight (100));
    buttonRow.removeFromRight (8);
    importButton.setBounds (buttonRow.removeFromRight (100));

    area.removeFromBottom (8);
    warningsHeader.setBounds (area.removeFromBottom (20));
    warningsEditor.setBounds (area.removeFromBottom (120));
    area.removeFromBottom (8);
    summaryEditor.setBounds (area);
}

juce::String ImportPreviewPanel::buildSummaryText() const
{
    juce::String text;
    text << "Kit name: " << importResult.kitName << "\n";
    text << "Format: " << importResult.importFormat << "\n";

    if (importResult.selectedSfzFile.existsAsFile())
        text << "SFZ: " << importResult.selectedSfzFile.getFileName() << "\n";

    text << "WAV files: " << importResult.stats.wavFileCount << "\n";
    text << "Library groups: " << importResult.stats.pieceCount << "\n";
    text << "Articulations: " << importResult.stats.articulationCount << "\n";
    text << "Velocity layers: " << importResult.stats.layerCount << "\n";
    text << "Round robin samples: " << importResult.stats.roundRobinCount << "\n\n";
    text << "Next: map these samples onto your current kit layout (your piece positions and MIDI map are kept).\n\n";
    text << buildPieceDetailsText();
    return text;
}

juce::String ImportPreviewPanel::buildPieceDetailsText() const
{
    juce::String text;

    for (const auto& piece : importResult.kit.getPieces())
    {
        text << piece.name << " (" << drumPieceTypeToString (piece.type) << ")\n";

        for (const auto& art : piece.articulations)
        {
            text << "  - " << art.name << " (MIDI " << art.midiNote << "): "
                 << art.layers.size() << " layer(s)";

            int rrTotal = 0;

            for (const auto& layer : art.layers)
                rrTotal += layer.roundRobins.size();

            text << ", " << rrTotal << " round robin sample(s)\n";
        }
    }

    return text;
}

juce::String ImportPreviewPanel::buildWarningsText() const
{
    if (importResult.warnings.empty())
        return "No warnings.";

    juce::String text;

    for (const auto& warning : importResult.warnings)
    {
        text << importWarningTypeToString (warning.type) << ": " << warning.message;

        if (warning.filePath.isNotEmpty())
            text << " [" << juce::File (warning.filePath).getFileName() << "]";

        text << "\n";
    }

    return text;
}
