#include "CatalogPanel.h"
#include "../PluginProcessor.h"
#include "../Catalog/KitInstaller.h"

CatalogPanel::CatalogPanel (KitForgeAudioProcessor& processorIn)
    : processorRef (processorIn)
{
    addAndMakeVisible (titleLabel);
    titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));

    addAndMakeVisible (searchBox);
    searchBox.setTextToShowWhenEmpty ("Search kits...", juce::Colours::grey);
    searchBox.onTextChange = [this] { updateVisibleKits(); };

    addAndMakeVisible (refreshButton);
    refreshButton.onClick = [this]
    {
        statusLabel.setText ("Refreshing catalog...", juce::dontSendNotification);
        processorRef.getServices().getCatalog().refreshManifest ([this] (bool ok, const CatalogManifest&)
        {
            juce::MessageManager::callAsync ([this, ok]
            {
                refresh();
                statusLabel.setText (ok ? "Catalog updated." : "Using cached / offline catalog.",
                                     juce::dontSendNotification);
            });
        });
    };

    addAndMakeVisible (installButton);
    installButton.onClick = [this] { installSelectedKit(); };

    addAndMakeVisible (removeButton);
    removeButton.onClick = [this] { removeSelectedKit(); };

    addAndMakeVisible (statusLabel);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);

    addAndMakeVisible (progressBar);
    progressBar.setVisible (false);

    addAndMakeVisible (kitList);
    refresh();
}

void CatalogPanel::refresh()
{
    updateVisibleKits();
    kitList.updateContent();
    kitList.repaint();
    updateButtonStates();
}

void CatalogPanel::updateVisibleKits()
{
    visibleKits = processorRef.getServices().getCatalog().search (searchBox.getText());
    selectedRow = juce::jlimit (-1, visibleKits.size() - 1, selectedRow);
    kitList.updateContent();
    updateButtonStates();
}

void CatalogPanel::updateButtonStates()
{
    const auto* kit = getSelectedKit();
    const bool hasSelection = kit != nullptr;
    const auto& catalog = processorRef.getServices().getCatalog();
    const bool installed = hasSelection && catalog.isKitInstalled (kit->id);
    const bool needsReinstall = hasSelection && catalog.kitNeedsReinstall (*kit);
    const bool busy = processorRef.getServices().getKitInstaller().isBusy();

    installButton.setButtonText (needsReinstall ? "Reinstall" : "Install");
    installButton.setEnabled (hasSelection && (! installed || needsReinstall) && ! busy);
    removeButton.setEnabled (hasSelection && installed && ! busy);
}

const CatalogKitEntry* CatalogPanel::getSelectedKit() const
{
    if (! juce::isPositiveAndBelow (selectedRow, visibleKits.size()))
        return nullptr;

    return &visibleKits.getReference (selectedRow);
}

void CatalogPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));
}

void CatalogPanel::resized()
{
    auto area = getLocalBounds().reduced (12);
    titleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (8);

    auto searchRow = area.removeFromTop (28);
    searchBox.setBounds (searchRow.removeFromLeft (searchRow.getWidth() - 90));
    searchRow.removeFromLeft (8);
    refreshButton.setBounds (searchRow);

    area.removeFromTop (8);

    auto buttonRow = area.removeFromTop (28);
    installButton.setBounds (buttonRow.removeFromLeft (100));
    buttonRow.removeFromLeft (8);
    removeButton.setBounds (buttonRow.removeFromLeft (100));

    area.removeFromTop (6);
    progressBar.setBounds (area.removeFromTop (18));
    area.removeFromTop (4);
    statusLabel.setBounds (area.removeFromTop (20));
    area.removeFromTop (8);
    kitList.setBounds (area);
}

int CatalogPanel::getNumRows()
{
    return visibleKits.size();
}

void CatalogPanel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (! juce::isPositiveAndBelow (row, visibleKits.size()))
        return;

    const auto& kit = visibleKits.getReference (row);
    const auto& catalog = processorRef.getServices().getCatalog();
    const bool installed = catalog.isKitInstalled (kit.id);
    const bool needsReinstall = catalog.kitNeedsReinstall (kit);

    if (selected)
        g.fillAll (juce::Colour (0xff3d5a80));

    g.setColour (juce::Colours::white);
    g.setFont (14.0f);

    juce::String title = kit.name + "  (" + kit.format + ", " + juce::String (kit.sizeMb, 0) + " MB)";

    if (installed)
        title += needsReinstall ? "  [Needs reinstall]" : "  [Installed]";

    g.drawText (title, 8, 0, w - 16, h / 2, juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::grey);
    g.setFont (12.0f);
    g.drawText (kit.tags.joinIntoString (", "), 8, h / 2, w - 16, h / 2, juce::Justification::centredLeft, true);
}

void CatalogPanel::listBoxItemClicked (int row, const juce::MouseEvent& e)
{
    selectedRow = row;
    updateButtonStates();

    if (e.getNumberOfClicks() >= 2)
        installSelectedKit();
}

void CatalogPanel::installSelectedKit()
{
    const auto* kit = getSelectedKit();

    if (kit == nullptr)
        return;

    if (processorRef.getServices().getCatalog().isKitInstalled (kit->id)
        && ! processorRef.getServices().getCatalog().kitNeedsReinstall (*kit))
    {
        statusLabel.setText ("Kit already installed.", juce::dontSendNotification);
        return;
    }

    installProgress = 0.0;
    progressBar.setVisible (true);
    installButton.setEnabled (false);
    removeButton.setEnabled (false);
    statusLabel.setText ("Starting install...", juce::dontSendNotification);

    processorRef.getServices().getKitInstaller().installKitAsync (*kit,
        [this] (const KitInstallProgress& progress)
        {
            installProgress = progress.progress;

            switch (progress.stage)
            {
                case KitInstallStage::downloading:
                    statusLabel.setText ("Downloading: " + progress.message, juce::dontSendNotification);
                    break;
                case KitInstallStage::extracting:
                    statusLabel.setText ("Extracting archive...", juce::dontSendNotification);
                    break;
                case KitInstallStage::importing:
                    statusLabel.setText ("Importing kit...", juce::dontSendNotification);
                    break;
                case KitInstallStage::finished:
                    statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
                    statusLabel.setText (progress.message, juce::dontSendNotification);
                    break;
                case KitInstallStage::failed:
                    statusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
                    statusLabel.setText (progress.message, juce::dontSendNotification);
                    break;
                default: break;
            }

            repaint();
        },
        [this, kitId = kit->id] (const KitInstallResult& result)
        {
            progressBar.setVisible (false);
            updateButtonStates();

            if (result.success && result.hasKitModel)
            {
                {
                    const juce::ScopedLock lock (processorRef.getModelLock());
                    processorRef.getKitModel().importContents (result.kit);
                }

                processorRef.rebuildEngine();

                if (onKitInstalled)
                    onKitInstalled();

                statusLabel.setColour (juce::Label::textColourId, juce::Colours::lightgreen);
                statusLabel.setText ("Loaded \"" + result.kit.kitName + "\" into Kit Builder.",
                                     juce::dontSendNotification);
            }
            else if (! result.success)
            {
                statusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
                statusLabel.setText (result.errorMessage, juce::dontSendNotification);
            }

            refresh();
            juce::ignoreUnused (kitId);
        });
}

void CatalogPanel::removeSelectedKit()
{
    const auto* kit = getSelectedKit();

    if (kit == nullptr)
        return;

    processorRef.getServices().getCatalog().removeKit (kit->id);
    processorRef.getServices().getSampleIndex().scanLibrariesOnDisk();
    statusLabel.setText ("Removed " + kit->name, juce::dontSendNotification);
    refresh();
}
