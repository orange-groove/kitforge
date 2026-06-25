#include "PluginEditor.h"
#include "UI/KitBuilderPanel.h"
#include "UI/KitCanvas.h"
#include "UI/LibraryBrowserPanel.h"
#include "Standalone/KitForgeStandaloneHeader.h"

KitForgeAudioProcessorEditor::KitForgeAudioProcessorEditor (KitForgeAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    kitBuilder = std::make_unique<KitBuilderPanel> (processorRef);
    libraryBrowser = std::make_unique<LibraryBrowserPanel> (processorRef);
    libraryBrowser->onKitInstalled = [this]
    {
        kitBuilder->getCanvas().rebuildFromModel();
        mainTabs.setCurrentTabIndex (0);
    };

    mainTabs.addTab ("Kit Builder", juce::Colour (0xff242424), kitBuilder.get(), false);
    mainTabs.addTab ("Library Browser", juce::Colour (0xff1e1e1e), libraryBrowser.get(), false);

    addAndMakeVisible (mainTabs);

    isStandaloneApp = (processorRef.wrapperType == juce::AudioProcessor::wrapperType_Standalone);

    if (isStandaloneApp)
    {
        standaloneHeader = std::make_unique<KitForgeStandaloneHeader>();
        addAndMakeVisible (*standaloneHeader);
    }

    processorRef.getKitModel().addListener ([this]
    {
        juce::MessageManager::callAsync ([this]
        {
            kitBuilder->getCanvas().refreshPieces();
            repaint();
        });
    });

    setSize (980, 680);
}

KitForgeAudioProcessorEditor::~KitForgeAudioProcessorEditor() = default;

void KitForgeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff242424));
}

void KitForgeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    if (standaloneHeader != nullptr)
    {
        standaloneHeader->setBounds (area.removeFromTop (36));
    }

    mainTabs.setBounds (area);
}
