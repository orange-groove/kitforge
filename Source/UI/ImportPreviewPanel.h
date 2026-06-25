#pragma once

#include <JuceHeader.h>
#include "../Importers/ImportResult.h"

/** Preview screen shown after scanning a library import, before installing. */
class ImportPreviewPanel final : public juce::Component
{
public:
    using ImportConfirmedCallback = std::function<void (ImportResult)>;
    using CancelledCallback = std::function<void()>;
    using SfzReimportFunction = std::function<ImportResult (const juce::File& sfzFile)>;

    ImportPreviewPanel (ImportResult resultIn,
                        ImportConfirmedCallback onImportIn,
                        CancelledCallback onCancelIn,
                        SfzReimportFunction reimportFnIn = {});

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** Shows a modal dialog with the import preview. */
    static void showDialog (juce::Component* parent,
                            ImportResult result,
                            ImportConfirmedCallback onImport,
                            CancelledCallback onCancel = {},
                            SfzReimportFunction reimportFn = {});

private:
    ImportResult importResult;
    ImportConfirmedCallback onImport;
    CancelledCallback onCancel;
    SfzReimportFunction reimportFn;

    juce::Label titleLabel;
    juce::Label sfzHeader { {}, "SFZ File" };
    juce::ComboBox sfzSelector;
    juce::TextEditor summaryEditor;
    juce::TextEditor warningsEditor;
    juce::Label warningsHeader { {}, "Warnings" };
    juce::TextButton importButton { "Map to Current Kit" };
    juce::TextButton cancelButton { "Cancel" };

    void refreshSummary();
    juce::String buildSummaryText() const;
    juce::String buildWarningsText() const;
    juce::String buildPieceDetailsText() const;
    void setupSfzSelector();
};
