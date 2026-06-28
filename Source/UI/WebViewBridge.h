#pragma once

#include <JuceHeader.h>

class KitForgeAudioProcessor;

#if JUCE_WEB_BROWSER

/** JSON message bridge between the embedded WebView UI and KitForge C++ backend. */
class WebViewBridge final : public juce::Component
{
public:
    static constexpr const char* kNativeFunctionName = "kitforgeMessage";
    static constexpr const char* kNativeEventName    = "kitforgeFromNative";

    explicit WebViewBridge (KitForgeAudioProcessor& processorIn);

    void attachToBrowser (juce::WebBrowserComponent& browser);
    void setCanvasSize (float width, float height);

    void handleMessageFromWeb (const juce::String& jsonText);
    void sendToWeb (const juce::var& message);

    /** Entry points for the native menu bar (mirror the UI's File menu actions). */
    void requestSaveKit();
    void requestSaveKitAs();
    void requestLoadKit();

    /** Invoked when the UI's resize grip requests a new editor size (width, height in px).
        Set by the editor; resizing the editor this way works in hosts where the native
        WebView covers the host/JUCE resize corner (e.g. FL Studio attached mode on macOS). */
    std::function<void (int, int)> onResizeEditorRequested;

    void pushKitState();
    void pushCatalogState();
    void sendError (const juce::String& message);
    void sendBusy (const juce::String& label);

    juce::WebBrowserComponent::Options buildBrowserOptions();

private:
    KitForgeAudioProcessor& processor;
    juce::WebBrowserComponent* browser = nullptr;
    float canvasWidth  = 980.0f;
    float canvasHeight = 680.0f;
    bool wasMidiLearning = false;

    std::atomic<bool> kitStatePushScheduled { false };
    std::atomic<bool> kitStateDirty { false };

    juce::FileChooser loadKitFileChooser;
    juce::FileChooser saveKitFileChooser;

    /** File backing the currently loaded kit; empty until the user loads/saves a kit file. */
    juce::File currentKitFile;
    juce::FileChooser importSfzFileChooser;
    juce::FileChooser importFolderFileChooser;
    juce::FileChooser importKitforgeFileChooser;
    juce::FileChooser assignSampleFileChooser;

    void schedulePushKitState();

    void handleReady();
    void dispatchMessage (const juce::var& message);
    juce::var buildKitStateMessage() const;
    juce::var buildCatalogStateMessage() const;

    void handleTriggerPiece (const juce::var& message);
    void handleMovePiece (const juce::var& message);
    void handleResizePiece (const juce::var& message);
    void handleLearnMidi (const juce::var& message);
    void handleCancelLearnMidi (const juce::var& message);
    void handleAssignSample (const juce::var& message);
    void handleUpdatePiece (const juce::var& message);
    void handleRenamePiece (const juce::var& message);
    void handleSetArticulationMidi (const juce::var& message);
    void handleReorderPiece (const juce::var& message);
    void handleDeletePiece (const juce::var& message);
    void handleResizeEditor (const juce::var& message);
    void handleSaveKit();
    void handleSaveKitAs();
    void handleLoadKit();
    void handleImportSfz();
    void handleImportLooseFolder();
    void handleInstallKitforge();
    void handleRemoveKit (const juce::var& message);
    void handleUseLibrary (const juce::var& message);
    void handleLoadInstalledKit (const juce::var& message);
    void handleRevealKit (const juce::var& message);
    void handleGetLibraryMapping (const juce::var& message);
    void handleApplyLibraryMapping (const juce::var& message);
    void handleSearchSampleSets (const juce::var& message);
    void handlePreviewSampleSet (const juce::var& message);
    void handleSwapSampleSet (const juce::var& message);
    void handleRebuildSampleIndex (const juce::var& message);
    void sendSampleIndexState();
    void sendKitValidation();
    void handleAiBuildKit (const juce::var& message);

    void onModelChanged();
    void checkMidiLearnTransition();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebViewBridge)
};

#endif
